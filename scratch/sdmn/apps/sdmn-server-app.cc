/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 University of Campinas (Unicamp)
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

#include "sdmn-server-app.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SdmnServerApp");
NS_OBJECT_ENSURE_REGISTERED (SdmnServerApp);

SdmnServerApp::SdmnServerApp ()
{
  NS_LOG_FUNCTION (this);
}

SdmnServerApp::~SdmnServerApp ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SdmnServerApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SdmnServerApp")
    .SetParent<Application> ()
    .AddConstructor<SdmnServerApp> ()
  ;
  return tid;
}

void
SdmnServerApp::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

} // namespace ns3
