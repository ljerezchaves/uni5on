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

#include "epc-qos-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpcQosTag");
NS_OBJECT_ENSURE_REGISTERED (EpcQosTag);

EpcQosTag::EpcQosTag ()
  : m_ts (Simulator::Now ().GetTimeStep ())
{
}

EpcQosTag::EpcQosTag (uint32_t seq, uint32_t teid)
  : m_ts (Simulator::Now ().GetTimeStep ()),
    m_seq (seq),
    m_teid (teid)
{
}

TypeId 
EpcQosTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcQosTag")
    .SetParent<Tag> ()
    .AddConstructor<EpcQosTag> ()
  ;
  return tid;
}

TypeId 
EpcQosTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t 
EpcQosTag::GetSerializedSize (void) const
{
  return 16;
}

void 
EpcQosTag::Serialize (TagBuffer i) const
{
  i.WriteU64 (m_ts);
  i.WriteU32 (m_seq);
  i.WriteU32 (m_teid);
}

void 
EpcQosTag::Deserialize (TagBuffer i)
{
  m_ts = i.ReadU64 ();
  m_seq = i.ReadU32 ();
  m_teid = i.ReadU32 ();
}

void 
EpcQosTag::Print (std::ostream &os) const
{
  os << " EpcQosTag teid=" << m_teid << " seq=" << m_seq << " ts=" << m_ts;
}

uint32_t 
EpcQosTag::GetTeid () const
{
  return m_teid;
}

uint32_t 
EpcQosTag::GetSeqNum () const
{
  return m_seq;
}

Time 
EpcQosTag::GetTimestamp () const
{
  return Time (m_ts);
}

};  // namespace ns3
