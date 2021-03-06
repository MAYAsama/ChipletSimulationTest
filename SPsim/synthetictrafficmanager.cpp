// $Id$

#include <sstream>

#include "synthetictrafficmanager.hpp"
#include "random_utils.hpp"

SyntheticTrafficManager::SyntheticTrafficManager(const Configuration config[], const vector<Network *> &net)
    : TrafficManager(config, net)
{

  // ============ Traffic ============

  _traffic = config[0].GetStrArray("traffic");
  _traffic.resize(_classes, _traffic.back());

  _traffic_pattern.resize(_classes);
  for (long long int c = 0; c < _classes; ++c)
  {
    _traffic_pattern[c] = TrafficPattern::New(_traffic[c], _nodes, &config[0]);
  }

  string packet_size_str = config[0].GetStr("packet_size");
  if (packet_size_str.empty())
  {
    _packet_size.push_back(vector<long long int>(1, config[0].GetLongInt("packet_size")));
  }
  else
  {
    vector<string> packet_size_strings = tokenize_str(packet_size_str);
    for (size_t i = 0; i < packet_size_strings.size(); ++i)
    {
      _packet_size.push_back(tokenize_int(packet_size_strings[i]));
    }
  }
  _packet_size.resize(_classes, _packet_size.back());

  string packet_size_rate_str = config[0].GetStr("packet_size_rate");
  if (packet_size_rate_str.empty())
  {
    long long int rate = config[0].GetLongInt("packet_size_rate");
    assert(rate >= 0);
    for (long long int c = 0; c < _classes; ++c)
    {
      long long int size = _packet_size[c].size();
      _packet_size_rate.push_back(vector<long long int>(size, rate));
      _packet_size_max_val.push_back(size * rate - 1);
    }
  }
  else
  {
    vector<string> packet_size_rate_strings = tokenize_str(packet_size_rate_str);
    packet_size_rate_strings.resize(_classes, packet_size_rate_strings.back());
    for (long long int c = 0; c < _classes; ++c)
    {
      vector<long long int> rates = tokenize_int(packet_size_rate_strings[c]);
      rates.resize(_packet_size[c].size(), rates.back());
      _packet_size_rate.push_back(rates);
      long long int size = rates.size();
      long long int max_val = -1;
      for (long long int i = 0; i < size; ++i)
      {
        long long int rate = rates[i];
        assert(rate >= 0);
        max_val += rate;
      }
      _packet_size_max_val.push_back(max_val);
    }
  }

  _reply_class = config[0].GetIntArray("reply_class");
  if (_reply_class.empty())
  {
    _reply_class.push_back(config[0].GetLongInt("reply_class"));
  }
  _reply_class.resize(_classes, _reply_class.back());

  _request_class.resize(_classes, -1);
  for (long long int c = 0; c < _classes; ++c)
  {
    long long int const reply_class = _reply_class[c];
    if (reply_class >= 0)
    {
      assert(_request_class[reply_class] < 0);
      _request_class[reply_class] = c;
    }
  }

  // ============ Injection queues ============

  _qtime.resize(_classes);
  _qdrained.resize(_classes);

  for (long long int c = 0; c < _classes; ++c)
  {
    _qtime[c].resize(_nodes);
    _qdrained[c].resize(_nodes);
  }
}

SyntheticTrafficManager::~SyntheticTrafficManager()
{
  for (long long int c = 0; c < _classes; ++c)
  {
    delete _traffic_pattern[c];
  }
}

void SyntheticTrafficManager::_RetirePacket(Flit *head, Flit *tail)
{
  assert(head);
  assert(tail);
  assert(head->pid == tail->pid);
  assert(head->cl == tail->cl);
  long long int const reply_class = _reply_class[head->cl];
  assert(reply_class < _classes);

  if (reply_class < 0)
  {
    long long int const request_class = _request_class[head->cl];
    assert(request_class < _classes);
    if (request_class < 0)
    {
      // single-packet transactions "magically" notify source of completion
      // when packet arrives at destination
      _requests_outstanding[head->cl][head->src]--;
    }
    else
    {
      // request-reply transactions complete when reply arrives
      _requests_outstanding[request_class][head->dest]--;
    }
  }
  else
  {
    _packet_seq_no[head->cl][head->dest]++;
    long long int size = _GetNextPacketSize(reply_class);
    _GeneratePacket(head->dest, head->src, size, reply_class, tail->atime + 1);
  }
}

void SyntheticTrafficManager::_Inject()
{

  for (long long int c = 0; c < _classes; ++c)
  {
    for (long long int source = 0; source < _nodes; ++source)
    //for (long long int source = 65; source < 67; ++source)
    {
      // Potentially generate packets for any (source,class)
      // that is currently empty
      if (_partial_packets[c][source].empty())
      {
        if (_request_class[c] >= 0)
        {
          _qtime[c][source] = _time;
        }
        else
        {
          while (_qtime[c][source] <= _time)
          {
            ++_qtime[c][source];
            if (_IssuePacket(source, c) >= 0)
            //if ((source==0||source==1)&&_IssuePacket(source, c) >= 0)
            { //generate a packet
              _requests_outstanding[c][source]++;
              _packet_seq_no[c][source]++;
              break;
            }
          }
        }
        if ((_sim_state == draining) && (_qtime[c][source] > _drain_time))
        {
          _qdrained[c][source] = true;
        }
      }
    }
  }
}

bool SyntheticTrafficManager::_PacketsOutstanding() const
{
  if (TrafficManager::_PacketsOutstanding())
  {
    return true;
  }
  for (long long int c = 0; c < _classes; ++c)
  {
    if (_measure_stats[c])
    {
      assert(_measured_in_flight_flits[c].empty());
      for (long long int s = 0; s < _nodes; ++s)
      {
        if (!_qdrained[c][s])
        {
          return true;
        }
      }
    }
  }
  return false;
}

void SyntheticTrafficManager::_ResetSim()
{
  TrafficManager::_ResetSim();

  //reset queuetime for all sources and initialize traffic patterns
  for (long long int c = 0; c < _classes; ++c)
  {
    _qtime[c].assign(_nodes, 0);
    _qdrained[c].assign(_nodes, false);
    _traffic_pattern[c]->reset();
  }
}

string SyntheticTrafficManager::_OverallStatsHeaderCSV() const
{
  ostringstream os;
  os << "traffic"
     << ',' << TrafficManager::_OverallStatsHeaderCSV();
  return os.str();
}

string SyntheticTrafficManager::_OverallClassStatsCSV(long long int c) const
{
  ostringstream os;
  os << _traffic[c] << ','
     << TrafficManager::_OverallClassStatsCSV(c);
  return os.str();
}

long long int SyntheticTrafficManager::_GetNextPacketSize(long long int cl) const
{
  assert(cl >= 0 && cl < _classes);

  vector<long long int> const &psize = _packet_size[cl];
  long long int sizes = psize.size();

  if (sizes == 1)
  {
    return psize[0];
  }

  vector<long long int> const &prate = _packet_size_rate[cl];
  long long int max_val = _packet_size_max_val[cl];

  long long int pct = RandomInt(max_val);

  for (long long int i = 0; i < (sizes - 1); ++i)
  {
    long long int const limit = prate[i];
    if (limit > pct)
    {
      return psize[i];
    }
    else
    {
      pct -= limit;
    }
  }
  assert(prate.back() > pct);
  return psize.back();
}

double SyntheticTrafficManager::_GetAveragePacketSize(long long int cl) const
{
  vector<long long int> const &psize = _packet_size[cl];
  long long int sizes = psize.size();
  if (sizes == 1)
  {
    return (double)psize[0];
  }
  vector<long long int> const &prate = _packet_size_rate[cl];
  long long int sum = 0;
  for (long long int i = 0; i < sizes; ++i)
  {
    sum += psize[i] * prate[i];
  }
  return (double)sum / (double)(_packet_size_max_val[cl] + 1);
}
