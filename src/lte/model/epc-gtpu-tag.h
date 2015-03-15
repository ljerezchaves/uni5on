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

#ifndef EPC_GTPU_TAG_H
#define EPC_GTPU_TAG_H

#include "ns3/tag.h"

namespace ns3 {

class Tag;

/** 
 * Tag used to identify the GTP TEID for packets. 
 */
class EpcGtpuTag : public Tag
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  /** Constructors */
  EpcGtpuTag ();
  EpcGtpuTag (uint32_t teid);

  // Inherited from Tag
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual uint32_t GetSerializedSize () const;
  virtual void Print (std::ostream &os) const;
 
  /** \return the teid field */
  uint32_t GetTeid () const;
  
  /** \return the sequence number field */
  void SetTeid (uint32_t teid);

private:
  uint32_t m_teid;  //!< GTP teid
};

};  // namespace ns3
#endif // EPC_GTPU_TAG_H

