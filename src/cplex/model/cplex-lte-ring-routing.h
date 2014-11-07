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

#ifndef CPLEX_LTE_RING_ROUTING_H
#define CPLEX_LTE_RING_ROUTING_H

#include <ilcplex/ilocplex.h>
#include <ns3/core-module.h>
#include <ns3/data-rate.h>

namespace ns3 {

/**
 * \ingroup cplex
 * A LTE ring network routing problem solved by CPLEX. This model can solve
 * routing problems for bidirectional flow demands between any node in the ring
 * to the gateway sink node.
 */
class CplexLteRingRouting : public Object
{
public:
  CplexLteRingRouting ();          //!< Default constructor
  virtual ~CplexLteRingRouting (); //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  virtual void DoDispose ();

  /**
   * Set the number of nodes in the ring.
   * \param nodes The number of nodes.
   */
  void SetNumNodes (uint16_t nodes);

  /**
   * Add a new flow demand for pair <node,flow>.
   * \param node Source node index.
   * \param flow Flow identifier.
   * \param demand The data rate demand.
   */
  void AddFlowDemand (uint16_t node, uint32_t flow, DataRate demand);

  /**
   * Get the optimun route for pair <node,flow>.
   * \param node Node index.
   * \param flow Flow identifier.
   * \return true for clockwise routing, false otherwise.
   */
  bool GetSolution (uint16_t node, uint32_t flow);

  /** 
   * Solve the load-balancing optimization problem 
   */
  void SolveLoadBalancing ();

private:
  /**
   * Returns the vector index for the i,j indexes from a upper diagonal matrix
   * of order m_nodes into a linear array index. This linear array stores only
   * elements for i < j (it igonres the main diagonal).
   * \param i The row index (starting in 0).
   * \param j The column index (starting in 0).
   * \return The vector index for pair node,flow.
   */
  uint32_t GetNodeFlowIndex (uint16_t node, uint32_t flow);
  
  /**
   * For a node index, this function computes the delta value, which indicates
   * if the traffic in clockwise direction from node to gateway will use the
   * link 'link'. 
   * \param node The node index.
   * \param link The link connection node 'link' to 'link + 1'.
   * \returns 1 if true (will use link 'link'), 0 otherwise.
   */
  uint8_t UsesLink (uint16_t node, uint16_t link);

  /** Mapping pair <node,flow> to linear index */
  typedef std::map<std::pair<uint16_t, uint32_t>, uint32_t> IndexMap_t;
  
  uint16_t              m_nodes;        //!< The number of nodes in the ring.
  DataRate              m_LinkDataRate; //!< The link data rate connecting the nodes.
  bool                  m_solved;       //!< Indicates that the problem has been solved.
  std::vector<uint64_t> m_demands;      //!< Traffic demand for each pair <node,flow>
  std::vector<uint16_t> m_nodeMap;      //!< Map traffic demand to node index
  std::vector<int>      m_routes;       //!< Optimal route for each pair <node,flow>
  IndexMap_t            m_indexMap;     //!< Maps each pair <node,flow> to m_demands vector index
};

}

#endif /* CPLEX_LTE_RING_ROUTING_H */

