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

#ifndef SDMN_SERVER_APP_H
#define SDMN_SERVER_APP_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/epc-tft.h"
#include "ns3/eps-bearer.h"
#include "ns3/traced-callback.h"
#include "qos-stats-calculator.h"
#include "ns3/string.h"

namespace ns3 {

/**
 * \ingroup applications
 * This class extends the Application class to proper work with SDMN
 * architecture. Only server applications (those which will be installed into
 * web server node) extends this class.
 */
class SdmnServerApp : public Application
{
public:
  SdmnServerApp ();            //!< Default constructor
  virtual ~SdmnServerApp ();   //!< Dummy destructor, see DoDipose

  /**
   * Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

protected:
  /** Destructor implementation */
  virtual void DoDispose (void);
};

} // namespace ns3
#endif /* SDMN_SERVER_APP_H */
