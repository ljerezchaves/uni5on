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

#include <ns3/core-module.h>
#include <ilcplex/ilocplex.h>

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
  void SetNodes (uint16_t nodes);

  /**
   * Add a new demand for pair i:j.
   * \param i Source node index.
   * \param j Destination node index.
   * \param demand The demand between i and j.
   */
  void AddDemand (uint16_t i, uint16_t j, int demand);

  /**
   * Get the optimun route between nodes i and j.
   * \param i Source node index.
   * \param j Destination node index.
   * \return 1 for clockwise routing, 0 for counterclockwise routing.
   */
  int GetRoute (uint16_t i, uint16_t j);

  /** Solve the LPI problem */
  void Solve ();

private:
  /** 
   * Compute the factorial of x.
   * \param x The integer x.
   * \return The x!.
   */
  uint64_t Factorial (uint32_t x);

  /** Compute the number of k-combinations in a set of n elements.
   * \param n The number of elements n.
   * \param k The k factor.
   * \return The number of k-combinations.
   */
  uint32_t Combinations (uint16_t n, uint16_t k);

  /**
   * Convert the i,j indexes from a upper diagonal matrix of order m_nodes into
   * a linear array index. This linear array stores only elements for i < j (it
   * igonres the main diagonal).
   * \param i The row index (starting in 0).
   * \param j The column index (starting in 0).
   * \return The linear array index.
   */
  uint32_t GetIndex (uint16_t i, uint16_t j);

  /**
   * Computes the value of $\delta_{i,j}^{l}$ for pair i:j and link l. 
   * \param i The source node index.
   * \param j The destination node index.
   * \param l The link connection node l to l + 1
   * \returns 1 if the link l will be used in clockwise routing, 0 otherwise.
   */
  uint8_t UsesLink (uint16_t i, uint16_t j, uint16_t link);

  uint16_t m_nodes;           //!< The number of nodes in the ring.
  uint64_t m_capacity;        //!< The link capacity connecting the nodes.
  uint32_t m_nElements;       //!< Number of pairs i:j
  bool m_solved;              //!< Indicates that the problem has been solved.
  
  std::vector<int> m_demands; //!< Traffic demands for each pair i:j
  std::vector<int> m_routes;  //!< Optimal route for each pair i:j
};

}

#endif /* CPLEX_LTE_RING_ROUTING_H */

