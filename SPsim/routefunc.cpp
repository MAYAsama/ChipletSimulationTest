// $Id$

/*routefunc.cpp
 *
 *This is where most of the routing functions reside. Some of the topologies
 *has their own "register routing functions" which must be called to access
 *those routing functions. 
 *
 *After writing a routing function, don't forget to register it. The reg 
 *format is rfname_topologyname. 
 *
 */

#include <cstdlib>
#include <cassert>

#include "booksim.hpp"
#include "routefunc.hpp"
#include "kncube.hpp"
#include "random_utils.hpp"
#include "misc_utils.hpp"
#include "fattree.hpp"
#include "tree4.hpp"
#include "qtree.hpp"
#include "cmesh.hpp"

map<string, tRoutingFunction> gRoutingFunctionMap;

/* Global information used by routing functions */

long long int gNumVCs;
long long int gNumClasses;

vector<long long int> gBeginVCs;
vector<long long int> gEndVCs;

// ============================================================
//  QTree: Nearest Common Ancestor
// ===
void qtree_nca(const Router *r, const Flit *f, long long int in_channel, OutputSet *outputs, bool inject)
{
  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  long long int out_port;

  if (inject)
  {

    out_port = -1;
  }
  else
  {

    long long int height = QTree::HeightFromID(r->GetID());
    long long int pos = QTree::PosFromID(r->GetID());

    long long int dest = f->dest;

    for (long long int i = height + 1; i < gN; i++)
      dest /= gK;
    if (pos == dest / gK)
      // Route down to child
      out_port = dest % gK;
    else
      // Route up to parent
      out_port = gK;
  }

  outputs->Clear();

  outputs->AddRange(out_port, vcBegin, vcEnd);
}

// ============================================================
//  Tree4: Nearest Common Ancestor w/ Adaptive Routing Up
// ===
void tree4_anca(const Router *r, const Flit *f, long long int in_channel, OutputSet *outputs, bool inject)
{
  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  long long int range = 1;

  long long int out_port;

  if (inject)
  {

    out_port = -1;
  }
  else
  {

    long long int dest = f->dest;

    const long long int NPOS = 16;

    long long int rH = r->GetID() / NPOS;
    long long int rP = r->GetID() % NPOS;

    if (rH == 0)
    {
      dest /= 16;
      out_port = 2 * dest + RandomInt(1);
    }
    else if (rH == 1)
    {
      dest /= 4;
      if (dest / 4 == rP / 2)
        out_port = dest % 4;
      else
      {
        out_port = gK;
        range = gK;
      }
    }
    else
    {
      if (dest / 4 == rP)
        out_port = dest % 4;
      else
      {
        out_port = gK;
        range = 2;
      }
    }

    //  cout << "Router("<<rH<<","<<rP<<"): id= " << f->id << " dest= " << f->dest << " out_port = "
    //       << out_port << endl;
  }

  outputs->Clear();

  for (long long int i = 0; i < range; ++i)
    outputs->AddRange(out_port + i, gBeginVCs[f->cl], gEndVCs[f->cl]);
}

// ============================================================
//  Tree4: Nearest Common Ancestor w/ Random Routing Up
// ===
void tree4_nca(const Router *r, const Flit *f, long long int in_channel, OutputSet *outputs, bool inject)
{
  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  long long int out_port;

  if (inject)
  {

    out_port = -1;
  }
  else
  {

    long long int dest = f->dest;

    const long long int NPOS = 16;

    long long int rH = r->GetID() / NPOS;
    long long int rP = r->GetID() % NPOS;

    if (rH == 0)
    {
      dest /= 16;
      out_port = 2 * dest + RandomInt(1);
    }
    else if (rH == 1)
    {
      dest /= 4;
      if (dest / 4 == rP / 2)
        out_port = dest % 4;
      else
        out_port = gK + RandomInt(gK - 1);
    }
    else
    {
      if (dest / 4 == rP)
        out_port = dest % 4;
      else
        out_port = gK + RandomInt(1);
    }

    //  cout << "Router("<<rH<<","<<rP<<"): id= " << f->id << " dest= " << f->dest << " out_port = "
    //       << out_port << endl;
  }

  outputs->Clear();

  outputs->AddRange(out_port, vcBegin, vcEnd);
}

// ============================================================
//  FATTREE: Nearest Common Ancestor w/ Random  Routing Up
// ===
void fattree_nca(const Router *r, const Flit *f,
                 long long int in_channel, OutputSet *outputs, bool inject)
{
  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  long long int out_port;

  if (inject)
  {

    out_port = -1;
  }
  else
  {

    long long int dest = f->dest;
    long long int router_id = r->GetID(); //routers are numbered with smallest at the top level
    long long int routers_per_level = powi(gK, gN - 1);
    long long int pos = router_id % routers_per_level;
    long long int router_depth = router_id / routers_per_level; //which level
    long long int routers_per_neighborhood = powi(gK, gN - router_depth - 1);
    long long int router_neighborhood = pos / routers_per_neighborhood; //coverage of this tree
    long long int router_coverage = powi(gK, gN - router_depth);        //span of the tree from this router

    //NCA reached going down
    if (dest < (router_neighborhood + 1) * router_coverage &&
        dest >= router_neighborhood * router_coverage)
    {
      //down ports are numbered first

      //ejection
      if (router_depth == gN - 1)
      {
        out_port = dest % gK;
      }
      else
      {
        //find the down port for the destination
        long long int router_branch_coverage = powi(gK, gN - (router_depth + 1));
        out_port = (dest - router_neighborhood * router_coverage) / router_branch_coverage;
      }
    }
    else
    {
      //up ports are numbered last
      assert(in_channel < gK); //came from a up channel
      out_port = gK + RandomInt(gK - 1);
    }
  }
  outputs->Clear();

  outputs->AddRange(out_port, vcBegin, vcEnd);
}

// ============================================================
//  FATTREE: Nearest Common Ancestor w/ Adaptive Routing Up
// ===
void fattree_anca(const Router *r, const Flit *f,
                  long long int in_channel, OutputSet *outputs, bool inject)
{

  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  long long int out_port;

  if (inject)
  {

    out_port = -1;
  }
  else
  {

    long long int dest = f->dest;
    long long int router_id = r->GetID(); //routers are numbered with smallest at the top level
    long long int routers_per_level = powi(gK, gN - 1);
    long long int pos = router_id % routers_per_level;
    long long int router_depth = router_id / routers_per_level; //which level
    long long int routers_per_neighborhood = powi(gK, gN - router_depth - 1);
    long long int router_neighborhood = pos / routers_per_neighborhood; //coverage of this tree
    long long int router_coverage = powi(gK, gN - router_depth);        //span of the tree from this router

    //NCA reached going down
    if (dest < (router_neighborhood + 1) * router_coverage &&
        dest >= router_neighborhood * router_coverage)
    {
      //down ports are numbered first

      //ejection
      if (router_depth == gN - 1)
      {
        out_port = dest % gK;
      }
      else
      {
        //find the down port for the destination
        long long int router_branch_coverage = powi(gK, gN - (router_depth + 1));
        out_port = (dest - router_neighborhood * router_coverage) / router_branch_coverage;
      }
    }
    else
    {
      //up ports are numbered last
      assert(in_channel < gK); //came from a up channel
      out_port = gK;
      long long int random1 = RandomInt(gK - 1); // Chose two ports out of the possible at random, compare loads, choose one.
      long long int random2 = RandomInt(gK - 1);
      if (r->GetUsedCredit(out_port + random1) > r->GetUsedCredit(out_port + random2))
      {
        out_port = out_port + random2;
      }
      else
      {
        out_port = out_port + random1;
      }
    }
  }
  outputs->Clear();

  outputs->AddRange(out_port, vcBegin, vcEnd);
}

// ============================================================
//  Mesh - adatpive XY,YX Routing
//         pick xy or yx min routing adaptively at the source router
// ===

long long int dor_next_mesh(long long int cur, long long int dest, bool descending = false);

void adaptive_xy_yx_mesh(const Router *r, const Flit *f, long long int in_channel, OutputSet *outputs, bool inject)
{
  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  long long int out_port;

  if (inject)
  {

    out_port = -1;
  }
  else if (r->GetID() == f->dest)
  {

    // at destination router, we don't need to separate VCs by dim order
    out_port = 2 * gN;
  }
  else if(r->cnum==f->viaint)       //
  {

    //each class must have at least 2 vcs assigned or else xy_yx will deadlock
    long long int const available_vcs = (vcEnd - vcBegin + 1) / 2;
    assert(available_vcs > 0);

    long long int out_port_xy = dor_next_mesh(r->inode, f->idesnode, false);
    long long int out_port_yx = dor_next_mesh(r->inode, f->idesnode, true);

    // Route order (XY or YX) determined when packet is injected
    //  into the network, adaptively
    bool x_then_y;
    if (in_channel < 2 * gN||in_channel==5)
    {
      x_then_y = (f->vc < (vcBegin + available_vcs));
    }
    else
    {
      long long int credit_xy = r->GetUsedCredit(out_port_xy);
      long long int credit_yx = r->GetUsedCredit(out_port_yx);
      if (credit_xy > credit_yx)
      {
        x_then_y = false;
      }
      else if (credit_xy < credit_yx)
      {
        x_then_y = true;
      }
      else
      {
        x_then_y = (RandomInt(1) > 0);
      }
    }

    if (x_then_y)
    {
      out_port = out_port_xy;
      vcEnd -= available_vcs;
    }
    else
    {
      out_port = out_port_yx;
      vcBegin += available_vcs;
    }
  }
  else if(f->viaint!=r->cnum)
  {// don't care on chiplet
  	  if(r->inode!=f->inbondnode)
  	  {
  	    long long int const available_vcs = (vcEnd - vcBegin + 1) / 2;
    assert(available_vcs > 0);

    long long int out_port_xy = dor_next_mesh(r->inode,f->inbondnode, false);
    long long int out_port_yx = dor_next_mesh(r->inode,f->inbondnode, true);

    // Route order (XY or YX) determined when packet is injected
    //  into the network, adaptively
    bool x_then_y;
    if (in_channel < 2 * gN||in_channel==5)
    {
      x_then_y = (f->vc < (vcBegin + available_vcs));
    }
    else
    {
      long long int credit_xy = r->GetUsedCredit(out_port_xy);
      long long int credit_yx = r->GetUsedCredit(out_port_yx);
      if (credit_xy > credit_yx)
      {
        x_then_y = false;
      }
      else if (credit_xy < credit_yx)
      {
        x_then_y = true;
      }
      else
      {
        x_then_y = (RandomInt(1) > 0);
      }
    }

    if (x_then_y)
    {
      out_port = out_port_xy;
      vcEnd -= available_vcs;
    }
    else
    {
      out_port = out_port_yx;
      vcBegin += available_vcs;
    }
  }
  else
  out_port=5;
  }
  

  outputs->Clear();

  outputs->AddRange(out_port, vcBegin, vcEnd);
}

void xy_yx_mesh(const Router *r, const Flit *f, long long int in_channel, OutputSet *outputs, bool inject)
{
  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  long long int out_port;

  if (inject)
  {

    out_port = -1;
  }
  else if (r->GetID() == f->dest)
  {

    // at destination router, we don't need to separate VCs by dim order
    out_port = 2 * gN;
  }
  else
  {

    //each class must have at least 2 vcs assigned or else xy_yx will deadlock
    long long int const available_vcs = (vcEnd - vcBegin + 1) / 2;
    assert(available_vcs > 0);

    // Route order (XY or YX) determined when packet is injected
    //  into the network
    bool x_then_y = ((in_channel < 2 * gN) ? (f->vc < (vcBegin + available_vcs)) : (RandomInt(1) > 0));

    if (x_then_y)
    {
      out_port = dor_next_mesh(r->GetID(), f->dest, false);
      vcEnd -= available_vcs;
    }
    else
    {
      out_port = dor_next_mesh(r->GetID(), f->dest, true);
      vcBegin += available_vcs;
    }
  }

  outputs->Clear();

  outputs->AddRange(out_port, vcBegin, vcEnd);
}

//
// End Balfour-Schultz
//=============================================================

//=============================================================

long long int dor_next_mesh(long long int cur, long long int dest, bool descending)
{
  if (cur == dest)
  {
    return 2 * gN; // Eject
  }

  long long int dim_left;

  if (descending)
  {
    for (dim_left = (gN - 1); dim_left > 0; --dim_left)
    {
      if ((cur * gK / gNodes) != (dest * gK / gNodes))
      {
        break;
      }
      cur = (cur * gK) % gNodes;
      dest = (dest * gK) % gNodes;
    }
    cur = (cur * gK) / gNodes;
    dest = (dest * gK) / gNodes;
  }
  else
  {
    for (dim_left = 0; dim_left < (gN - 1); ++dim_left)
    {
      if ((cur % gK) != (dest % gK))
      {
        break;
      }
      cur /= gK;
      dest /= gK;
    }
    cur %= gK;
    dest %= gK;
  }

  if (cur < dest)
  {
    return 2 * dim_left; // Right
  }
  else
  {
    return 2 * dim_left + 1; // Left
  }
}

//=============================================================

void dor_next_torus(long long int cur, long long int dest, long long int in_port, long long int *out_port, long long int *partition, bool balance = false)
{
  long long int dim_left;
  long long int dir;
  long long int dist2;

  for (dim_left = 0; dim_left < gN; ++dim_left)
  {
    if ((cur % gK) != (dest % gK))
    {
      break;
    }
    cur /= gK;
    dest /= gK;
  }

  if (dim_left < gN)
  {

    if ((in_port / 2) != dim_left)
    {
      // Turning into a new dimension

      cur %= gK;
      dest %= gK;
      dist2 = gK - 2 * ((dest - cur + gK) % gK);

      if ((dist2 > 0) ||
          ((dist2 == 0) && (RandomInt(1))))
      {
        *out_port = 2 * dim_left; // Right
        dir = 0;
      }
      else
      {
        *out_port = 2 * dim_left + 1; // Left
        dir = 1;
      }

      if (partition)
      {
        if (balance)
        {
          // Cray's "Partition" allocation
          // Two datelines: one between k-1 and 0 which forces VC 1
          //                another between ((k-1)/2) and ((k-1)/2 + 1) which
          //                forces VC 0 otherwise any VC can be used

          if (((dir == 0) && (cur > dest)) ||
              ((dir == 1) && (cur < dest)))
          {
            *partition = 1;
          }
          else if (((dir == 0) && (cur <= (gK - 1) / 2) && (dest > (gK - 1) / 2)) ||
                   ((dir == 1) && (cur > (gK - 1) / 2) && (dest <= (gK - 1) / 2)))
          {
            *partition = 0;
          }
          else
          {
            *partition = RandomInt(1); // use either VC set
          }
        }
        else
        {
          // Deterministic, fixed dateline between nodes k-1 and 0

          if (((dir == 0) && (cur > dest)) ||
              ((dir == 1) && (dest < cur)))
          {
            *partition = 1;
          }
          else
          {
            *partition = 0;
          }
        }
      }
    }
    else
    {
      // Inverting the least significant bit keeps
      // the packet moving in the same direction
      *out_port = in_port ^ 0x1;
    }
  }
  else
  {
    *out_port = 2 * gN; // Eject
  }
}

//=============================================================

void dim_order_mesh(const Router *r, const Flit *f, long long int in_channel, OutputSet *outputs, bool inject)
{
  //long long int out_port = inject ? -1 : dor_next_mesh(r->GetID(), f->dest);

  int out_port=0;
  if(inject)
  {
    out_port=-1;
  }
  /*
  else if(f->viaint==1&&r->routercounts>0&&r->inode==f->idesnode)
  {
    out_port = 4;
  }
  else if(f->viaint==1&&r->routercounts>0&&r->inode!=f->idesnode)
  {
    out_port = 5;
  }
  else if(f->viaint==1&&r->routercounts==0&&f->idesnode!=r->inode)
  {

    out_port = dor_next_mesh(r->inode,f->idesnode);
  }
  else if(f->viaint==1&&r->routercounts==0&&f->idesnode==r->inode)
  {
    out_port = 5;
  }
  */
  else if(f->viaint==r->cnum)  //the same chip
  {
  	out_port=dor_next_mesh(r->inode,f->idesnode);
  }
  else if(f->viaint!=r->cnum)
  {
  	if(r->cnum!=0)
  	{
  	  //if(r->inode!=r->bondnode)
  	  //out_port=dor_next_mesh(r->inode,r->bondnode);
  	  //else
  	    out_port=5;
  	}
  	else
  	{
  	  //if(r->inode!=f->inbondnode)
  	  //  out_port=dor_next_mesh(r->inode,f->inbondnode);
  	  //else
  	  if(r->inode!=f->idesnode)
  	  out_port=dor_next_mesh(r->inode,f->idesnode);
  	  else
  	  out_port=5;
  	}	
  }

  
  else if(f->viaint==0)
  {
    out_port = inject ? -1 : dor_next_mesh( r->inode, f->idesnode );
  }
  else
  {
    cout<<"routing error"<<endl;
  }

  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  if (!inject && f->watch)
  {
    *gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
               << "Adding VC range ["
               << vcBegin << ","
               << vcEnd << "]"
               << " at output port " << out_port
               << " for flit " << f->id
               << " (input port " << in_channel
               << ", destination " << f->dest << ")"
               << "." << endl;
  }

  outputs->Clear();

  outputs->AddRange(out_port, vcBegin, vcEnd);
}

//=============================================================

void dim_order_ni_mesh(const Router *r, const Flit *f, long long int in_channel, OutputSet *outputs, bool inject)
{
  long long int out_port = inject ? -1 : dor_next_mesh(r->GetID(), f->dest);

  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  // at the destination router, we don't need to separate VCs by destination
  if (inject || (r->GetID() != f->dest))
  {

    long long int const vcs_per_dest = (vcEnd - vcBegin + 1) / gNodes;
    assert(vcs_per_dest > 0);

    vcBegin += f->dest * vcs_per_dest;
    vcEnd = vcBegin + vcs_per_dest - 1;
  }

  if (!inject && f->watch)
  {
    *gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
               << "Adding VC range ["
               << vcBegin << ","
               << vcEnd << "]"
               << " at output port " << out_port
               << " for flit " << f->id
               << " (input port " << in_channel
               << ", destination " << f->dest << ")"
               << "." << endl;
  }

  outputs->Clear();

  outputs->AddRange(out_port, vcBegin, vcEnd);
}

//=============================================================

void dim_order_pni_mesh(const Router *r, const Flit *f, long long int in_channel, OutputSet *outputs, bool inject)
{
  long long int out_port = inject ? -1 : dor_next_mesh(r->GetID(), f->dest);

  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  if (inject || (r->GetID() != f->dest))
  {
    long long int next_coord = f->dest;
    if (!inject)
    {
      long long int out_dim = out_port / 2;
      for (long long int d = 0; d < out_dim; ++d)
      {
        next_coord /= gK;
      }
    }
    next_coord %= gK;
    assert(next_coord >= 0 && next_coord < gK);
    long long int vcs_per_dest = (vcEnd - vcBegin + 1) / gK;
    assert(vcs_per_dest > 0);
    vcBegin += next_coord * vcs_per_dest;
    vcEnd = vcBegin + vcs_per_dest - 1;
  }

  if (!inject && f->watch)
  {
    *gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
               << "Adding VC range ["
               << vcBegin << ","
               << vcEnd << "]"
               << " at output port " << out_port
               << " for flit " << f->id
               << " (input port " << in_channel
               << ", destination " << f->dest << ")"
               << "." << endl;
  }

  outputs->Clear();

  outputs->AddRange(out_port, vcBegin, vcEnd);
}

//=============================================================

// Random intermediate in the minimal quadrant defined
// by the source and destination
long long int rand_min_intr_mesh(long long int src, long long int dest)
{
  long long int dist;

  long long int intm = 0;
  long long int offset = 1;

  for (long long int n = 0; n < gN; ++n)
  {
    dist = (dest % gK) - (src % gK);

    if (dist > 0)
    {
      intm += offset * ((src % gK) + RandomInt(dist));
    }
    else
    {
      intm += offset * ((dest % gK) + RandomInt(-dist));
    }

    offset *= gK;
    dest /= gK;
    src /= gK;
  }

  return intm;
}

//=============================================================

void romm_mesh(const Router *r, const Flit *f, long long int in_channel, OutputSet *outputs, bool inject)
{
  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  long long int out_port;

  if (inject)
  {

    out_port = -1;
  }
  else
  {

    if (in_channel == 2 * gN)
    {
      f->ph = 0; // Phase 0
      f->intm = rand_min_intr_mesh(f->src, f->dest);
    }

    if ((f->ph == 0) && (r->GetID() == f->intm))
    {
      f->ph = 1; // Go to phase 1
    }

    out_port = dor_next_mesh(r->GetID(), (f->ph == 0) ? f->intm : f->dest);

    // at the destination router, we don't need to separate VCs by phase
    if (r->GetID() != f->dest)
    {

      //each class must have at least 2 vcs assigned or else valiant valiant will deadlock
      long long int available_vcs = (vcEnd - vcBegin + 1) / 2;
      assert(available_vcs > 0);

      if (f->ph == 0)
      {
        vcEnd -= available_vcs;
      }
      else
      {
        assert(f->ph == 1);
        vcBegin += available_vcs;
      }
    }
  }

  outputs->Clear();

  outputs->AddRange(out_port, vcBegin, vcEnd);
}

//=============================================================

void romm_ni_mesh(const Router *r, const Flit *f, long long int in_channel, OutputSet *outputs, bool inject)
{
  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  // at the destination router, we don't need to separate VCs by destination
  if (inject || (r->GetID() != f->dest))
  {

    long long int const vcs_per_dest = (vcEnd - vcBegin + 1) / gNodes;
    assert(vcs_per_dest > 0);

    vcBegin += f->dest * vcs_per_dest;
    vcEnd = vcBegin + vcs_per_dest - 1;
  }

  long long int out_port;

  if (inject)
  {

    out_port = -1;
  }
  else
  {

    if (in_channel == 2 * gN)
    {
      f->ph = 0; // Phase 0
      f->intm = rand_min_intr_mesh(f->src, f->dest);
    }

    if ((f->ph == 0) && (r->GetID() == f->intm))
    {
      f->ph = 1; // Go to phase 1
    }

    out_port = dor_next_mesh(r->GetID(), (f->ph == 0) ? f->intm : f->dest);
  }

  outputs->Clear();

  outputs->AddRange(out_port, vcBegin, vcEnd);
}

//=============================================================

void min_adapt_mesh(const Router *r, const Flit *f, long long int in_channel, OutputSet *outputs, bool inject)
{
  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  outputs->Clear();

  if (inject)
  {
    // injection can use all VCs
    outputs->AddRange(-1, vcBegin, vcEnd);
    return;
  }
  else if (r->GetID() == f->dest)
  {
    // ejection can also use all VCs
    outputs->AddRange(2 * gN, vcBegin, vcEnd);
    return;
  }

  long long int in_vc;

  if (in_channel == 2 * gN)
  {
    in_vc = vcEnd; // ignore the injection VC
  }
  else
  {
    in_vc = f->vc;
  }

  // DOR for the escape channel (VC 0), low priority
  long long int out_port = dor_next_mesh(r->GetID(), f->dest);
  outputs->AddRange(out_port, 0, vcBegin, vcBegin);

  if (f->watch)
  {
    *gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
               << "Adding VC range ["
               << vcBegin << ","
               << vcBegin << "]"
               << " at output port " << out_port
               << " for flit " << f->id
               << " (input port " << in_channel
               << ", destination " << f->dest << ")"
               << "." << endl;
  }

  if (in_vc != vcBegin)
  { // If not in the escape VC
    // Minimal adaptive for all other channels
    long long int cur = r->GetID();
    long long int dest = f->dest;

    for (long long int n = 0; n < gN; ++n)
    {
      if ((cur % gK) != (dest % gK))
      {
        // Add minimal direction in dimension 'n'
        if ((cur % gK) < (dest % gK))
        { // Right
          if (f->watch)
          {
            *gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
                       << "Adding VC range ["
                       << (vcBegin + 1) << ","
                       << vcEnd << "]"
                       << " at output port " << 2 * n
                       << " with priority " << 1
                       << " for flit " << f->id
                       << " (input port " << in_channel
                       << ", destination " << f->dest << ")"
                       << "." << endl;
          }
          outputs->AddRange(2 * n, vcBegin + 1, vcEnd, 1);
        }
        else
        { // Left
          if (f->watch)
          {
            *gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
                       << "Adding VC range ["
                       << (vcBegin + 1) << ","
                       << vcEnd << "]"
                       << " at output port " << 2 * n + 1
                       << " with priority " << 1
                       << " for flit " << f->id
                       << " (input port " << in_channel
                       << ", destination " << f->dest << ")"
                       << "." << endl;
          }
          outputs->AddRange(2 * n + 1, vcBegin + 1, vcEnd, 1);
        }
      }
      cur /= gK;
      dest /= gK;
    }
  }
}

//=============================================================

void planar_adapt_mesh(const Router *r, const Flit *f, long long int in_channel, OutputSet *outputs, bool inject)
{
  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  outputs->Clear();

  if (inject)
  {
    // injection can use all VCs
    outputs->AddRange(-1, vcBegin, vcEnd);
    return;
  }

  long long int cur = r->GetID();
  long long int dest = f->dest;

  if (cur != dest)
  {

    long long int in_vc = f->vc;
    long long int vc_mult = (vcEnd - vcBegin + 1) / 3;

    // Find the first unmatched dimension -- except
    // for when we're in the first dimension because
    // of misrouting in the last adaptive plane.
    // In this case, go to the last dimension instead.

    long long int n;
    for (n = 0; n < gN; ++n)
    {
      if (((cur % gK) != (dest % gK)) &&
          !((in_channel / 2 == 0) &&
            (n == 0) &&
            (in_vc < vcBegin + 2 * vc_mult)))
      {
        break;
      }

      cur /= gK;
      dest /= gK;
    }

    assert(n < gN);

    if (f->watch)
    {
      *gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
                 << "PLANAR ADAPTIVE: flit " << f->id
                 << " in adaptive plane " << n << "." << endl;
    }

    // We're in adaptive plane n

    // Can route productively in d_{i,2}
    bool increase;
    bool fault;
    if ((cur % gK) < (dest % gK))
    { // Increasing
      increase = true;
      if (!r->IsFaultyOutput(2 * n))
      {
        outputs->AddRange(2 * n, vcBegin + 2 * vc_mult, vcEnd);
        fault = false;

        if (f->watch)
        {
          *gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
                     << "PLANAR ADAPTIVE: increasing in dimension " << n
                     << "." << endl;
        }
      }
      else
      {
        fault = true;
      }
    }
    else
    { // Decreasing
      increase = false;
      if (!r->IsFaultyOutput(2 * n + 1))
      {
        outputs->AddRange(2 * n + 1, vcBegin + 2 * vc_mult, vcEnd);
        fault = false;

        if (f->watch)
        {
          *gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
                     << "PLANAR ADAPTIVE: decreasing in dimension " << n
                     << "." << endl;
        }
      }
      else
      {
        fault = true;
      }
    }

    n = (n + 1) % gN;
    cur /= gK;
    dest /= gK;

    if (!increase)
    {
      vcBegin += vc_mult;
    }
    vcEnd = vcBegin + vc_mult - 1;

    long long int d1_min_c;
    if ((cur % gK) < (dest % gK))
    { // Increasing in d_{i+1}
      d1_min_c = 2 * n;
    }
    else if ((cur % gK) != (dest % gK))
    { // Decreasing in d_{i+1}
      d1_min_c = 2 * n + 1;
    }
    else
    {
      d1_min_c = -1;
    }

    // do we want to 180?  if so, the last
    // route was a misroute in this dimension,
    // if there is no fault in d_i, just ignore
    // this dimension, otherwise continue to misroute
    if (d1_min_c == in_channel)
    {
      if (fault)
      {
        d1_min_c = in_channel ^ 1;
      }
      else
      {
        d1_min_c = -1;
      }

      if (f->watch)
      {
        *gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
                   << "PLANAR ADAPTIVE: avoiding 180 in dimension " << n
                   << "." << endl;
      }
    }

    if (d1_min_c != -1)
    {
      if (!r->IsFaultyOutput(d1_min_c))
      {
        outputs->AddRange(d1_min_c, vcBegin, vcEnd);
      }
      else if (fault)
      {
        // major problem ... fault in d_i and d_{i+1}
        r->Error("There seem to be faults in d_i and d_{i+1}");
      }
    }
    else if (fault)
    { // need to misroute!
      bool atedge;
      if (cur % gK == 0)
      {
        d1_min_c = 2 * n;
        atedge = true;
      }
      else if (cur % gK == gK - 1)
      {
        d1_min_c = 2 * n + 1;
        atedge = true;
      }
      else
      {
        d1_min_c = 2 * n + RandomInt(1); // random misroute

        if (d1_min_c == in_channel)
        { // don't 180
          d1_min_c = in_channel ^ 1;
        }
        atedge = false;
      }

      if (!r->IsFaultyOutput(d1_min_c))
      {
        outputs->AddRange(d1_min_c, vcBegin, vcEnd);
      }
      else if (!atedge && !r->IsFaultyOutput(d1_min_c ^ 1))
      {
        outputs->AddRange(d1_min_c ^ 1, vcBegin, vcEnd);
      }
      else
      {
        // major problem ... fault in d_i and d_{i+1}
        r->Error("There seem to be faults in d_i and d_{i+1}");
      }
    }
  }
  else
  {
    outputs->AddRange(2 * gN, vcBegin, vcEnd);
  }
}

//=============================================================

void valiant_mesh(const Router *r, const Flit *f, long long int in_channel, OutputSet *outputs, bool inject)
{
  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  long long int out_port;

  if (inject)
  {
    out_port = -1;
  }
  else
  {
    if (in_channel == 2 * gN)
    {
      f->ph = 0; // Phase 0
      f->intm = RandomInt(gNodes - 1);
      //      printf("\nIntermediate node for the packet from source : %lld, destination : %lld is %lld\n", f->src, f->dest, f->intm);		//Sneha
    }

    if ((f->ph == 0) && (r->GetID() == f->intm))
    {
      f->ph = 1; // Go to phase 1
    }

    out_port = dor_next_mesh(r->GetID(), (f->ph == 0) ? f->intm : f->dest);
    printf("\nThe outport for the packet from Source : %lld to Dest : %lld and intermediate node : %lld at router %lld is: %lld\n", f->src, f->dest, f->intm, r->GetID(), out_port); //Sneha

    // at the destination router, we don't need to separate VCs by phase
    if (r->GetID() != f->dest)
    {
      //each class must have at least 2 vcs assigned or else valiant valiant will deadlock
      long long int const available_vcs = (vcEnd - vcBegin + 1) / 2;
      assert(available_vcs > 0);
      if (f->ph == 0)
      {
        vcEnd -= available_vcs;
      }
      else
      {
        assert(f->ph == 1);
        vcBegin += available_vcs;
      }
    }
  }
  outputs->Clear();
  outputs->AddRange(out_port, vcBegin, vcEnd);
}

//=============================================================

void valiant_torus(const Router *r, const Flit *f, long long int in_channel, OutputSet *outputs, bool inject)
{
  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  long long int out_port;

  if (inject)
  {

    out_port = -1;
  }
  else
  {

    long long int phase;
    if (in_channel == 2 * gN)
    {
      phase = 0; // Phase 0
      f->intm = RandomInt(gNodes - 1);
    }
    else
    {
      phase = f->ph / 2;
    }

    if ((phase == 0) && (r->GetID() == f->intm))
    {
      phase = 1;           // Go to phase 1
      in_channel = 2 * gN; // ensures correct vc selection at the beginning of phase 2
    }

    long long int ring_part;
    dor_next_torus(r->GetID(), (phase == 0) ? f->intm : f->dest, in_channel,
                   &out_port, &ring_part, false);

    f->ph = 2 * phase + ring_part;

    // at the destination router, we don't need to separate VCs by phase, etc.
    if (r->GetID() != f->dest)
    {

      long long int const ring_available_vcs = (vcEnd - vcBegin + 1) / 2;
      assert(ring_available_vcs > 0);

      if (ring_part == 0)
      {
        vcEnd -= ring_available_vcs;
      }
      else
      {
        assert(ring_part == 1);
        vcBegin += ring_available_vcs;
      }

      long long int const ph_available_vcs = ring_available_vcs / 2;
      assert(ph_available_vcs > 0);

      if (phase == 0)
      {
        vcEnd -= ph_available_vcs;
      }
      else
      {
        assert(phase == 1);
        vcBegin += ph_available_vcs;
      }
    }
  }

  outputs->Clear();

  outputs->AddRange(out_port, vcBegin, vcEnd);
}

//=============================================================

void valiant_ni_torus(const Router *r, const Flit *f, long long int in_channel, OutputSet *outputs, bool inject)
{
  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  // at the destination router, we don't need to separate VCs by destination
  if (inject || (r->GetID() != f->dest))
  {

    long long int const vcs_per_dest = (vcEnd - vcBegin + 1) / gNodes;
    assert(vcs_per_dest > 0);

    vcBegin += f->dest * vcs_per_dest;
    vcEnd = vcBegin + vcs_per_dest - 1;
  }

  long long int out_port;

  if (inject)
  {

    out_port = -1;
  }
  else
  {

    long long int phase;
    if (in_channel == 2 * gN)
    {
      phase = 0; // Phase 0
      f->intm = RandomInt(gNodes - 1);
    }
    else
    {
      phase = f->ph / 2;
    }

    if ((f->ph == 0) && (r->GetID() == f->intm))
    {
      f->ph = 1;           // Go to phase 1
      in_channel = 2 * gN; // ensures correct vc selection at the beginning of phase 2
    }

    long long int ring_part;
    dor_next_torus(r->GetID(), (f->ph == 0) ? f->intm : f->dest, in_channel,
                   &out_port, &ring_part, false);

    f->ph = 2 * phase + ring_part;

    // at the destination router, we don't need to separate VCs by phase, etc.
    if (r->GetID() != f->dest)
    {

      long long int const ring_available_vcs = (vcEnd - vcBegin + 1) / 2;
      assert(ring_available_vcs > 0);

      if (ring_part == 0)
      {
        vcEnd -= ring_available_vcs;
      }
      else
      {
        assert(ring_part == 1);
        vcBegin += ring_available_vcs;
      }

      long long int const ph_available_vcs = ring_available_vcs / 2;
      assert(ph_available_vcs > 0);

      if (phase == 0)
      {
        vcEnd -= ph_available_vcs;
      }
      else
      {
        assert(phase == 1);
        vcBegin += ph_available_vcs;
      }
    }

    if (f->watch)
    {
      *gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
                 << "Adding VC range ["
                 << vcBegin << ","
                 << vcEnd << "]"
                 << " at output port " << out_port
                 << " for flit " << f->id
                 << " (input port " << in_channel
                 << ", destination " << f->dest << ")"
                 << "." << endl;
    }
  }

  outputs->Clear();

  outputs->AddRange(out_port, vcBegin, vcEnd);
}

//=============================================================

void dim_order_torus(const Router *r, const Flit *f, long long int in_channel,
                     OutputSet *outputs, bool inject)
{
  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  long long int out_port;

  if (inject)
  {

    out_port = -1;
  }
  else
  {

    long long int cur = r->GetID();
    long long int dest = f->dest;

    dor_next_torus(cur, dest, in_channel,
                   &out_port, &f->ph, false);

    // at the destination router, we don't need to separate VCs by ring partition
    if (cur != dest)
    {

      long long int const available_vcs = (vcEnd - vcBegin + 1) / 2;
      assert(available_vcs > 0);

      if (f->ph == 0)
      {
        vcEnd -= available_vcs;
      }
      else
      {
        vcBegin += available_vcs;
      }
    }

    if (f->watch)
    {
      *gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
                 << "Adding VC range ["
                 << vcBegin << ","
                 << vcEnd << "]"
                 << " at output port " << out_port
                 << " for flit " << f->id
                 << " (input port " << in_channel
                 << ", destination " << f->dest << ")"
                 << "." << endl;
    }
  }

  outputs->Clear();

  outputs->AddRange(out_port, vcBegin, vcEnd);
}

//=============================================================

void dim_order_ni_torus(const Router *r, const Flit *f, long long int in_channel,
                        OutputSet *outputs, bool inject)
{
  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  long long int out_port;

  if (inject)
  {

    out_port = -1;
  }
  else
  {

    long long int cur = r->GetID();
    long long int dest = f->dest;

    dor_next_torus(cur, dest, in_channel,
                   &out_port, NULL, false);

    // at the destination router, we don't need to separate VCs by destination
    if (cur != dest)
    {

      long long int const vcs_per_dest = (vcEnd - vcBegin + 1) / gNodes;
      assert(vcs_per_dest);

      vcBegin += f->dest * vcs_per_dest;
      vcEnd = vcBegin + vcs_per_dest - 1;
    }

    if (f->watch)
    {
      *gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
                 << "Adding VC range ["
                 << vcBegin << ","
                 << vcEnd << "]"
                 << " at output port " << out_port
                 << " for flit " << f->id
                 << " (input port " << in_channel
                 << ", destination " << f->dest << ")"
                 << "." << endl;
    }
  }

  outputs->Clear();

  outputs->AddRange(out_port, vcBegin, vcEnd);
}

//=============================================================

void dim_order_bal_torus(const Router *r, const Flit *f, long long int in_channel,
                         OutputSet *outputs, bool inject)
{
  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  long long int out_port;

  if (inject)
  {

    out_port = -1;
  }
  else
  {

    long long int cur = r->GetID();
    long long int dest = f->dest;

    dor_next_torus(cur, dest, in_channel,
                   &out_port, &f->ph, true);

    // at the destination router, we don't need to separate VCs by ring partition
    if (cur != dest)
    {

      long long int const available_vcs = (vcEnd - vcBegin + 1) / 2;
      assert(available_vcs > 0);

      if (f->ph == 0)
      {
        vcEnd -= available_vcs;
      }
      else
      {
        assert(f->ph == 1);
        vcBegin += available_vcs;
      }
    }

    if (f->watch)
    {
      *gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
                 << "Adding VC range ["
                 << vcBegin << ","
                 << vcEnd << "]"
                 << " at output port " << out_port
                 << " for flit " << f->id
                 << " (input port " << in_channel
                 << ", destination " << f->dest << ")"
                 << "." << endl;
    }
  }

  outputs->Clear();

  outputs->AddRange(out_port, vcBegin, vcEnd);
}

//=============================================================

void min_adapt_torus(const Router *r, const Flit *f, long long int in_channel, OutputSet *outputs, bool inject)
{
  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  outputs->Clear();

  if (inject)
  {
    // injection can use all VCs
    outputs->AddRange(-1, vcBegin, vcEnd);
    return;
  }
  else if (r->GetID() == f->dest)
  {
    // ejection can also use all VCs
    outputs->AddRange(2 * gN, vcBegin, vcEnd);
  }

  long long int in_vc;
  if (in_channel == 2 * gN)
  {
    in_vc = vcEnd; // ignore the injection VC
  }
  else
  {
    in_vc = f->vc;
  }

  long long int cur = r->GetID();
  long long int dest = f->dest;

  long long int out_port;

  if (in_vc > (vcBegin + 1))
  { // If not in the escape VCs
    // Minimal adaptive for all other channels

    for (long long int n = 0; n < gN; ++n)
    {
      if ((cur % gK) != (dest % gK))
      {
        long long int dist2 = gK - 2 * (((dest % gK) - (cur % gK) + gK) % gK);

        if (dist2 > 0)
        {                                                        /*) || 
			     ( ( dist2 == 0 ) && ( RandomInt( 1 ) ) ) ) {*/
          outputs->AddRange(2 * n, vcBegin + 3, vcBegin + 3, 1); // Right
        }
        else
        {
          outputs->AddRange(2 * n + 1, vcBegin + 3, vcBegin + 3, 1); // Left
        }
      }

      cur /= gK;
      dest /= gK;
    }

    // DOR for the escape channel (VCs 0-1), low priority ---
    // trick the algorithm with the in channel.  want VC assignment
    // as if we had injected at this node
    dor_next_torus(r->GetID(), f->dest, 2 * gN,
                   &out_port, &f->ph, false);
  }
  else
  {
    // DOR for the escape channel (VCs 0-1), low priority
    dor_next_torus(cur, dest, in_channel,
                   &out_port, &f->ph, false);
  }

  if (f->ph == 0)
  {
    outputs->AddRange(out_port, vcBegin, vcBegin, 0);
  }
  else
  {
    outputs->AddRange(out_port, vcBegin + 1, vcBegin + 1, 0);
  }
}

//=============================================================

void dest_tag_fly(const Router *r, const Flit *f, long long int in_channel,
                  OutputSet *outputs, bool inject)
{
  long long int vcBegin = gBeginVCs[f->cl];
  long long int vcEnd = gEndVCs[f->cl];
  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

  long long int out_port;

  if (inject)
  {

    out_port = -1;
  }
  else
  {

    long long int stage = (r->GetID() * gK) / gNodes;
    long long int dest = f->dest;

    while (stage < (gN - 1))
    {
      dest /= gK;
      ++stage;
    }

    out_port = dest % gK;
  }

  outputs->Clear();

  outputs->AddRange(out_port, vcBegin, vcEnd);
}

//=============================================================

void chaos_torus(const Router *r, const Flit *f,
                 long long int in_channel, OutputSet *outputs, bool inject)
{
  outputs->Clear();

  if (inject)
  {
    outputs->AddRange(-1, 0, 0);
    return;
  }

  long long int cur = r->GetID();
  long long int dest = f->dest;

  if (cur != dest)
  {
    for (long long int n = 0; n < gN; ++n)
    {

      if ((cur % gK) != (dest % gK))
      {
        long long int dist2 = gK - 2 * (((dest % gK) - (cur % gK) + gK) % gK);

        if (dist2 >= 0)
        {
          outputs->AddRange(2 * n, 0, 0); // Right
        }

        if (dist2 <= 0)
        {
          outputs->AddRange(2 * n + 1, 0, 0); // Left
        }
      }

      cur /= gK;
      dest /= gK;
    }
  }
  else
  {
    outputs->AddRange(2 * gN, 0, 0);
  }
}

//=============================================================

void chaos_mesh(const Router *r, const Flit *f, long long int in_channel, OutputSet *outputs, bool inject)
{
  outputs->Clear();

  if (inject)
  {
    outputs->AddRange(-1, 0, 0);
    return;
  }

  long long int cur = r->GetID();
  long long int dest = f->dest;

  if (cur != dest)
  {
    for (long long int n = 0; n < gN; ++n)
    {
      if ((cur % gK) != (dest % gK))
      {
        // Add minimal direction in dimension 'n'
        if ((cur % gK) < (dest % gK))
        { // Right
          outputs->AddRange(2 * n, 0, 0);
        }
        else
        { // Left
          outputs->AddRange(2 * n + 1, 0, 0);
        }
      }
      cur /= gK;
      dest /= gK;
    }
  }
  else
  {
    outputs->AddRange(2 * gN, 0, 0);
  }
}

//=============================================================

void InitializeRoutingMap(const Configuration &config)
{

  gNumVCs = config.GetLongInt("num_vcs");

  long long int const classes = config.GetLongInt("classes");

  //
  // traffic class partitions
  //

  gBeginVCs = config.GetIntArray("start_vc");
  if (gBeginVCs.empty())
  {
    long long int const start_vc = config.GetLongInt("start_vc");
    if (start_vc < 0)
    {
      for (long long int cl = 0; cl < classes; ++cl)
        gBeginVCs.push_back((cl * gNumVCs) / classes);
    }
    else
      gBeginVCs.push_back(start_vc);
  }
  gBeginVCs.resize(classes, gBeginVCs.back());

  gEndVCs = config.GetIntArray("end_vc");
  if (gEndVCs.empty())
  {
    long long int const end_vc = config.GetLongInt("end_vc");
    if (end_vc < 0)
    {
      for (long long int cl = 0; cl < classes; ++cl)
        gEndVCs.push_back(((cl + 1) * gNumVCs) / classes - 1);
    }
    else
      gEndVCs.push_back(end_vc);
  }
  gEndVCs.resize(classes, gEndVCs.back());
  for (long long int c = 0; c < classes; ++c)
  {
    assert(gBeginVCs[c] <= gEndVCs[c]);
  }

  /* Register routing functions here */

  // ===================================================
  // Balfour-Schultz
  gRoutingFunctionMap["nca_fattree"] = &fattree_nca;
  gRoutingFunctionMap["anca_fattree"] = &fattree_anca;
  gRoutingFunctionMap["nca_qtree"] = &qtree_nca;
  gRoutingFunctionMap["nca_tree4"] = &tree4_nca;
  gRoutingFunctionMap["anca_tree4"] = &tree4_anca;
  gRoutingFunctionMap["dor_mesh"] = &dim_order_mesh;
  gRoutingFunctionMap["xy_yx_mesh"] = &xy_yx_mesh;
  gRoutingFunctionMap["adaptive_xy_yx_mesh"] = &adaptive_xy_yx_mesh;
  // End Balfour-Schultz
  // ===================================================

  gRoutingFunctionMap["dim_order_mesh"] = &dim_order_mesh;
  gRoutingFunctionMap["dim_order_ni_mesh"] = &dim_order_ni_mesh;
  gRoutingFunctionMap["dim_order_pni_mesh"] = &dim_order_pni_mesh;
  gRoutingFunctionMap["dim_order_torus"] = &dim_order_torus;
  gRoutingFunctionMap["dim_order_ni_torus"] = &dim_order_ni_torus;
  gRoutingFunctionMap["dim_order_bal_torus"] = &dim_order_bal_torus;

  gRoutingFunctionMap["romm_mesh"] = &romm_mesh;
  gRoutingFunctionMap["romm_ni_mesh"] = &romm_ni_mesh;

  gRoutingFunctionMap["min_adapt_mesh"] = &min_adapt_mesh;
  gRoutingFunctionMap["min_adapt_torus"] = &min_adapt_torus;

  gRoutingFunctionMap["planar_adapt_mesh"] = &planar_adapt_mesh;

  // FIXME: This is broken.
  //  gRoutingFunctionMap["limited_adapt_mesh"] = &limited_adapt_mesh;

  gRoutingFunctionMap["valiant_mesh"] = &valiant_mesh;
  gRoutingFunctionMap["valiant_torus"] = &valiant_torus;
  gRoutingFunctionMap["valiant_ni_torus"] = &valiant_ni_torus;

  gRoutingFunctionMap["dest_tag_fly"] = &dest_tag_fly;

  gRoutingFunctionMap["chaos_mesh"] = &chaos_mesh;
  gRoutingFunctionMap["chaos_torus"] = &chaos_torus;
}
