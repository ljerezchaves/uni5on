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

#ifndef ROUTING_INFO_H
#define ROUTING_INFO_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>

namespace ns3 {

class OpenFlowEpcController;
class RingController;

/** EPS context bearer */
typedef EpcS11SapMme::BearerContextCreated ContextBearer_t;

/** List of created context bearers */
typedef std::list<ContextBearer_t> BearerList_t;


// ------------------------------------------------------------------------ //
/**
 * \ingroup epcof
 * Metadata associated to a routing path between
 * two any switches in the OpenFlow network.
 */
class RoutingInfo : public Object
{
  friend class OpenFlowEpcController;
  friend class RingController;

public:
  RoutingInfo ();          //!< Default constructor
  virtual ~RoutingInfo (); //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private member accessors.
   * \return The requested field.
   */
  //\{
  GbrQosInformation GetQosInfo (void) const;
  EpsBearer::Qci GetQciInfo (void) const;
  EpsBearer GetEpsBearer (void) const;
  uint32_t GetTeid (void) const;
  uint64_t GetImsi (void) const;
  uint16_t GetCellId (void) const;
  uint16_t GetEnbSwIdx (void) const;
  uint16_t GetSgwSwIdx (void) const;
  Ipv4Address GetEnbAddr (void) const;
  Ipv4Address GetSgwAddr (void) const;
  uint16_t GetPriority (void) const;
  uint16_t GetTimeout (void) const;
  bool HasDownlinkTraffic (void) const;
  bool HasUplinkTraffic (void) const;
  bool IsGbr (void) const;
  bool IsDefault (void) const;
  bool IsInstalled (void) const;
  bool IsActive (void) const;
  //\}

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  /**
   * Set the internal installed flag.
   * \param installed The value to set.
   */
  void SetInstalled (bool installed);

  /**
   * Set the internal active flag.
   * \param active The value to set.
   */
  void SetActive (bool active);

  /**
   * Increase the priority value by one unit.
   */
  void IncreasePriority (void);

private:
  uint32_t          m_teid;         //!< GTP TEID
  uint64_t          m_imsi;         //!< UE IMSI
  uint16_t          m_cellId;       //!< eNB cell ID
  uint16_t          m_sgwIdx;       //!< Sgw switch index
  uint16_t          m_enbIdx;       //!< eNB switch index
  Ipv4Address       m_sgwAddr;      //!< Sgw IPv4 address
  Ipv4Address       m_enbAddr;      //!< eNB IPv4 address
  uint16_t          m_priority;     //!< Flow priority
  uint16_t          m_timeout;      //!< Flow idle timeout
  bool              m_isDefault;    //!< This info is for default bearer
  bool              m_isInstalled;  //!< Rule is installed into switches
  bool              m_isActive;     //!< Application traffic is active
  ContextBearer_t   m_bearer;       //!< EPS bearer information
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup epcof
 * Metadata associated to GTP tunnel meter rules.
 */
class MeterInfo : public Object
{
  friend class OpenFlowEpcController;
  friend class RingController;

public:
  MeterInfo ();          //!< Default constructor
  virtual ~MeterInfo (); //!< Dummy destructor, see DoDipose

  /**
   * Complete constructor.
   * \param rInfo RoutingInfo pointer.
   * \attention This MeterInfo object must be aggregated to rInfo.
   */
  MeterInfo (Ptr<RoutingInfo> rInfo);

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private member accessors.
   * \return The requested field.
   */
  //\{
  bool IsInstalled (void) const;
  bool HasDown (void) const;
  bool HasUp (void) const;
  //\}

  /**
   * \name Dpctl commands to add or delete meter rules
   * \return The requested command.
   */
  //\{
  std::string GetDownAddCmd (void) const;
  std::string GetUpAddCmd (void) const;
  std::string GetDelCmd (void) const;
  //\}

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  /** \return RoutingInfo pointer. */
  Ptr<RoutingInfo> GetRoutingInfo ();

  /**
   * Set the internal installed flag.
   * \param installed The value to set.
   */
  void SetInstalled (bool installed);

private:
  uint32_t m_teid;          //!< GTP TEID
  bool     m_isInstalled;   //!< True when this meter is installed
  bool     m_hasDown;       //!< True for downlink meter
  bool     m_hasUp;         //!< True for uplink meter
  uint64_t m_downBitRate;   //!< Downlink meter drop bit rate (bps)
  uint64_t m_upBitRate;     //!< Uplink meter drop bit rate (bps)
  Ptr<RoutingInfo> m_rInfo; //!< Routing information
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup epcof
 * Metadata associated to GBR beares.
 */
class GbrInfo : public Object
{
  friend class OpenFlowEpcController;
  friend class RingController;

public:
  GbrInfo ();          //!< Default constructor
  virtual ~GbrInfo (); //!< Dummy destructor, see DoDipose

  /**
   * Complete constructor.
   * \param rInfo RoutingInfo pointer.
   * \attention This GbrInfo object must be aggregated to rInfo.
   */
  GbrInfo (Ptr<RoutingInfo> rInfo);

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private member accessors.
   * \return The requested field.
   */
  //\{
  uint8_t GetDscp (void) const;
  uint64_t GetDownBitRate (void) const;
  uint64_t GetUpBitRate (void) const;
  bool IsReserved (void) const;
  //\}

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  /** \return RoutingInfo pointer. */
  Ptr<RoutingInfo> GetRoutingInfo ();

  /**
   * Set the internal reserved flag.
   * \param reserved The value to set.
   */
  void SetReserved (bool reserved);

private:
  uint32_t m_teid;          //!< GTP TEID
  uint8_t  m_dscp;          //!< DiffServ DSCP value for this bearer
  bool     m_isReserved;    //!< True when resources are reserved
  bool     m_hasDown;       //!< True for downlink reserve
  bool     m_hasUp;         //!< True for uplink reserve
  uint64_t m_downBitRate;   //!< Downlink reserved bit rate
  uint64_t m_upBitRate;     //!< Uplink reserved bit rate
  Ptr<RoutingInfo> m_rInfo; //!< Routing information
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup epcof
 * Metadata associated to a ring routing path between
 * two any switches in the OpenFlow ring network.
 */
class RingRoutingInfo : public Object
{
  friend class RingController;

public:
  /** Routing direction in the ring. */
  enum RoutingPath
  {
    CLOCK = 1,
    COUNTER = 2
  };

  RingRoutingInfo ();          //!< Default constructor
  virtual ~RingRoutingInfo (); //!< Dummy destructor, see DoDipose

  /**
   * Complete constructor.
   * \param rInfo RoutingInfo pointer.
   * \param downPath The _shortest_ path for downlink (uplink will get the
   * inverse path).
   * \attention This RingRoutingInfo object must be aggregated to rInfo.
   */
  RingRoutingInfo (Ptr<RoutingInfo> rInfo, RoutingPath shortDownPath);

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private member accessors.
   * \return The requested field.
   */
  //\{
  bool IsInverted (void) const;
  uint16_t GetSgwSwIdx (void) const;
  uint16_t GetEnbSwIdx (void) const;
  RoutingPath GetDownPath (void) const;
  RoutingPath GetUpPath (void) const;
  std::string GetPathDesc (void) const;
  //\}

  /**
   * Invert the routing path
   * \param path The original routing path.
   * \return The inverse routing path.
   */
  static RoutingPath InvertPath (RoutingPath path);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  /** \return RoutingInfo pointer. */
  Ptr<RoutingInfo> GetRoutingInfo ();

private:
  /**
   * \name Path invertion methods.
   */
  //\{
  void InvertPaths ();
  void ResetToShortestPaths ();
  //\}

  Ptr<RoutingInfo> m_rInfo;       //!< Routing information
  RoutingPath      m_downPath;    //!< Downlink routing path
  RoutingPath      m_upPath;      //!< Uplink routing path
  bool             m_isInverted;  //!< True when paths are inverted
};

};  // namespace ns3
#endif // ROUTING_INFO_H

