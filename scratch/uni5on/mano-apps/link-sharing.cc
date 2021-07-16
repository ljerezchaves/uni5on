/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 University of Campinas (Unicamp)
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

#include "link-sharing.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LinkSharing");
NS_OBJECT_ENSURE_REGISTERED (LinkSharing);

LinkSharing::LinkSharing ()
{
  NS_LOG_FUNCTION (this);
}

LinkSharing::~LinkSharing ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
LinkSharing::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LinkSharing")
    .SetParent<Object> ()
  ;
  return tid;
}

void
LinkSharing::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  Object::DoDispose ();
}

void
LinkSharing::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  Object::NotifyConstructionCompleted ();
}

} // namespace ns3
