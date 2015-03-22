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

#include "seq-num-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SeqNumTag");
NS_OBJECT_ENSURE_REGISTERED (SeqNumTag);

SeqNumTag::SeqNumTag ()
  : m_seq (0)
{
}

SeqNumTag::SeqNumTag (uint32_t seq)
  : m_seq (seq)
{
}

TypeId 
SeqNumTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SeqNumTag")
    .SetParent<Tag> ()
    .AddConstructor<SeqNumTag> ()
  ;
  return tid;
}

TypeId 
SeqNumTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t 
SeqNumTag::GetSerializedSize (void) const
{
  return 4;
}

void 
SeqNumTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_seq);
}

void 
SeqNumTag::Deserialize (TagBuffer i)
{
  m_seq = i.ReadU32 ();
}

void 
SeqNumTag::Print (std::ostream &os) const
{
  os << " SeqNumTag seq=" << m_seq;
}

uint32_t 
SeqNumTag::GetSeqNum () const
{
  return m_seq;
}

};  // namespace ns3
