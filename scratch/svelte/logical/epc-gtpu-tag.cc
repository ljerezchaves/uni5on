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

#include "epc-gtpu-tag.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (EpcGtpuTag);

EpcGtpuTag::EpcGtpuTag ()
  : m_meta (0),
  m_teid (0),
  m_time (Simulator::Now ().GetTimeStep ())
{
}

EpcGtpuTag::EpcGtpuTag (uint32_t teid, EpcInputNode node, QosType type)
  : m_meta (0),
  m_teid (teid),
  m_time (Simulator::Now ().GetTimeStep ())
{
  SetMetadata (node, type);
}

TypeId
EpcGtpuTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcGtpuTag")
    .SetParent<Tag> ()
    .AddConstructor<EpcGtpuTag> ()
  ;
  return tid;
}

TypeId
EpcGtpuTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
EpcGtpuTag::GetSerializedSize (void) const
{
  return 13;
}

void
EpcGtpuTag::Serialize (TagBuffer i) const
{
  i.WriteU8  (m_meta);
  i.WriteU32 (m_teid);
  i.WriteU64 (m_time);
}

void
EpcGtpuTag::Deserialize (TagBuffer i)
{
  m_meta = i.ReadU8 ();
  m_teid = i.ReadU32 ();
  m_time = i.ReadU64 ();
}

void
EpcGtpuTag::Print (std::ostream &os) const
{
  os << " teid=" << m_teid
     << " input=" << EpcInputNodeStr (GetInputNode ())
     << " type=" << QosTypeStr (GetQosType ())
     << " timestamp=" << m_time;
}

Direction
EpcGtpuTag::GetDirection () const
{
  return GetInputNode () == EpcGtpuTag::PGW ?
         Direction::DLINK : Direction::ULINK;
}

EpcGtpuTag::EpcInputNode
EpcGtpuTag::GetInputNode () const
{
  uint8_t node = m_meta;
  node &= META_NODE;
  return static_cast<EpcGtpuTag::EpcInputNode> (node);
}

QosType
EpcGtpuTag::GetQosType () const
{
  uint8_t type = m_meta;
  type &= META_TYPE;
  type >>= 1;
  return static_cast<QosType> (type);
}

SliceId
EpcGtpuTag::GetSliceId () const
{
  return GetSliceIdFromTeid (m_teid);
}

uint32_t
EpcGtpuTag::GetTeid () const
{
  return m_teid;
}

Time
EpcGtpuTag::GetTimestamp () const
{
  return Time (m_time);
}

std::string
EpcGtpuTag::EpcInputNodeStr (EpcInputNode node)
{
  switch (node)
    {
    case EpcInputNode::ENB:
      return "enb";
    case EpcInputNode::PGW:
      return "pgw";
    default:
      return std::string ();
    }
}

void
EpcGtpuTag::SetMetadata (EpcInputNode node, QosType type)
{
  NS_ASSERT_MSG (node <= 0x1, "Input node cannot exceed 1 bit.");
  NS_ASSERT_MSG (type <= 0x1, "QoS type cannot exceed 1 bit.");

  m_meta = 0x0;
  m_meta |= static_cast<uint8_t> (type);
  m_meta <<= 1;
  m_meta |= static_cast<uint8_t> (node);
}

} // namespace ns3
