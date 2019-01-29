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

#ifndef SVELTE_QOS_QUEUE_H
#define SVELTE_QOS_QUEUE_H

#include <ns3/ofswitch13-module.h>

namespace ns3 {

/**
 * \ingroup svelteInfra
 * This class implements the specialized QoS queue for the SVELTE architecture.
 */
class SvelteQosQueue : public OFSwitch13Queue
{
public:
  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  SvelteQosQueue ();           //!< Default constructor.
  virtual ~SvelteQosQueue ();  //!< Dummy destructor, see DoDispose.

  // Inherited from Queue.
  Ptr<Packet> Dequeue (void);
  Ptr<Packet> Remove (void);
  Ptr<const Packet> Peek (void) const;

protected:
  // Inherited from Object.
  virtual void DoInitialize (void);

private:
  /**
   * Identify the highest-priority non-empty queue.
   * \return The queue ID.
   */
  int GetNonEmptyQueue (void) const;

  ObjectFactory         m_facQueues;  //!< Factory for internal queues.
  int                   m_numQueues;  //!< Number of internal queues.

  NS_LOG_TEMPLATE_DECLARE;            //!< Redefinition of the log component.
};

} // namespace ns3
#endif /* SVELTE_QOS_QUEUE_H */
