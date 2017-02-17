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

#include "sdran-cloud-container.h"
#include <ns3/names.h>

namespace ns3 {

SdranCloudContainer::SdranCloudContainer ()
{
}

SdranCloudContainer::SdranCloudContainer (Ptr<SdranCloud> sdranCloud)
{
  m_objects.push_back (sdranCloud);
}

SdranCloudContainer::SdranCloudContainer (std::string sdranCloudName)
{
  Ptr<SdranCloud> sdranCloud = Names::Find<SdranCloud> (sdranCloudName);
  m_objects.push_back (sdranCloud);
}

SdranCloudContainer::SdranCloudContainer (const SdranCloudContainer &a,
                                          const SdranCloudContainer &b)
{
  Add (a);
  Add (b);
}

SdranCloudContainer::Iterator
SdranCloudContainer::Begin (void) const
{
  return m_objects.begin ();
}

SdranCloudContainer::Iterator
SdranCloudContainer::End (void) const
{
  return m_objects.end ();
}

uint32_t
SdranCloudContainer::GetN (void) const
{
  return m_objects.size ();
}

Ptr<SdranCloud>
SdranCloudContainer::Get (uint32_t i) const
{
  return m_objects[i];
}

void
SdranCloudContainer::Create (uint32_t n)
{
  for (uint32_t i = 0; i < n; i++)
    {
      m_objects.push_back (CreateObject<SdranCloud> ());
    }
}

void
SdranCloudContainer::Add (SdranCloudContainer other)
{
  for (Iterator i = other.Begin (); i != other.End (); i++)
    {
      m_objects.push_back (*i);
    }
}

void
SdranCloudContainer::Add (Ptr<SdranCloud> sdranCloud)
{
  m_objects.push_back (sdranCloud);
}

void
SdranCloudContainer::Add (std::string sdranCloudName)
{
  Ptr<SdranCloud> sdranCloud = Names::Find<SdranCloud> (sdranCloudName);
  m_objects.push_back (sdranCloud);
}

} // namespace ns3
