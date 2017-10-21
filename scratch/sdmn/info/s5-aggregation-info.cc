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

#include "routing-info.h"
#include "s5-aggregation-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("S5AggregationInfo");
NS_OBJECT_ENSURE_REGISTERED (S5AggregationInfo);

S5AggregationInfo::S5AggregationInfo (Ptr<RoutingInfo> rInfo)
  : m_mode (OperationMode::OFF),
    m_threshhold (0.0),
    m_usage (0.0)
{
  NS_LOG_FUNCTION (this);

  AggregateObject (rInfo);
}

S5AggregationInfo::~S5AggregationInfo ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
S5AggregationInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::S5AggregationInfo")
    .SetParent<Object> ()
  ;
  return tid;
}

OperationMode
S5AggregationInfo::GetOperationMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_mode;
}

std::string
S5AggregationInfo::GetOperationModeStr (void) const
{
  NS_LOG_FUNCTION (this);

  return OperationModeStr (m_mode);
}

double
S5AggregationInfo::GetSliceUsage (void) const
{
  NS_LOG_FUNCTION (this);

  return m_usage;
}

double
S5AggregationInfo::GetThreshold (void) const
{
  NS_LOG_FUNCTION (this);

  return m_threshhold;
}

void
S5AggregationInfo::SetOperationMode (OperationMode value)
{
  NS_LOG_FUNCTION (this << value);

  m_mode = value;
}

void
S5AggregationInfo::SetSliceUsage (double value)
{
  NS_LOG_FUNCTION (this << value);

  m_usage = value;
}

void
S5AggregationInfo::SetThreshold (double value)
{
  NS_LOG_FUNCTION (this << value);

  m_threshhold = value;
}

bool
S5AggregationInfo::IsAggregated (void) const
{
  NS_LOG_FUNCTION (this);

  if ((m_mode == OperationMode::AUTO && m_usage > m_threshhold)
      || m_mode == OperationMode::OFF)
    {
      return false;
    }
  else
    {
      return true;
    }
}

void
S5AggregationInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

};  // namespace ns3
