// $Id$

/*main.cpp
 *
 *The starting point of the network simulator
 *-Include all network header files
 *-initilize the network
 *-initialize the traffic manager and set it to run
 *
 *
 */
#include <sys/time.h>

#include <string>
#include <cstdlib>
#include <iostream>
#include <fstream>

#include <sstream>
#include "booksim.hpp"
#include "routefunc.hpp"
#include "traffic.hpp"
#include "booksim_config.hpp"
#include "trafficmanager.hpp"
#include "random_utils.hpp"
#include "network.hpp"
#include "injection.hpp"
#include "power_module.hpp"
//
#include "misc_utils.hpp"
//
#include "asyncConfig.hpp"

///////////////////////////////////////////////////////////////////////////////
//Global declarations
//////////////////////

//AsyncConfig *asyncConfig = NULL;
vector<AsyncConfig*> asyncConfig;

/* the current traffic manager instance */
TrafficManager *trafficManager = NULL;

long long int GetSimTime()
{
  return trafficManager->getTime();
}

class Stats;
Stats *GetStats(const std::string &name)
{
  Stats *test = trafficManager->getStats(name);
  if (test == 0)
  {
    cout << "warning statistics " << name << " not found" << endl;
  }
  return test;
}

/* printing activity factor*/
bool gPrintActivity;

long long int gK; //radix
long long int gN; //dimension
long long int gC; //concentration

long long int gNodes;
//gyj
vector<long long int> leftnode;
vector<long long int> rightnode;
//generate nocviewer trace
bool gTrace;

ostream *gWatchOut;

// Orion Power Support
int g_number_of_injected_flits = 0;
int g_number_of_retired_flits = 0;
int g_total_cs_register_writes = 0;

/////////////////////////////////////////////////////////////////////////////

void nodepair( BookSimConfig const  config[],long long int configsize)   //for mesh
{
    long long int size = configsize;
    long long int k0 = config[0].GetLongInt("k");
    long long int n0 = config[0].GetLongInt("n");
    for (long long int i = 1; i < size; i++)
    {
        long long int st = config[i].GetLongInt("start_point");
        long long int k = config[i].GetLongInt("k");
        long long int n = config[i].GetLongInt("n");
        for (long long int j = 0; j < k; j++)
        {
            for (long long int l = 0; l < k; l++)
            {
                leftnode.push_back(st+j*k0+l);
                rightnode.push_back(j * k + l);
            }
        }
    }
}

bool Simulate(BookSimConfig const  config[],long long int configsize)
{
  vector<Network *> net;                          //gyj 
  deque<TimedModule*> _25timed_modules;
  vector<FlitChannel*> _25chan;
  vector<CreditChannel*> _25chan_cred;
  long long int _25channels = 0;
  for (long long int i = 1; i < configsize; i++)
  {
      _25channels += powi(config[i].GetLongInt("k"),config[i].GetLongInt("n"));    //for mesh
  }
  _25channels*=2;
  _25chan.resize(_25channels);
  _25chan_cred.resize(_25channels);
  for (long long int c = 0; c < _25channels; ++c) {
      ostringstream name;
      name <<"_25fchan_" << c;
      _25chan[c] = new FlitChannel(0, name.str(), 1);
      _25timed_modules.push_back(_25chan[c]);
      name.str("");
      name <<"_25cchan_" << c;
      _25chan_cred[c] = new CreditChannel(0, name.str());
      _25timed_modules.push_back(_25chan_cred[c]);
  }
  long long int token1 = 0;//
  long long int* stcount =&token1;//
  long long int subnets = config[0].GetLongInt("subnets");
  /*To include a new network, must register the network here
   *add an else if statement with the name of the network
   */
  net.resize(configsize);//
  nodepair(config,configsize);
  for (long long int i = 0; i < configsize; ++i)
  {
    ostringstream name;
    name << "network_" << i;
    net[i] = Network::New(config[i], name.str(),configsize,i);
    net[i]->Setchannel(config[i],_25chan, _25chan_cred,leftnode,rightnode,stcount);
    //    net[i]->DumpChannelMap();		//Sneha
    //    net[i]->DumpNodeMap();		//Sneha
  }
  
  gK=config[0].GetLongInt("k");
  gNodes=config[0].GetLongInt("k")*config[0].GetLongInt("k");
  
  /*tcc and characterize are legacy not sure how to use them */

  trafficManager = TrafficManager::New(config, net);
  trafficManager->Setdeque25(_25timed_modules);
  trafficManager->Setleftnode(leftnode);
  /*Start the simulation run */

  double total_time;                   /* Amount of time we've run */
  struct timeval start_time, end_time; /* Time before/after user code */
  total_time = 0.0;
  gettimeofday(&start_time, NULL);

  bool result = trafficManager->Run();
  //bool result=1;
  gettimeofday(&end_time, NULL);
  total_time = ((double)(end_time.tv_sec) + (double)(end_time.tv_usec) / 1000000.0) - ((double)(start_time.tv_sec) + (double)(start_time.tv_usec) / 1000000.0);

  cout << "\n*****************************************\n";
  cout << "Total run time " << total_time << endl;

  /*for (long long int i = 0; i < subnets; ++i)
  {
    ///Power analysis
    if (config[0].GetLongInt("sim_power") > 0)
    {
      Power_Module pnet(net[i], config[0]);
      pnet.run();
    }
    //delete net[i];
  }*/
  for(int i=0;i<configsize;i++)
  {
    delete net[i];
  }
  delete trafficManager;
  trafficManager = NULL;
  for(long long int c = 0; c < _25channels; ++c)
  {
    delete _25chan[c];
    delete _25chan_cred[c];
  }

  return result;
}

int main(int argc, char **argv)
{

//  BookSimConfig config;
  int configsize=argc-1;
  BookSimConfig config[configsize];
  asyncConfig.resize(configsize);
  BookSimConfig config0;
  for(int i=1;i<argc;i++)
  {
    if ( !ParseArgs(&config0, 2, argv[i] ) ) {
    cerr << "Usage: " << argv[i] << " configfile... [param=value...]" << endl;
    return 0;
    } 
    config[i-1]=config0;
    asyncConfig[i-1] = new AsyncConfig(config[i-1]);
  }
//  asyncConfig = new AsyncConfig(config);
  /*initialize routing, traffic, injection functions  */
  InitializeRoutingMap(config[0]);

  gPrintActivity = (config[0].GetLongInt("print_activity") > 0);
  gTrace = (config[0].GetLongInt("viewer_trace") > 0);

  string watch_out_file = config[0].GetStr("watch_out");
  if (watch_out_file == "")
  {
    gWatchOut = NULL;
  }
  else if (watch_out_file == "-")
  {
    gWatchOut = &cout;
  }
  else
  {
    gWatchOut = new ofstream(watch_out_file.c_str());
  }

  /*configure and run the simulator */
  bool result = Simulate(config,configsize);
//need to modify later
  
  if (asyncConfig[0]->doGating)
  {
    long long int totalViableIdleTicksSum = 0;
    long long int totalViableGatedTicksSum = 0;

    long long int totalViableIdleTimesSum = 0;
    long long int totalGatedTimesSum = 0;
    cout << "\n--------------Gating Results---------------------\n";
    //per router viable idle tick sum
    cout << "\nViable idle Ticks Sum, ";

    for (unsigned int i = 0; i < asyncConfig[0]->viableIdleTicksSum.size(); i++)
    {
      cout << asyncConfig[0]->viableIdleTicksSum[i] << ", ";
      totalViableIdleTicksSum = totalViableIdleTicksSum + asyncConfig[0]->viableIdleTicksSum[i];
    }

    //per router viable idle Times sum
    cout << "\nViable idle Times Sum, ";

    for (unsigned int i = 0; i < asyncConfig[0]->viableIdleTimesSum.size(); i++)
    {
      cout << asyncConfig[0]->viableIdleTimesSum[i] << ", ";
      totalViableIdleTimesSum = totalViableIdleTimesSum + asyncConfig[0]->viableIdleTimesSum[i];
    }

    //per router gated Ticks Sum
    cout << "\nViable gated Ticks Sum, ";

    for (unsigned int i = 0; i < asyncConfig[0]->viableGatedTicksSum.size(); i++)
    {
      cout << asyncConfig[0]->viableGatedTicksSum[i] << ", ";
      totalViableGatedTicksSum = totalViableGatedTicksSum + asyncConfig[0]->viableGatedTicksSum[i];
    }

    //per router gated Times Sum
    cout << "\nViable gated Times Sum, ";

    for (unsigned int i = 0; i < asyncConfig[0]->gatedTimesSum.size(); i++)
    {
      cout << asyncConfig[0]->gatedTimesSum[i] << ", ";
      totalGatedTimesSum = totalGatedTimesSum + asyncConfig[0]->gatedTimesSum[i];
    }

    //Overall Result

    cout << "\nOverall viable idle ticks, " << totalViableIdleTicksSum << endl;
    cout << "Overall viable idle times, " << totalViableIdleTimesSum << endl;
    cout << "Overall gated ticks, " << totalViableGatedTicksSum << endl;
    cout << "Overall gated times, " << totalGatedTimesSum << endl;
  }
for(long long int i=0;i<configsize;i++)
{
  delete asyncConfig[i];
  asyncConfig[i] = NULL;
}
  return result ? -1 : 0;
}
