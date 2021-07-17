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

#ifndef LINK_SHARING_H
#define LINK_SHARING_H

#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>
#include "../slices/slice-controller.h"
#include "../metadata/link-info.h"

namespace ns3 {

class TransportController;
class LinkInfo;

/**
 * \ingroup uni5onMano
 * The inter-slice link sharing application for transport network.
 */
class LinkSharing : public Object
{
  friend class TransportController;
  friend class RingController;

public:
  /**
   * Enumeration of available link sharing operation modes.
   */
  typedef enum
  {
    NONE = 0,   //!< Disabled meters.
    SHAR = 1,   //!< Non-GBR aggregate meter.
    STAT = 2,   //!< Non-GBR individual meters.
    DYNA = 3    //!< Non-GBR dynamic individual meters.
  } ShareMode;

  // Total number of valid ShareMode items.
  #define N_SLICE_MODES (static_cast<int> (ShareMode::DYNA) + 1)

  /**
   * Complete constructor
   * \param transpCtrl The transport controller.
   */
  LinkSharing (Ptr<TransportController> transpCtrl);
  virtual ~LinkSharing ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Attribute accessors.
   * \return The requested information.
   */
  //\{
  DataRate  GetExtraStep          (void) const;
  DataRate  GetGuardStep          (void) const;
  ShareMode GetSharingMode        (void) const;
  OpMode    GetSpareUse           (void) const;
  Time      GetTimeout            (void) const;
  //\}

  /**
   * \ingroup uni5on
   * Get the inter-slicing operation mode name.
   * \param mode The inter-slicing operation mode.
   * \return The string with the inter-slicing operation mode name.
   */
  static std::string ShareModeStr (ShareMode mode);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /**
   * Get the list of slice controller applications.
   * \param sharing Filter controller applications with enabled sharing flag.
   * \return The list of controller applications.
   */
  const SliceControllerList_t& GetSliceControllerList (
    bool sharing = false) const;

private:
  /**
   * Adjust the link sharing extra bit rate.
   * \param lInfo The link information.
   * \param dir The link direction.
   */
  void DynamicExtraAdjust (Ptr<LinkInfo> lInfo, LinkInfo::LinkDir dir);

  /**
   * Periodically triggers the link sharing extra bit rate adjustment for
   * transport network links.
   */
  void DynamicTimeout (void);

  /**
   * Notify this application of a successful handshake between a transport
   * switch and the transport controller.
   * \param swDpId The transport switch datapath ID.
   */
  void NotifyHandshakeSuccessful (uint64_t swDpId);

  /**
   * Notify this application that all the logical slices have already been
   * configured and the slice controllers were created.
   * \param controllers The logical slice controllers.
   */
  void NotifySlicesBuilt (ApplicationContainer &controllers);

  /**
   * Adjust the link sharing OpenFlow meters
   * \param lInfo The link information.
   * \param slice The network slice.
   */
  void MeterAdjust (Ptr<LinkInfo> lInfo, SliceId slice);

  /**
   * Apply the link sharing OpenFlow meters
   * \param swDpId The transport switch datapath id.
   * \param slice The network slice.
   */
  void MeterApply (uint64_t swDpId, SliceId slice);

  /**
   * Install the link sharing OpenFlow meters
   * \param lInfo The link information.
   * \param slice The network slice.
   */
  void MeterInstall (Ptr<LinkInfo> lInfo, SliceId slice);

  Ptr<TransportController>  m_controller;   //!< Transport controller.
  DataRate                  m_extraStep;    //!< Extra adjustment step.
  DataRate                  m_guardStep;    //!< Dynamic slice link guard.
  ShareMode                 m_sharingMode;  //!< Link sharing operation mode.
  OpMode                    m_spareUse;     //!< Spare bit rate sharing mode.
  Time                      m_timeout;      //!< Dynamic slice timeout interval.

  /** Slice controllers sorted by increasing priority. */
  SliceControllerList_t m_sliceCtrlsAll;

  /** Slice controllers with enabled sharing sorted by increasing priority. */
  SliceControllerList_t m_sliceCtrlsSha;
};

} // namespace ns3
#endif // LINK_SHARING_H
