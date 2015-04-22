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
 * Metadata associated to LTE context information for controller usage. 
 */
class ContextInfo : public Object
{
  friend class OpenFlowEpcController;

public:
  ContextInfo ();          //!< Default constructor
  virtual ~ContextInfo (); //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  virtual void DoDispose ();

  /** Const private member access methods. */
  //\{
  uint16_t GetEnbIdx () const;      //!< \return eNB switch index
  uint16_t GetSgwIdx () const;      //!< \return Gateway switch index
  Ipv4Address GetEnbAddr () const;  //!< \return eNB Ipv4 address
  Ipv4Address GetSgwAddr () const;  //!< \return Gateway Ipv4 address
  //\}

private:
  uint64_t      m_imsi;         //!< UE IMSI
  uint16_t      m_cellId;       //!< eNB Cell ID
  uint16_t      m_enbIdx;       //!< eNB switch index
  uint16_t      m_sgwIdx;       //!< Gateway switch index
  Ipv4Address   m_enbAddr;      //!< eNB IPv4 addr
  Ipv4Address   m_sgwAddr;      //!< Gateway IPv4 addr
  BearerList_t  m_bearerList;   //!< List of bearers
};


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

  /** Destructor implementation */
  virtual void DoDispose ();

  /** \return True if the associated EPS bearer is of GBR type. */
  bool IsGbr ();

  /** \return The EPS Bearer. */
  EpsBearer GetEpsBearer ();

  /** \return The Bearer QoS information. */
  GbrQosInformation GetQosInfo ();
  
private:
  uint32_t          m_teid;         //!< GTP TEID
  uint16_t          m_sgwIdx;       //!< Sgw switch index
  uint16_t          m_enbIdx;       //!< eNB switch index
  Ipv4Address       m_sgwAddr;      //!< Sgw IPv4 address
  Ipv4Address       m_enbAddr;      //!< eNB IPv4 address
  Ptr<Application>  m_app;          //!< Traffic application
  int               m_priority;     //!< Flow priority
  int               m_timeout;      //!< Flow idle timeout
  bool              m_isDefault;    //!< This info is for default bearer
  bool              m_isInstalled;  //!< Rule is installed into switches
  bool              m_isActive;     //!< Application traffic is active
  ContextBearer_t   m_bearer;       //!< EPS bearer information
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup epcof
 * Metadata associated to meter rules.
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

  /** Destructor implementation */
  virtual void DoDispose ();

  /** \return RoutingInfo pointer. */
  Ptr<RoutingInfo> GetRoutingInfo ();

  /** Get Dpctl commands to add or delete meter rules */
  //\{
  std::string GetDownAddCmd ();
  std::string GetUpAddCmd ();
  std::string GetDelCmd ();
  //\}

private:
  uint32_t m_teid;          //!< GTP TEID
  bool     m_isInstalled;   //!< True when this meter is installed
  bool     m_hasDown;       //!< True for downlink meter
  bool     m_hasUp;         //!< True for uplink meter
  DataRate m_downDataRate;  //!< Downlink meter drop rate (bps)
  DataRate m_upDataRate;    //!< Uplink meter drop rate (bps)
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

  /** Destructor implementation */
  virtual void DoDispose ();

  /** \return RoutingInfo pointer. */
  Ptr<RoutingInfo> GetRoutingInfo ();

private:
  uint32_t m_teid;          //!< GTP TEID
  bool     m_isReserved;    //!< True when the resources are reserved
  bool     m_hasDown;       //!< True for downlink gbr
  bool     m_hasUp;         //!< True for uplink gbr
  DataRate m_downDataRate;  //!< Downlink guaranteed data rate
  DataRate m_upDataRate;    //!< Uplink guaranteed data rate
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
  enum RoutingPath {
    CLOCK = 1,
    COUNTER = 2
  };

  RingRoutingInfo ();          //!< Default constructor
  virtual ~RingRoutingInfo (); //!< Dummy destructor, see DoDipose

  /** 
   * Complete constructor.
   * \param rInfo RoutingInfo pointer. 
   * \param downPath The path for downlink (uplink will get the inverse path).
   * \attention This RingRoutingInfo object must be aggregated to rInfo.
   */
  RingRoutingInfo (Ptr<RoutingInfo> rInfo, RoutingPath downPath);
  
  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Invert the routing path
   * \param path The original routing path.
   * \return The inverse routing path.
   */
  static RoutingPath InvertPath (RoutingPath path);

  /** Destructor implementation */
  virtual void DoDispose ();

  /** \return RoutingInfo pointer. */
  Ptr<RoutingInfo> GetRoutingInfo ();

protected:
  /** Invert down/up routing direction. */
  void InvertDownPath ();
  void InvertUpPath ();
  void ResetPaths ();

  Ptr<RoutingInfo> m_rInfo;     //!< Routing information
  RoutingPath      m_downPath;  //!< Downlink routing path
  RoutingPath      m_upPath;    //!< Uplink routing path
  bool             m_isDownInv; //!< True when down path is inverted
  bool             m_isUpInv;   //!< True when up path is inverted
};

};  // namespace ns3
#endif // ROUTING_INFO_H

