// $Id$

#include <sstream>

#include "workloadtrafficmanager.hpp"

WorkloadTrafficManager::WorkloadTrafficManager(const Configuration config[], const vector<Network *> &net)
    : TrafficManager(config, net), _overall_runtime(0)
{
  _sample_period = config[0].GetLongInt("sample_period");
  _max_samples = config[0].GetLongInt("max_samples");
  _warmup_periods = config[0].GetLongInt("warmup_periods");

  vector<string> workload = config[0].GetStrArray("workload");
  workload.resize(_classes, workload.back());

  _workload.resize(_classes);
  /*Sneha*/
  for (long long int i = 0; i < 64; i++)
  {
    source_map_1[i] = 0;
    dest_map_1[i] = 0;
    source_map_5[i] = 0;
    dest_map_5[i] = 0;
  }
  /*Sneha*/
  for (long long int c = 0; c < _classes; ++c)
  {
    Workload *wl = Workload::New(workload[c], _net[0]->NumNodes(), &config[0]);
    assert(wl);
    _workload[c] = wl;
  }
}

WorkloadTrafficManager::~WorkloadTrafficManager()
{
  for (long long int c = 0; c < _classes; ++c)
  {
    delete _workload[c];
  }
}

void WorkloadTrafficManager::_Inject()
{
  for (long long int c = 0; c < _classes; ++c)
  {
    Workload *const wl = _workload[c];
    while (!wl->empty())
    {
      long long int const source = wl->source();
      if ((_net.size()==1&&(_partial_packets[c][source].empty()))|| (_net.size()>1&&(_partial_packets[c][source+_net[0]->NumNodes()].empty())))
      {
        ++_requests_outstanding[c][source];
        ++_packet_seq_no[c][source];
        long long int const dest = wl->dest();
        long long int const size = wl->size();
        long long int const time = (_include_queuing == 1) ? wl->time() : _time;
        long long int  pid;
        if(_net.size()==1)
        {
          pid = _GeneratePacket(source, dest, size, c, time);
        
        /*Sneha*/
        if (wl->size() == 1)
        {
          source_map_1[source] += 1;
          dest_map_1[dest] += 1;
        }
        else if (wl->size() == 9)
        {
          source_map_5[source] += 1;
          dest_map_5[dest] += 1;
        }
        /*Sneha*/
        }
        else
        {
         pid = _GeneratePacket(source+_net[0]->NumNodes(), dest+_net[0]->NumNodes(), size, c, time);
        if (wl->size() == 1)
        {
          source_map_1[source] += 1;
          dest_map_1[dest] += 1;
        }
        else if (wl->size() == 9)
        {
          source_map_5[source] += 1;
          dest_map_5[dest] += 1;
        }
        }
        wl->inject(pid);
      }
      else
      {
        wl->defer();
      }
    }
    wl->advanceTime();
  }
}

void WorkloadTrafficManager::_RetirePacket(Flit *head, Flit *tail)
{
  TrafficManager::_RetirePacket(head, tail);
  _workload[head->cl]->retire(head->pid);
}

void WorkloadTrafficManager::_ResetSim()
{
  TrafficManager::_ResetSim();

  for (long long int c = 0; c < _classes; ++c)
  {
    _workload[c]->reset();
  }
}

bool WorkloadTrafficManager::_SingleSim()
{
  _sim_state = warming_up;

  if (_warmup_periods > 0)
  {

    cout << "Warming up..." << endl;

    while (_time < _warmup_periods * _sample_period)
    {

      if ((_time % 1000000) == 0)
      {
        cout << "\nTick: " << _time / (long long int)1000000 << "M" << endl;
      }

      _Step();

      if ((_time % _sample_period) == 0)
      {
        UpdateStats();
        DisplayStats();
      }
    }

    _ClearStats();

    cout << "Warmup ends after " << _warmup_periods * _sample_period
         << " cycles." << endl;
  }

  _sim_state = running;

  cout << "Beginning measurements..." << endl;

  while (!_Completed() &&
         ((_max_samples < 0) ||
          (_time < (_warmup_periods + _max_samples) * _sample_period)))
  {
    if ((_time % 1000000) == 0)
    {
      cout << "\nTick: " << _time / (long long int)1000000 << "M" << endl;
    }

    _Step();

    if ((_time % _sample_period) == 0)
    {
      UpdateStats();
      DisplayStats();
    }
  }

  cout << "Completed measurements after " << _time << " cycles." << endl;

  _sim_state = draining;
  _drain_time = _time;

  return 1;
}

bool WorkloadTrafficManager::_Completed()
{
  for (long long int c = 0; c < _classes; ++c)
  {
    if (_measure_stats[c] &&
        (!_workload[c]->completed() || !_measured_in_flight_flits[c].empty()))
    {
      return false;
    }
  }
  return true;
}

void WorkloadTrafficManager::_UpdateOverallStats()
{
  TrafficManager::_UpdateOverallStats();
  _overall_runtime += (_drain_time - _reset_time);
}

string WorkloadTrafficManager::_OverallStatsHeaderCSV() const
{
  ostringstream os;
  os << TrafficManager::_OverallStatsHeaderCSV()
     << ',' << "runtime";
  return os.str();
}

string WorkloadTrafficManager::_OverallClassStatsCSV(long long int c) const
{
  ostringstream os;
  os << TrafficManager::_OverallClassStatsCSV(c)
     << ',' << (double)_overall_runtime / (double)_total_sims;
  return os.str();
}

void WorkloadTrafficManager::_DisplayClassStats(long long int c, ostream &os) const
{
  _workload[c]->printStats(os);
  TrafficManager::_DisplayClassStats(c, os);
}

void WorkloadTrafficManager::_DisplayOverallClassStats(long long int c, ostream &os) const
{
  TrafficManager::_DisplayOverallClassStats(c, os);
  os << "Overall workload runtime = " << (double)_overall_runtime / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;
  /*Sneha*/
  cout << "\nThe distribution of the source packets of size 1 is: \n";
  for (long long int i = 0; i < 64; i++)
  {
    cout << "\nSource [" << i << "] = " << source_map_1[i];
  }
  cout << "\nThe distribution of the source packets of size 9 is: \n";
  for (long long int i = 0; i < 64; i++)
  {
    cout << "\nSource [" << i << "] = " << source_map_5[i];
  }
  cout << "\nThe distribution of the dest packets of size 1 is: \n";
  for (long long int i = 0; i < 64; i++)
  {
    cout << "\ndest [" << i << "] = " << dest_map_1[i];
  }
  cout << "\nThe distribution of the dest packets of size 9 is: \n";
  for (long long int i = 0; i < 64; i++)
  {
    cout << "\ndest [" << i << "] = " << dest_map_5[i];
  }
  /*Sneha*/
}
