// $Id$

#include <limits>
#include <sstream>
#include <fstream>

#include "batchtrafficmanager.hpp"

BatchTrafficManager::BatchTrafficManager(const Configuration &config,
                                         const vector<Network *> &net)
    : SyntheticTrafficManager(&config, net), _last_id(-1), _last_pid(-1),
      _overall_min_batch_time(0), _overall_avg_batch_time(0),
      _overall_max_batch_time(0)
{

  _max_outstanding = config.GetIntArray("max_outstanding_requests");
  if (_max_outstanding.empty())
  {
    _max_outstanding.push_back(config.GetLongInt("max_outstanding_requests"));
  }
  _max_outstanding.resize(_classes, _max_outstanding.back());

  _batch_size = config.GetIntArray("batch_size");
  if (_batch_size.empty())
  {
    _batch_size.push_back(config.GetLongInt("batch_size"));
  }
  _batch_size.resize(_classes, _batch_size.back());

  _batch_count = config.GetLongInt("batch_count");

  _batch_time = new Stats(this, "batch_time", 1.0, 1000);
  _stats["batch_time"] = _batch_time;

  string sent_packets_out_file = config.GetStr("sent_packets_out");
  if (sent_packets_out_file == "")
  {
    _sent_packets_out = NULL;
  }
  else
  {
    _sent_packets_out = new ofstream(sent_packets_out_file.c_str());
  }
}

BatchTrafficManager::~BatchTrafficManager()
{
  delete _batch_time;
  if (_sent_packets_out)
    delete _sent_packets_out;
}

void BatchTrafficManager::_RetireFlit(Flit *f, long long int dest)
{
  _last_id = f->id;
  _last_pid = f->pid;
  TrafficManager::_RetireFlit(f, dest);
}

long long int BatchTrafficManager::_IssuePacket(long long int source, long long int cl)
{
  if (((_max_outstanding[cl] <= 0) ||
       (_requests_outstanding[cl][source] < _max_outstanding[cl])) &&
      (_packet_seq_no[cl][source] < _batch_size[cl]))
  {
    long long int dest = _traffic_pattern[cl]->dest(source);
    long long int size = _GetNextPacketSize(cl);
    long long int time = ((_include_queuing == 1) ? _qtime[cl][source] : _time);
    return _GeneratePacket(source, dest, size, cl, time);
  }
  return -1;
}

void BatchTrafficManager::_ClearStats()
{
  SyntheticTrafficManager::_ClearStats();
  _batch_time->Clear();
}

bool BatchTrafficManager::_SingleSim()
{
  long long int batch_index = 0;
  while (batch_index < _batch_count)
  {
    for (long long int c = 0; c < _classes; ++c)
    {
      _packet_seq_no[c].assign(_nodes, 0);
    }
    _last_id = -1;
    _last_pid = -1;
    _sim_state = running;
    long long int start_time = _time;
    bool batch_complete;
    cout << "Sending batch " << batch_index + 1 << " (" << _batch_size << " packets)..." << endl;
    do
    {
      _Step();
      batch_complete = true;
      for (long long int source = 0; (source < _nodes) && batch_complete; ++source)
      {
        for (long long int c = 0; c < _classes; ++c)
        {
          if (_packet_seq_no[c][source] < _batch_size[c])
          {
            batch_complete = false;
            break;
          }
        }
      }
      if (_sent_packets_out)
      {
        *_sent_packets_out << _packet_seq_no << endl;
      }
    } while (!batch_complete);
    cout << "Batch injected. Time used is " << _time - start_time << " cycles." << endl;

    long long int sent_time = _time;
    cout << "Waiting for batch to complete..." << endl;

    long long int empty_steps = 0;

    bool requests_outstanding = false;
    for (long long int c = 0; c < _classes; ++c)
    {
      for (long long int n = 0; n < _nodes; ++n)
      {
        requests_outstanding |= (_requests_outstanding[c][n] > 0);
      }
    }

    while (requests_outstanding)
    {
      _Step();

      ++empty_steps;

      if (empty_steps % 1000 == 0)
      {
        _DisplayRemaining();
        cout << ".";
      }

      requests_outstanding = false;
      for (long long int c = 0; c < _classes; ++c)
      {
        for (long long int n = 0; n < _nodes; ++n)
        {
          requests_outstanding |= (_requests_outstanding[c][n] > 0);
        }
      }
    }
    cout << endl;
    cout << "Batch received. Time used is " << _time - sent_time << " cycles." << endl
         << "Last packet was " << _last_pid << ", last flit was " << _last_id << "." << endl;

    _batch_time->AddSample(_time - start_time);

    cout << _sim_state << endl;

    UpdateStats();
    DisplayStats();

    ++batch_index;
  }
  _sim_state = draining;
  _drain_time = _time;
  return 1;
}

void BatchTrafficManager::_UpdateOverallStats()
{
  SyntheticTrafficManager::_UpdateOverallStats();
  _overall_min_batch_time += _batch_time->Min();
  _overall_avg_batch_time += _batch_time->Average();
  _overall_max_batch_time += _batch_time->Max();
}

string BatchTrafficManager::_OverallStatsHeaderCSV() const
{
  ostringstream os;
  os << SyntheticTrafficManager::_OverallStatsHeaderCSV() << ','
     << "min_batch_time" << ','
     << "avg_batch_time" << ','
     << "max_batch_time";
  return os.str();
}

string BatchTrafficManager::_OverallClassStatsCSV(long long int c) const
{
  ostringstream os;
  os << SyntheticTrafficManager::_OverallClassStatsCSV(c) << ','
     << _overall_min_batch_time / (double)_total_sims << ','
     << _overall_avg_batch_time / (double)_total_sims << ','
     << _overall_max_batch_time / (double)_total_sims;
  return os.str();
}

void BatchTrafficManager::_DisplayClassStats(long long int c, ostream &os) const
{
  TrafficManager::_DisplayClassStats(c, os);
  os << "Minimum batch duration = " << _batch_time->Min() << endl
     << "Average batch duration = " << _batch_time->Average() << endl
     << "Maximum batch duration = " << _batch_time->Max() << endl;
}

void BatchTrafficManager::_WriteClassStats(long long int c, ostream &os) const
{
  SyntheticTrafficManager::_WriteClassStats(c, os);
  os << "batch_time(" << c + 1 << ") = " << _batch_time->Average() << ";" << endl;
}

void BatchTrafficManager::_DisplayOverallClassStats(long long int c, ostream &os) const
{
  SyntheticTrafficManager::_DisplayOverallClassStats(c, os);
  os << "Overall min batch duration = " << _overall_min_batch_time / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl
     << "Overall min batch duration = " << _overall_avg_batch_time / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl
     << "Overall min batch duration = " << _overall_max_batch_time / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;
}
