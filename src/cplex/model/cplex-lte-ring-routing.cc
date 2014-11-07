/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 University of Campinas (Unicamp)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#include "cplex-lte-ring-routing.h"

ILOSTLBEGIN

NS_LOG_COMPONENT_DEFINE ("CplexLteRingRouting");

namespace ns3 {

CplexLteRingRouting::CplexLteRingRouting ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

CplexLteRingRouting::~CplexLteRingRouting ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
CplexLteRingRouting::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_demands.clear ();
  m_nodeMap.clear ();
  m_indexMap.clear ();
  m_routes.clear ();
}

TypeId 
CplexLteRingRouting::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CplexLteRingRouting")
    .SetParent<Object> ()
    .AddConstructor<CplexLteRingRouting> ()
    .AddAttribute ("NumSwitches", 
                   "The number of OpenFlow switches in the ring.",
                   UintegerValue (3),
                   MakeUintegerAccessor (&CplexLteRingRouting::SetNumNodes),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("LinkDataRate", 
                   "The capacity of each link in the ring",
                   DataRateValue (DataRate ("10Mb/s")),
                   MakeDataRateAccessor (&CplexLteRingRouting::m_LinkDataRate),
                   MakeDataRateChecker ())
  ;
  return tid;
}

void
CplexLteRingRouting::SetNumNodes (uint16_t nodes)
{
  m_solved = false;
  m_nodes = nodes;
  m_demands.clear ();
  m_nodeMap.clear ();
  m_indexMap.clear ();
}

void
CplexLteRingRouting::AddFlowDemand (uint16_t node, uint32_t flow, 
                                    DataRate demand)
{
  NS_ASSERT (node > 0 && flow > 0);

  // Insert the demmand into vector
  m_demands.push_back (demand.GetBitRate ());
  m_nodeMap.push_back (node);
  uint32_t index = m_demands.size () - 1;
  
  // Map the node,flow to vector index
  std::pair<uint16_t, uint32_t> key = std::make_pair (node, flow);
  std::pair<IndexMap_t::iterator, bool> ret;
  ret = m_indexMap.insert (std::make_pair (key, index)); 
  if (ret.second == true) 
    {
      NS_LOG_DEBUG ("Including demand for node " << node << 
                    ", flow " << flow << " of " << demand);
      return;
    }

  NS_FATAL_ERROR ("Pair <node,flow> already exists.");
}

bool
CplexLteRingRouting::GetSolution (uint16_t node, uint32_t flow)
{
  NS_ASSERT (m_solved);

  uint32_t index = GetNodeFlowIndex (node, flow);
  return m_routes [index] ? true : false;
}

uint32_t
CplexLteRingRouting::GetNodeFlowIndex (uint16_t node, uint32_t flow)
{
  NS_ASSERT (node > 0 && flow > 0);

  std::pair<uint16_t, uint32_t> key = std::make_pair (node, flow);
  IndexMap_t::iterator ret;
  ret = m_indexMap.find (key); 
  if (ret != m_indexMap.end ()) 
    {
      return ret->second;
    }
  
  NS_FATAL_ERROR ("Pair <node,flow> does not exists.");
}

uint8_t
CplexLteRingRouting::UsesLink (uint16_t node, uint16_t link)
{
  NS_ASSERT (node > 0); 

  // The gateway node is always node 0, and the nodes are diposed in the ring
  // following clockwise order. 
  return (node <= link);
}

void
CplexLteRingRouting::SolveLoadBalancing ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_solved = false;
  m_routes.clear ();
  
  // Create the environment
  IloEnv env;   
  try 
    {
      IloModel model (env);
      IloNum c = m_LinkDataRate.GetBitRate ();
      IloNum size = m_demands.size ();
      IloNum i, l;
      
      // These decision variables represents the routing choice for each pair
      // <node,flow> (clockwise or counterclockwise).
      IloBoolVarArray u  = IloBoolVarArray (env);
      IloBoolVarArray uc = IloBoolVarArray (env);
      for (i = 0; i < size; i++) 
        {
          std::ostringstream un, ucn;
          un  << "U_"  << i;
          ucn << "Uc_" << i;
          u.add  (IloBoolVar (env, un.str().c_str()));
          uc.add (IloBoolVar (env, ucn.str().c_str()));
        }

      // Traffic demand for each pair <node,flow> (set by AddFlowDemand ())
      IloIntArray h = IloIntArray (env);
      for (i = 0; i < size; i++)
        {
          h.add (IloInt (m_demands [i]));
        }
  
      // Objective function
      // Load balancing factor, which we want to minimize
      IloNumVar r = IloNumVar (env, 0.0, IloInfinity, "r");
      model.add (IloMinimize (env, r));

      // Constraint: load must be non-negative
      model.add (r >= 0.0);

      // Constraint: only one path can be used: clockwise or counterclockwise
      for (i = 0; i < size; i++)
        {
          model.add (u [i] + uc [i] == 1);
        }

      // Constraint: cannot exceed link capacity
      for (l = 0; l < m_nodes; l++)
        {
          IloExpr exp (env);
          for (i = 0; i < size; i++) 
            {
              exp += h[i] *      UsesLink (m_nodeMap[i], l)  *  u[i];
              exp += h[i] * (1 - UsesLink (m_nodeMap[i], l)) * uc[i];
            }
          model.add (exp <= c * r);
        }

      // Optimize
      IloCplex cplex (model);
      cplex.exportModel ("RingLoadBalancing.lp");
      cplex.setOut (env.getNullStream());
      cplex.setWarning (env.getNullStream());
      cplex.solve ();
       
      // Print results
      if (cplex.getStatus () == IloAlgorithm::Infeasible)
        {
          NS_FATAL_ERROR ("No LTERingRouting solution.");
        }
      
      NS_LOG_DEBUG ("Load-balancing factor: " << cplex.getObjValue ());
      if (cplex.getObjValue () > 1)
        {
          NS_LOG_ERROR ("Traffic demand exceeds ring capacity.");
        }

      // Saving results
      m_solved = true;
      for (i = 0; i < size; i++)
        {
          m_routes.push_back (cplex.getValue (u[i]));
        }
    }
  catch (IloException& ex) 
    {
      NS_LOG_ERROR ("Error: " << ex);
    }
  catch (...) 
    {
      NS_LOG_ERROR ("Unknown Error");
    }

  // Destroy the environment
  env.end ();
}

} // namespace ns3
