/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 University of Campinas (Unicamp)
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

#include "svelte-qos-queue.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT \
  std::clog << "[dp " << m_dpId << " port " << m_portNo << "] ";

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SvelteQosQueue");
NS_OBJECT_ENSURE_REGISTERED (SvelteQosQueue);

static ObjectFactory
GetDefaultQueueFactory ()
{
  // Setting default internal queue configuration.
  ObjectFactory queueFactory;
  queueFactory.SetTypeId ("ns3::DropTailQueue<Packet>");
  queueFactory.Set ("MaxSize", StringValue ("100p"));
  return queueFactory;
}

TypeId
SvelteQosQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SvelteQosQueue")
    .SetParent<OFSwitch13Queue> ()
    .AddConstructor<SvelteQosQueue> ()
    .AddAttribute ("NumQueues",
                   "The number of internal priority queues.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (3),
                   MakeUintegerAccessor (
                     &SvelteQosQueue::m_numQueues),
                   MakeUintegerChecker<int> (1, NETDEV_MAX_QUEUES))
    .AddAttribute ("QueueFactory",
                   "The object factory for internal priority queues.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   ObjectFactoryValue (GetDefaultQueueFactory ()),
                   MakeObjectFactoryAccessor (
                     &SvelteQosQueue::m_facQueues),
                   MakeObjectFactoryChecker ())
  ;
  return tid;
}

SvelteQosQueue::SvelteQosQueue ()
  : OFSwitch13Queue (),
  NS_LOG_TEMPLATE_DEFINE ("SvelteQosQueue")
{
  NS_LOG_FUNCTION (this);
}

SvelteQosQueue::~SvelteQosQueue ()
{
  NS_LOG_FUNCTION (this);
}

Ptr<Packet>
SvelteQosQueue::Dequeue (void)
{
  NS_LOG_FUNCTION (this);

  int queueId = GetNonEmptyQueue ();
  if (queueId >= 0)
    {
      NS_LOG_DEBUG ("Packet to be dequeued from queue " << queueId);
      Ptr<Packet> packet = GetQueue (queueId)->Dequeue ();
      NotifyDequeue (packet);
      return packet;
    }

  NS_LOG_DEBUG ("Queue empty");
  return 0;
}

Ptr<Packet>
SvelteQosQueue::Remove (void)
{
  NS_LOG_FUNCTION (this);

  int queueId = GetNonEmptyQueue ();
  if (queueId >= 0)
    {
      NS_LOG_DEBUG ("Packet to be removed from queue " << queueId);
      Ptr<Packet> packet = GetQueue (queueId)->Remove ();
      NotifyRemove (packet);
      return packet;
    }

  NS_LOG_DEBUG ("Queue empty");
  return 0;
}

Ptr<const Packet>
SvelteQosQueue::Peek (void) const
{
  NS_LOG_FUNCTION (this);

  int queueId = GetNonEmptyQueue ();
  if (queueId >= 0)
    {
      NS_LOG_DEBUG ("Packet to be peeked from queue " << queueId);
      return GetQueue (queueId)->Peek ();
    }

  NS_LOG_DEBUG ("Queue empty");
  return 0;
}

void
SvelteQosQueue::DoInitialize ()
{
  NS_LOG_FUNCTION (this);

  // Creating the internal priority queues.
  for (int queueId = 0; queueId < m_numQueues; queueId++)
    {
      AddQueue (m_facQueues.Create<Queue<Packet> > ());
    }

  // Chain up.
  OFSwitch13Queue::DoInitialize ();
}

int
SvelteQosQueue::GetNonEmptyQueue (void) const
{
  NS_LOG_FUNCTION (this);

  for (int queueId = 0; queueId < GetNQueues (); queueId++)
    {
      if (GetQueue (queueId)->IsEmpty () == false)
        {
          return queueId;
        }
    }

  NS_LOG_DEBUG ("All internal queues are empty.");
  return -1;
}

} // namespace ns3
