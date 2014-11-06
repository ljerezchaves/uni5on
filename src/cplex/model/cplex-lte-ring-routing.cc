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
}

TypeId 
CplexLteRingRouting::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CplexLteRingRouting")
    .SetParent<Object> ()
    .AddConstructor<CplexLteRingRouting> ()
    .AddAttribute ("NumberNodes",
                   "The number of nodes in the ring",
                   UintegerValue (4),
                   MakeUintegerAccessor (&CplexLteRingRouting::SetNodes),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("LinkCapacity", 
                   "The capacity of each link in the ring (bps)",
                   UintegerValue (16),
                   MakeUintegerAccessor (&CplexLteRingRouting::m_capacity),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

void
CplexLteRingRouting::SetNodes (uint16_t nodes)
{
  m_solved = false;
  m_nodes = nodes;
  m_nElements = Combinations (m_nodes, 2);
  m_demands.clear ();
  m_demands.reserve (m_nElements);
  for (uint16_t i = 0; i < m_nElements; i++)
    {
      m_demands [i] = 0;
    }

  m_routes.clear ();
  m_routes.reserve (m_nElements);
}

void
CplexLteRingRouting::AddDemand (uint16_t i, uint16_t j, int demand)
{
  if (i == j) return;
  uint16_t index = i < j ? GetIndex (i, j) : GetIndex (j, i);
  m_demands [index] += demand;
}

int
CplexLteRingRouting::GetRoute (uint16_t i, uint16_t j)
{
  NS_ASSERT_MSG (m_solved, "Routing not solved yet.");
  return m_routes [GetIndex (i, j)];
}

uint32_t
CplexLteRingRouting::GetIndex (uint16_t i, uint16_t j)
{
  NS_ASSERT_MSG (i < j, "Invalide values for i and j indexes");
  return (uint32_t)((i * m_nodes + j)/*A*/ - (i + 1)/*B*/ - (i * (i + 1) / 2)/*C*/);
  // Note:  A --> common conversion from matrix to array
  //        B --> for the non-used elements in line i
  //        C --> for the non-used elements in lines i-1 to line 0
}

uint8_t
CplexLteRingRouting::UsesLink (uint16_t i, uint16_t j, uint16_t link)
{
  NS_ASSERT_MSG (i < j, "Invalide values for i and j indexes");
  return (i <= link && j > link);
}

uint64_t 
CplexLteRingRouting::Factorial (uint32_t x)
{
  uint64_t f = 1;
  uint32_t i;
  for (i = 1; i <= x; i++)
    f = f * i;
  return f;
}

uint32_t
CplexLteRingRouting::Combinations (uint16_t n, uint16_t k)
{
  NS_ASSERT (n > 0 && n >= k);
  return (uint32_t)(Factorial (n) / ( Factorial (k) * Factorial (n - k)));
}

void
CplexLteRingRouting::Solve ()
{
  NS_LOG_FUNCTION_NOARGS ();
  
  // Create the environment
  IloEnv env;   
  try 
    {
      IloModel model (env);
      IloInt i, j, l;
      
      // These decision variables represents the routing choice for each pair i:j
      // (clockwise or counterclockwise).
      IloBoolVarArray u  = IloBoolVarArray (env);
      IloBoolVarArray uc = IloBoolVarArray (env);
      for (i = 0; i < m_nodes -1; i++) 
        {
          for (j = i + 1; j < m_nodes; j++)
            {
              std::ostringstream un, ucn;
              un  << "U_"  << i << j;
              ucn << "Uc_" << i << j;
              u.add  (IloBoolVar (env, un.str().c_str()));
              uc.add (IloBoolVar (env, ucn.str().c_str()));
            }
        }

      // Traffic demand for each pair i:j (values set by AddDemand ())
      IloIntArray h = IloIntArray (env);
      for (i = 0; i < m_nElements; i++)
        {
          h.add (IloInt (m_demands [i]));
        }
  
      // Objective function
      // Load balancing factor, which we want to minimize
      IloNumVar r = IloNumVar (env, 0.0, IloInfinity, "r");
      model.add (IloMinimize (env, r));

      // Constraint: load must be non-negative
      model.add (r >= 0.0);

      // Constraint: only one path can be selected: clockwise or counterclockwise
      for (i = 0; i < m_nElements; i++)
        {
          model.add (u [i] + uc [i] == 1);
        }

      // Constraint: cannot exceed link capacity
      for (l = 0; l < m_nodes; l++)
        {
          IloExpr exp (env);
          for (i = 0; i < m_nodes -1; i++) 
            {
              for (j = i + 1; j < m_nodes; j++)
                {
                  exp += h [GetIndex (i, j)] *      UsesLink (i, j, l)  * u  [GetIndex (i, j)];
                  exp += h [GetIndex (i, j)] * (1 - UsesLink (i, j, l)) * uc [GetIndex (i, j)];
                }
            }
          model.add (exp <= 16 * r);
        }

      // Optimize
      IloCplex cplex (model);
      cplex.exportModel ("LTERingRouting.lp");
      cplex.setOut (env.getNullStream());
      cplex.setWarning (env.getNullStream());
      cplex.solve ();
       
      // Print results
      if (cplex.getStatus () == IloAlgorithm::Infeasible)
        {
          NS_LOG_WARN ("No LTERingRouting solution.");
        }
    
      if (cplex.getObjValue () > 1)
        {
          NS_LOG_WARN ("Traffic demand exceeds ring capacity.");
        }

      // Saving results
      m_solved = true;
      for (i = 0; i < m_nElements; i++)
        {
          m_routes [i] = cplex.getValue (u[i]);
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
