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

#include "cplex-ring-routing.h"

ILOSTLBEGIN

NS_LOG_COMPONENT_DEFINE ("CplexRingRouting");

namespace ns3 {

CplexRingRouting::CplexRingRouting ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

CplexRingRouting::~CplexRingRouting ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
CplexRingRouting::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

TypeId 
CplexRingRouting::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CplexRingRouting")
    .SetParent<Object> ()
    .AddConstructor<CplexRingRouting> ()
    .AddAttribute ("NumberNodes", 
                   "The number of nodes in the ring",
                   UintegerValue (4),
                   MakeUintegerAccessor (&CplexRingRouting::m_nodes),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("LinkCapacity", 
                   "The capacity of each link in the ring",
                   UintegerValue (16),
                   MakeUintegerAccessor (&CplexRingRouting::m_capacity),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

uint32_t
CplexRingRouting::GetIndex (uint16_t i, uint16_t j, uint16_t n)
{
  NS_ASSERT_MSG (i < j, "Invalide values for i and j indexes");
  return (uint32_t)((i * n + j)/*A*/ - (i + 1)/*B*/ - (i * (i + 1) / 2)/*C*/);
  // Note:  A --> common conversion from matrix to array
  //        B --> for the non-used elements in line i
  //        C --> for the non-used elements in lines i-1 to line 0
}

uint8_t
CplexRingRouting::UsesLink (uint16_t i, uint16_t j, uint16_t link)
{
  NS_ASSERT_MSG (i < j, "Invalide values for i and j indexes");
  return (i <= link && j > link);
}

uint64_t 
CplexRingRouting::Factorial (uint32_t x)
{
  uint64_t f = 1;
  uint32_t i;
  for (i = 1; i <= x; i++)
    f = f * i;
  return f;
}

uint32_t
CplexRingRouting::Combinations (uint16_t n, uint16_t k)
{
  NS_ASSERT (n > 0 && n >= k);
  return (uint32_t)(Factorial (n) / ( Factorial (k) * Factorial (n - k)));
}

void
CplexRingRouting::Solve ()
{
  NS_LOG_FUNCTION_NOARGS ();
  
  // Create the environment
  IloEnv env;   
  try 
    {
      IloModel model (env);
      IloInt i, j, l;
      
      IloInt nElements = Combinations (m_nodes, 2);

      // These decision variables represents the routing choice for each pair i:j
      // (clockwise or counterclockwise).
      IloBoolVarArray clock   = IloBoolVarArray (env);
      IloBoolVarArray counter = IloBoolVarArray (env);
      for (i = 0; i < m_nodes -1; i++) 
        {
          for (j = i + 1; j < m_nodes; j++)
            {
              std::ostringstream u, uc;
              u  << "U_"  << i+1 << j+1;
              uc << "C_" << i+1 << j+1;
              clock.add   (IloBoolVar (env, u.str().c_str()));
              counter.add (IloBoolVar (env, uc.str().c_str()));
            }
        }

      // Traffic demmand for each pair i:j (values from book example)
      IloIntArray demmand = IloIntArray (env, nElements, 4, 4, 8, 4, 8, 8);

      // Objective function
      // Load balancing factor, which we want to minimize
      IloNumVar load = IloNumVar (env, 0.0, IloInfinity, "r");
      model.add (IloMinimize (env, load));

      // Constraint: load must be non-negative
      model.add (load >= 0.0);

      // Constraint: only one path can be selected: clockwise or counterclockwise
      for (i = 0; i < nElements; i++)
        {
          model.add (1 == clock [i] + counter [i]);
        }

      // Constraint: cannot exceed link capacity
      for (l = 0; l < m_nodes; l++)
        {
          IloExpr exp (env);
          for (i = 0; i < m_nodes -1; i++) 
            {
              for (j = i + 1; j < m_nodes; j++)
                {
                  exp += demmand [GetIndex (i, j, m_nodes)] * UsesLink (i, j, l) * clock [GetIndex (i, j, m_nodes)];
                  exp += demmand [GetIndex (i, j, m_nodes)] * (1 - UsesLink (i, j, l)) * counter [GetIndex (i, j, m_nodes)];
                }
            }
          model.add (exp <= 16 * load);
        }

      // Optimize
      IloCplex cplex (model);
      cplex.exportModel ("ringrouting.lp");
      cplex.setOut (env.getNullStream());
      cplex.setWarning (env.getNullStream());
      cplex.solve ();
       
      // Print results
      if (cplex.getStatus () == IloAlgorithm::Infeasible)
        {
          env.out () << "No Solution" << endl;
        }
      env.out () << "Load-balancing factor: " << cplex.getObjValue () << endl;
      
      for (i = 0; i < m_nodes -1; i++) 
        {
          for (j = i + 1; j < m_nodes; j++)
            {
              env.out () << "For " << i+1 << " to " << j+1 << ": ";
              if (cplex.getValue (clock[GetIndex (i, j, m_nodes)]))
                {
                  env.out () << "clockwise" << endl; 
                }
              else
                {
                  env.out () << "counterclockwise" << endl;
                }

            }
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
