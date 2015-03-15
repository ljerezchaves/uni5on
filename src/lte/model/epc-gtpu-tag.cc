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
  : m_teid (0)
{
}

EpcGtpuTag::EpcGtpuTag (uint32_t teid)
  : m_teid (teid)
{
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
  return 4;
}

void 
EpcGtpuTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_teid);
}

void 
EpcGtpuTag::Deserialize (TagBuffer i)
{
  m_teid = i.ReadU32 ();
}

void 
EpcGtpuTag::Print (std::ostream &os) const
{
  os << " TEID=" << m_teid;
}

uint32_t 
EpcGtpuTag::GetTeid () const
{
  return m_teid;
}

void 
EpcGtpuTag::SetTeid (uint32_t teid)
{
  m_teid = teid;
}

};  // namespace ns3
