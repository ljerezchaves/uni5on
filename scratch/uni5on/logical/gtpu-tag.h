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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef EPC_GTPU_TAG_H
#define EPC_GTPU_TAG_H

#include "ns3/tag.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "../uni5on-common.h"

namespace ns3 {

class Tag;

/**
 * Tag used for GTP packets withing the EPC.
 */
class GtpuTag : public Tag
{
public:
  /** EPC element where this tagged was inserted into the packet */
  enum InputNode
  {
    ENB = 0,  //!< At the eNB node
    PGW = 1   //!< At the P-GW node
  };

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  /** Constructors */
  GtpuTag ();
  GtpuTag (uint32_t teid, InputNode node, QosType type, bool aggr);

  // Inherited from Tag
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Print (std::ostream &os) const;

  /**
   * Private member accessors for tag information.
   * \return The requested information.
   */
  //\{
  Direction     GetDirection  (void) const;
  InputNode     GetInputNode  (void) const;
  QosType       GetQosType    (void) const;
  SliceId       GetSliceId    (void) const;
  uint32_t      GetTeid       (void) const;
  Time          GetTimestamp  (void) const;
  bool          IsAggregated  (void) const;
  //\}

  /**
   * Get the EPC input node name.
   * \param node The EPC input node.
   * \return The string with the EPC input node name.
   */
  static std::string InputNodeStr (InputNode node);

private:
  /**
   * Set internal metadata field.
   * \param node The input node.
   * \param type The QoS traffic type.
   * \param aggr True for aggregated traffic.
   */
  void SetMetadata (InputNode node, QosType type, bool aggr);

  uint8_t   m_meta;       //!< Packet metadata.
  uint32_t  m_teid;       //!< GTP teid.
  uint64_t  m_time;       //!< Input timestamp.
};

} // namespace ns3
#endif // EPC_GTPU_TAG_H
