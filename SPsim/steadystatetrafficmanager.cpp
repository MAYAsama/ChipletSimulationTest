// $Id$

#include <cmath>
#include <sstream>
//
#include "random_utils.hpp"
#include <iostream>
#include <algorithm>

#include "steadystatetrafficmanager.hpp"

SteadyStateTrafficManager::SteadyStateTrafficManager(const Configuration config[], const vector<Network *> &net)
    : SyntheticTrafficManager(config, net)
{
  _load = config[0].GetFloatArray("injection_rate");
  if (_load.empty())
  {
    _load.push_back(config[0].GetFloat("injection_rate"));
  }
  _load.resize(_classes, _load.back());

  if (config[0].GetLongInt("injection_rate_uses_flits"))
  {
    for (long long int c = 0; c < _classes; ++c)
    {
      _load[c] /= _GetAveragePacketSize(c);
    }
  }

  _injection = config[0].GetStrArray("injection_process");
  _injection.resize(_classes, _injection.back());

  _injection_process.resize(_classes);
  for (long long int c = 0; c < _classes; ++c)
  {
    _injection_process[c] = InjectionProcess::New(_injection[c], _nodes, _load[c], &config[0]);
  }

  _measure_latency = (config[0].GetStr("sim_type") == "latency");

  _sample_period = config[0].GetLongInt("sample_period");
  _max_samples = config[0].GetLongInt("max_samples");
  _warmup_periods = config[0].GetLongInt("warmup_periods");

  _latency_thres = config[0].GetFloatArray("latency_thres");
  if (_latency_thres.empty())
  {
    _latency_thres.push_back(config[0].GetFloat("latency_thres"));
  }
  _latency_thres.resize(_classes, _latency_thres.back());

  _warmup_threshold = config[0].GetFloatArray("warmup_thres");
  if (_warmup_threshold.empty())
  {
    _warmup_threshold.push_back(config[0].GetFloat("warmup_thres"));
  }
  _warmup_threshold.resize(_classes, _warmup_threshold.back());

  _acc_warmup_threshold = config[0].GetFloatArray("acc_warmup_thres");
  if (_acc_warmup_threshold.empty())
  {
    _acc_warmup_threshold.push_back(config[0].GetFloat("acc_warmup_thres"));
  }
  _acc_warmup_threshold.resize(_classes, _acc_warmup_threshold.back());

  _stopping_threshold = config[0].GetFloatArray("stopping_thres");
  if (_stopping_threshold.empty())
  {
    _stopping_threshold.push_back(config[0].GetFloat("stopping_thres"));
  }
  _stopping_threshold.resize(_classes, _stopping_threshold.back());

  _acc_stopping_threshold = config[0].GetFloatArray("acc_stopping_thres");
  if (_acc_stopping_threshold.empty())
  {
    _acc_stopping_threshold.push_back(config[0].GetFloat("acc_stopping_thres"));
  }
  _acc_stopping_threshold.resize(_classes, _acc_stopping_threshold.back());
}

SteadyStateTrafficManager::~SteadyStateTrafficManager()
{
  for (long long int c = 0; c < _classes; ++c)
  {
    delete _injection_process[c];
  }
}

long long int SteadyStateTrafficManager::_IssuePacket(long long int source, long long int cl)
{
  if(_net.size()==1){
  if (_injection_process[cl]->test(source))
  {
    long long int dest = _traffic_pattern[cl]->dest(source);
    //long long int dest = source==0?10:2;
    long long int size = _GetNextPacketSize(cl);
    long long int time = ((_include_queuing == 1) ? _qtime[cl][source] : _time);
    return _GeneratePacket(source, dest, size, cl, time);
  }
  return -1;
  }
  
  else
  {
      //vector<int>::iterator it = find(leftnode.begin(), leftnode.end(), source-_net[0]->NumNodes());
      //auto index = std::distance(std::begin(leftnode), it);
      if(source-_net[0]->NumNodes()>=0&& RandomFloat() < _load[0])
      {
      	long long int tsource = leftnode[source-_net[0]->NumNodes()]+_net[0]->NumNodes();
        long long int dest = leftnode[RandomInt(_nodes-_net[0]->NumNodes() - 1)]+_net[0]->NumNodes();//
        //long long int dest = source==81?65:73;
        //long long int dest = source==65?11:3;
        long long int size = _GetNextPacketSize(cl);
        long long int time = ((_include_queuing == 1) ? _qtime[cl][source] : _time);
        return _GeneratePacket(tsource, dest, size, cl, time);
      }
      return -1;
  }
}

void SteadyStateTrafficManager::_ResetSim()
{
  SyntheticTrafficManager::_ResetSim();

  for (long long int c = 0; c < _classes; ++c)
  {
    _injection_process[c]->reset();
  }
}

bool SteadyStateTrafficManager::_SingleSim()
{
  // warm-up
  // all packets after warmup_time are marked
  // converge
  // drain, wait until all packets finish

  if (_warmup_periods > 0)
  {
    _sim_state = warming_up;
  }
  else
  {
    _sim_state = running;
  }

  long long int converged = 0;

  //once warmed up, we require 3 converging runs to end the simulation
  vector<double> prev_latency(_classes, 0.0);
  vector<double> prev_accepted(_classes, 0.0);
  bool clear_last = false;
  long long int total_phases = 0;
  while ((total_phases < _max_samples) &&
         ((_sim_state != running) ||
          (converged < 3)))
  {

    if (clear_last || (((_sim_state == warming_up) && ((total_phases % 2) == 0))))
    {
      clear_last = false;
      _ClearStats();
    }

    for (long long int iter = 0; iter < _sample_period; ++iter)
    {
      if ((_time % 1000000) == 0)
      {
        cout << "\nTick: " << _time / (long long int)1000000 << "M" << endl;
      }
      _Step();
    }

    UpdateStats();
    DisplayStats();

    long long int lat_exc_class = -1;
    long long int lat_chg_exc_class = -1;
    long long int acc_chg_exc_class = -1;

    for (long long int c = 0; c < _classes; ++c)
    {

      if (_measure_stats[c] == 0)
      {
        continue;
      }

      double cur_latency = _plat_stats[c]->Average();

      long long int total_accepted_count;
      _ComputeStats(_accepted_flits[c], &total_accepted_count);
      double total_accepted_rate = (double)total_accepted_count / (double)(_time - _reset_time);
      double cur_accepted = total_accepted_rate / (double)_nodes;

      double latency_change = fabs((cur_latency - prev_latency[c]) / cur_latency);
      prev_latency[c] = cur_latency;

      double accepted_change = fabs((cur_accepted - prev_accepted[c]) / cur_accepted);
      prev_accepted[c] = cur_accepted;

      double latency = (double)_plat_stats[c]->Sum();
      double count = (double)_plat_stats[c]->NumSamples();

      map<long long int, Flit *>::const_iterator iter;
      for (iter = _total_in_flight_flits[c].begin();
           iter != _total_in_flight_flits[c].end();
           iter++)
      {
        latency += (double)(_time - iter->second->ctime);
        count++;
      }

      if ((lat_exc_class < 0) &&
          (_latency_thres[c] >= 0.0) &&
          ((latency / count) > _latency_thres[c]))
      {
        lat_exc_class = c;
      }

      cout << "class " << c << " latency change    = " << latency_change << endl;
      if (lat_chg_exc_class < 0)
      {
        if ((_sim_state == warming_up) &&
            (_warmup_threshold[c] >= 0.0) &&
            (latency_change > _warmup_threshold[c]))
        {
          lat_chg_exc_class = c;
        }
        else if ((_sim_state == running) &&
                 (_stopping_threshold[c] >= 0.0) &&
                 (latency_change > _stopping_threshold[c]))
        {
          lat_chg_exc_class = c;
        }
      }

      cout << "class " << c << " throughput change = " << accepted_change << endl;
      if (acc_chg_exc_class < 0)
      {
        if ((_sim_state == warming_up) &&
            (_acc_warmup_threshold[c] >= 0.0) &&
            (accepted_change > _acc_warmup_threshold[c]))
        {
          acc_chg_exc_class = c;
        }
        else if ((_sim_state == running) &&
                 (_acc_stopping_threshold[c] >= 0.0) &&
                 (accepted_change > _acc_stopping_threshold[c]))
        {
          acc_chg_exc_class = c;
        }
      }
    }

    // Fail safe for latency mode, throughput will ust continue
    if (_measure_latency && (lat_exc_class >= 0))
    {

      cout << "Average latency for class " << lat_exc_class << " exceeded " << _latency_thres[lat_exc_class] << " cycles. Aborting simulation." << endl;
      converged = 0;
      _sim_state = draining;
      _drain_time = _time;
      break;
    }

    if (_sim_state == warming_up)
    {
      if ((_warmup_periods > 0) ? (total_phases + 1 >= _warmup_periods) : ((!_measure_latency || (lat_chg_exc_class < 0)) && (acc_chg_exc_class < 0)))
      {
        cout << "Warmed up ..."
             << "Time used is " << _time << " cycles" << endl;

        clear_last = true;
        _sim_state = running;
      }
    }
    else if (_sim_state == running)
    {
      if ((!_measure_latency || (lat_chg_exc_class < 0)) &&
          (acc_chg_exc_class < 0))
      {
        //++converged;
      }
      else
      {
        converged = 0;
      }
    }
    ++total_phases;
  }

  if (_sim_state == running)
  {
    ++converged;

    _sim_state = draining;
    _drain_time = _time;

    if (_measure_latency)
    {
      cout << "Draining all recorded packets ..." << endl;
      long long int empty_steps = 0;
      while (_PacketsOutstanding())
      {
        _Step();

        ++empty_steps;

        if (empty_steps % 1000 == 0)
        {

          long long int lat_exc_class = -1;

          for (long long int c = 0; c < _classes; c++)
          {

            double threshold = _latency_thres[c];

            if (threshold < 0.0)
            {
              continue;
            }

            double acc_latency = _plat_stats[c]->Sum();
            double acc_count = (double)_plat_stats[c]->NumSamples();

            map<long long int, Flit *>::const_iterator iter;
            for (iter = _total_in_flight_flits[c].begin();
                 iter != _total_in_flight_flits[c].end();
                 iter++)
            {
              acc_latency += (double)(_time - iter->second->ctime);
              acc_count++;
            }

            if ((acc_latency / acc_count) > threshold)
            {
              lat_exc_class = c;
              break;
            }
          }

          if (lat_exc_class >= 0)
          {
            cout << "Average latency for class " << lat_exc_class << " exceeded " << _latency_thres[lat_exc_class] << " cycles. Aborting simulation." << endl;
            converged = 0;
            _sim_state = warming_up;
            break;
          }

          _DisplayRemaining();
        }
      }
    }
  }
  else
  {
    cout << "Too many sample periods needed to converge" << endl;
  }

  return (converged > 0);
}

string SteadyStateTrafficManager::_OverallStatsHeaderCSV() const
{
  ostringstream os;
  os << "load"
     << ',' << SyntheticTrafficManager::_OverallStatsHeaderCSV();
  return os.str();
}

string SteadyStateTrafficManager::_OverallClassStatsCSV(long long int c) const
{
  ostringstream os;
  os << _load[c]
     << ',' << SyntheticTrafficManager::_OverallClassStatsCSV(c);
  return os.str();
}
