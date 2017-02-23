/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Campinas (Unicamp)
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

#ifndef SDRAN_CLOUD_H
#define SDRAN_CLOUD_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/mobility-module.h>

namespace ns3 {

/**
 * This class represents the SDRAN cloud at the SDMN architecture.
 */
class SdranCloud : public Object
{
public:
  SdranCloud ();           //!< Default constructor.
  virtual ~SdranCloud ();  //!< Dummy destructor.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \return The SDRAN Cloud ID. */
  uint32_t GetId (void) const;

  /**
   * Get the container of eNBs nodes created by this SDRAN cloud.
   * \return The container of eNB nodes.
   */
  NodeContainer GetEnbNodes (void) const;

  /** \return The S-GW node pointer. */
  Ptr<Node> GetSgwNode ();

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  // Inherited from ObjectBase
  void NotifyConstructionCompleted (void);

private:
  uint32_t                m_sdranId;        //!< SDRAN cloud id.
  uint32_t                m_nSites;         //!< Number of cell sites.
  uint32_t                m_nEnbs;          //!< Number of eNBs (3 * m_nSites).
  NodeContainer           m_enbNodes;       //!< eNB nodes.
  Ptr<Node>               m_sgwNode;        //!< S-GW node.

  static uint32_t         m_enbCounter;     //!< Global eNB counter.
  static uint32_t         m_sdranCounter;   //!< Global SDRAN cloud counter.
};

} // namespace ns3
#endif /* SDRAN_CLOUD_H */
