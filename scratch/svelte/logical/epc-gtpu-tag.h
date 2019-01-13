/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Campinas (Unicamp)
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

#ifndef EPC_GTPU_TAG_H
#define EPC_GTPU_TAG_H

#include "ns3/tag.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "../svelte-common.h"

namespace ns3 {

class Tag;

/**
 * Tag used to identify the GTP TEID for packets.
 */
class EpcGtpuTag : public Tag
{
public:
  /** LTE EPC element where this tagged was inserted into the packet */
  enum EpcInputNode
  {
    ENB = 0,  //!< At the eNB node
    PGW = 1   //!< At the P-GW node
  };

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  /** Constructors */
  EpcGtpuTag ();
  EpcGtpuTag (uint32_t teid, EpcInputNode inputNode);

  // Inherited from Tag
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual uint32_t GetSerializedSize () const;
  virtual void Print (std::ostream &os) const;

  /** \return the direction for this traffic */
  Direction GetDirection () const;

  /** \return the teid field */
  uint32_t GetTeid () const;

  /** \return the input node field */
  EpcInputNode GetInputNode () const;

  /** \return the timestamp field */
  Time GetTimestamp () const;

  /** Set the teid field */
  void SetTeid (uint32_t teid);

  /** Set the input node field */
  void SetInputNode (EpcInputNode inputNode);

  /** \return true when downlink traffic */
  bool IsDownlink () const;

  /** \return true when uplink traffic */
  bool IsUplink () const;

private:
  uint32_t  m_teid;       //!< GTP teid
  uint8_t   m_inputNode;  //!< Input node
  uint64_t  m_ts;         //!< Input timestamp
};

} // namespace ns3
#endif // EPC_GTPU_TAG_H
