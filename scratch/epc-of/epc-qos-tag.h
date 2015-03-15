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

#ifndef EPC_QOS_TAG_H
#define EPC_QOS_TAG_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
   
namespace ns3 {

class Tag;

/** 
 * Tag used to measure traffic QoS over the OpenFlow LTE EPC network. 
 */
class EpcQosTag : public Tag
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  /** Constructors */
  EpcQosTag ();
  EpcQosTag (uint32_t seq, uint32_t teid);

  // Inherited from Tag
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual uint32_t GetSerializedSize () const;
  virtual void Print (std::ostream &os) const;
 
  /** \return the teid field */
  uint32_t GetTeid () const;
  
  /** \return the sequence number field */
  uint32_t GetSeqNum () const;

  /** \return the timestamp field */
  Time GetTimestamp () const;

private:
  uint64_t m_ts;    //!< Creating timestamp
  uint32_t m_seq;   //!< Packet sequence number
  uint32_t m_teid;  //!< GTP teid
};

};  // namespace ns3
#endif // EPC_QOS_TAG_H

