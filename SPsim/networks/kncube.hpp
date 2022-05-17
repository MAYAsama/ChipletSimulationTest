// $Id$
#ifndef _KNCUBE_HPP_
#define _KNCUBE_HPP_

#include "network.hpp"

class KNCube : public Network
{

  bool _mesh;

  long long int _k;
  long long int _n;

  void _ComputeSize(const Configuration &config);
  void _BuildNet(const Configuration &config);
  void _Set25channel(const Configuration &config,vector<FlitChannel*> _25chan, vector<CreditChannel*> _25chan_cred, vector<long long int> leftnode, vector<long long int> rightnode,long long int *stcount);
  long long int _LeftChannel(long long int node, long long int dim);
  long long int _RightChannel(long long int node, long long int dim);

  long long int _LeftNode(long long int node, long long int dim);
  long long int _RightNode(long long int node, long long int dim);

public:
//bond node set
//  int _bondnode[4][4]={{1,2,25,26},{5,6,29,30},{33,34,57,58},{37,38,61,62}};
//  unordered_map<int, int> mp{{0,0},{1,0},{4,0},{5,0},{2,1},{3,1},{6,1},{7,1},{8,2},{9,2},{12,2},{13,2},{10,3},{11,3},{14,3},{15,3}};
  
//
  KNCube( const Configuration &config, const string & name, bool mesh,long long int configsize ,long long int id);
  static void RegisterRoutingFunctions();

  long long int GetN() const;
  long long int GetK() const;

  double Capacity() const;

  void InsertRandomFaults(const Configuration &config);
};

#endif
