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

#include "gtpu-tag.h"
#include "../mano-apps/global-ids.h"

// Metadata bitmap.
#define META_NODE 0
#define META_TYPE 1
#define META_AGGR 2

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (GtpuTag);

GtpuTag::GtpuTag ()
  : m_meta (0),
  m_teid (0),
  m_time (Simulator::Now ().GetTimeStep ())
{
}

GtpuTag::GtpuTag (uint32_t teid, InputNode node, QosType type, bool aggr)
  : m_meta (0),
  m_teid (teid),
  m_time (Simulator::Now ().GetTimeStep ())
{
  SetMetadata (node, type, aggr);
}

TypeId
GtpuTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GtpuTag")
    .SetParent<Tag> ()
    .AddConstructor<GtpuTag> ()
  ;
  return tid;
}

TypeId
GtpuTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
GtpuTag::GetSerializedSize (void) const
{
  return 13;
}

void
GtpuTag::Serialize (TagBuffer i) const
{
  i.WriteU8  (m_meta);
  i.WriteU32 (m_teid);
  i.WriteU64 (m_time);
}

void
GtpuTag::Deserialize (TagBuffer i)
{
  m_meta = i.ReadU8 ();
  m_teid = i.ReadU32 ();
  m_time = i.ReadU64 ();
}

void
GtpuTag::Print (std::ostream &os) const
{
  os << " teid=" << m_teid
     << " node=" << InputNodeStr (GetInputNode ())
     << " type=" << QosTypeStr (GetQosType ())
     << " aggr=" << (IsAggregated () ? "true" : "false")
     << " time=" << m_time;
}

Direction
GtpuTag::GetDirection () const
{
  return GetInputNode () == GtpuTag::PGW ?
         Direction::DLINK : Direction::ULINK;
}

GtpuTag::InputNode
GtpuTag::GetInputNode () const
{
  uint8_t node = (m_meta & (1U << META_NODE));
  return static_cast<GtpuTag::InputNode> (node >> META_NODE);
}

QosType
GtpuTag::GetQosType () const
{
  if (IsAggregated ())
    {
      return QosType::NON;
    }

  uint8_t type = (m_meta & (1U << META_TYPE));
  return static_cast<QosType> (type >> META_TYPE);
}

SliceId
GtpuTag::GetSliceId () const
{
  return GlobalIds::TeidGetSliceId (m_teid);
}

uint32_t
GtpuTag::GetTeid () const
{
  return m_teid;
}

Time
GtpuTag::GetTimestamp () const
{
  return Time (m_time);
}

bool
GtpuTag::IsAggregated () const
{
  return (m_meta & (1U << META_AGGR));
}

std::string
GtpuTag::InputNodeStr (InputNode node)
{
  switch (node)
    {
    case InputNode::ENB:
      return "enb";
    case InputNode::PGW:
      return "pgw";
    default:
      return std::string ();
    }
}

void
GtpuTag::SetMetadata (InputNode node, QosType type, bool aggr)
{
  NS_ASSERT_MSG (node <= 0x01, "Input node cannot exceed 1 bit.");
  NS_ASSERT_MSG (type <= 0x01, "QoS type cannot exceed 1 bit.");

  m_meta = 0x00;
  m_meta |= (static_cast<uint8_t> (node) << META_NODE);
  m_meta |= (static_cast<uint8_t> (type) << META_TYPE);
  m_meta |= (static_cast<uint8_t> (aggr) << META_AGGR);
}

} // namespace ns3
