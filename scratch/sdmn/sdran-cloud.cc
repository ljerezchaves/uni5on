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

#include "sdran-cloud.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SdranCloud");
NS_OBJECT_ENSURE_REGISTERED (SdranCloud);

// Initializing global counters.
uint32_t SdranCloud::m_enbCounter = 0;
uint32_t SdranCloud::m_sdranCounter = 0;

SdranCloud::SdranCloud ()
{
  NS_LOG_FUNCTION (this);

  // Set SDRAN Cloud ID.
  m_sdranId = ++m_sdranCounter;

  // Create the S-GW node and set its name.
  m_sgwNode = CreateObject<Node> ();
  std::string sgwName = "sgw" + m_sdranId;
  Names::Add (sgwName, m_sgwNode);
}

SdranCloud::~SdranCloud ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SdranCloud::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SdranCloud")
    .SetParent<Object> ()
    .AddConstructor<SdranCloud> ()
    .AddAttribute ("NumSites", "The total number of cell sites managed by "
                   "this SDRAN cloud (each site has 3 eNBs).",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1),
                   MakeUintegerAccessor (&SdranCloud::m_nSites),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

uint32_t
SdranCloud::GetId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sdranId;
}

Ptr<Node>
SdranCloud::GetSgwNode ()
{
  NS_LOG_FUNCTION (this);

  return m_sgwNode;
}

NodeContainer
SdranCloud::GetEnbNodes (void) const
{
  NS_LOG_FUNCTION (this);

  return m_enbNodes;
}

void
SdranCloud::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_sgwNode = 0;
}

void
SdranCloud::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("SDRAN cloud with " << m_nSites << " cell sites.");

  // Set the number of eNBs based on the number of cell sites.
  m_nEnbs = 3 * m_nSites;

  // Create the eNBs nodes and set their names.
  m_enbNodes.Create (m_nEnbs);
  NodeContainer::Iterator it;
  for (it = m_enbNodes.Begin (); it != m_enbNodes.End (); ++it)
    {
      std::ostringstream enbName;
      enbName << "enb" << ++m_enbCounter;
      Names::Add (enbName.str (), *it);
    }

  // Set the constant mobility model for eNB positioning
  MobilityHelper mobilityHelper;
  mobilityHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityHelper.Install (m_enbNodes);

  // Chain up
  Object::NotifyConstructionCompleted ();
}

} // namespace ns3
