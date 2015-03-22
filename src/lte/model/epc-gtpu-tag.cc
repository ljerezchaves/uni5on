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

EpcGtpuTag::EpcGtpuTag (uint32_t teid, EpcInputNode inputNode)
  : m_teid (teid)
{
  m_inputNode = (uint8_t)inputNode;
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
  return 5;
}

void 
EpcGtpuTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_teid);
  i.WriteU8 (m_inputNode);
}

void 
EpcGtpuTag::Deserialize (TagBuffer i)
{
  m_teid = i.ReadU32 ();
  m_inputNode = i.ReadU8 ();
}

void 
EpcGtpuTag::Print (std::ostream &os) const
{
  os << " TEID=" << m_teid 
     << " input=" << (m_inputNode == (uint8_t)EpcGtpuTag::ENB ? "eNb" : "Pgw");
}

uint32_t 
EpcGtpuTag::GetTeid () const
{
  return m_teid;
}

EpcGtpuTag::EpcInputNode 
EpcGtpuTag::GetInputNode () const
{
  return m_inputNode == 0 ? EpcGtpuTag::ENB : EpcGtpuTag::PGW;
}

void 
EpcGtpuTag::SetTeid (uint32_t teid)
{
  m_teid = teid;
}

void 
EpcGtpuTag::SetInputNode (EpcInputNode inputNode)
{
  m_inputNode = (uint8_t)inputNode;
}

bool
EpcGtpuTag::IsDownlink () const
{
  return (bool)m_inputNode;
}

bool
EpcGtpuTag::IsUplink () const
{
  return (bool)!m_inputNode;
}

};  // namespace ns3
