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

#ifndef SEQ_NUM_TAG_H
#define SEQ_NUM_TAG_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"

namespace ns3 {

class Tag;

/**
 * \ingroup sdmn
 * Tag used to save packet sequence number.
 */
class SeqNumTag : public Tag
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  /** Constructors */
  SeqNumTag ();
  SeqNumTag (uint32_t seq);

  // Inherited from Tag
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual uint32_t GetSerializedSize () const;
  virtual void Print (std::ostream &os) const;

  /** \return the sequence number field */
  uint32_t GetSeqNum () const;

private:
  uint32_t m_seq;   //!< Packet sequence number
};

};  // namespace ns3
#endif // SEQ_NUM_TAG_H

