/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 University of Campinas (Unicamp)
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

#ifndef OPENFLOW_EPC_NETWORK_H
#define OPENFLOW_EPC_NETWORK_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/csma-module.h>
#include <ns3/ofswitch13-module.h>
#include <ns3/qos-stats-calculator.h>

namespace ns3 {
  
/** A pair of switches index */
typedef std::pair<uint16_t, uint16_t> SwitchPair_t;

/** Bandwitdh stats between two switches */
typedef std::pair<SwitchPair_t, double> BandwidthStats_t;

class OpenFlowEpcController;

/**
 * \ingroup epcof
 * Metadata associated to a connection between 
 * two any switches in the OpenFlow network.
 */
class ConnectionInfo : public Object
{
  friend class OpenFlowEpcNetwork;
  friend class RingNetwork;
  friend class OpenFlowEpcController;
  friend class RingController;

public:
 ConnectionInfo ();          //!< Default constructor
  virtual ~ConnectionInfo (); //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  virtual void DoDispose ();

  /**
   * Reserve some bandwith between these two switches.
   * \param dr The DataRate to reserve.
   * \return True if everything is ok, false otherwise.
   */
  bool ReserveDataRate (DataRate dr);   //!< Reserve bandwitdth

  /**
   * Release some bandwith between these two switches.
   * \param dr The DataRate to release.
   * \return True if everything is ok, false otherwise.
   */
  bool ReleaseDataRate (DataRate dr);

  /**
   * Get the availabe bandwitdh between these two switches.
   * \return True available DataRate.
   */
  DataRate GetAvailableDataRate ();     //!< Get available bandwitdth 

  /**
   * Get the availabe bandwitdh between these two switches, considering a
   * saving reserve factor.
   * \param bwFactor The bandwidth saving factor.
   * \return True available DataRate.
   */
  DataRate GetAvailableDataRate (double bwFactor);

  /**
   * Return the bandwidth usage ratio, ignoring the saving reserve factor.
   * \return The usage ratio.
   */
  double GetUsageRatio (void) const;

protected:
  /** Information associated to the first switch */
  //\{
  uint16_t switchIdx1;                  //!< Switch index
  Ptr<OFSwitch13NetDevice> switchDev1;  //!< OpenFlow device
  Ptr<CsmaNetDevice> portDev1;          //!< OpenFlow csma port device
  uint32_t portNum1;                    //!< OpenFlow port number
  //\}

  /** Information associated to the second switch */
  //\{
  uint16_t switchIdx2;                  //!< Switch index
  Ptr<OFSwitch13NetDevice> switchDev2;  //!< OpenFlow device
  Ptr<CsmaNetDevice> portDev2;          //!< OpenFlow csma port device
  uint32_t portNum2;                    //!< OpenFlow port number
  //\}

  /** Information associated to the connection between these two switches */
  //\{
  DataRate maxDataRate;         //!< Maximum nominal bandwidth (half-duplex)
  DataRate reservedDataRate;    //!< Reserved bandwitdth (half-duplex)
  //\}
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup epcof
 * Create an OpenFlow network infrastructure to be used by
 * OpenFlowEpcHelper on LTE networks.
 */
class OpenFlowEpcNetwork : public Object
{

friend class OpenFlowEpcController;

public:
  OpenFlowEpcNetwork ();          //!< Default constructor
  virtual ~OpenFlowEpcNetwork (); //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);
  
  /** Destructor implementation */
  virtual void DoDispose ();

  /**
   * Called by OpenFlowEpcHelper to proper connect the SgwPgw and eNBs to the
   * S1-U OpenFlow network infrastructure.
   * \internal This method must create the NetDevice at node and assign an IPv4
   * address to it.
   * \param node The SgwPgw or eNB node pointer.
   * \param cellId The eNB cell ID.
   * \return A pointer to the NetDevice created at node.
   */
  virtual Ptr<NetDevice> AttachToS1u (Ptr<Node> node, uint16_t cellId) = 0;
  
  /**
   * Called by OpenFlowEpcHelper to proper connect the eNBs nodes to the X2
   * OpenFlow network infrastructure.
   * \internal This method must create the NetDevice at node and assign an IPv4
   * address to it.
   * \param node The eNB node pointer.
   * \return A pointer to the NetDevice created at node.
   */
  virtual Ptr<NetDevice> AttachToX2 (Ptr<Node> node) = 0;
  
  /** 
   * Creates the OpenFlow network infrastructure with existing OpenFlow
   * Controller application.
   * \param controller The OpenFlow controller for this EPC OpenFlow network.
   * \param eNbSwitches The switch index for each eNB.
   */
  virtual void CreateTopology (Ptr<OpenFlowEpcController> controller, 
      std::vector<uint16_t> eNbSwitches) = 0;

  /** 
   * Enable pcap on switch data ports.
   * \param prefix The file prefix.
   * \param promiscuous If true, enable promisc trace.
   */
  void EnableDataPcap (std::string prefix, bool promiscuous = false);

  /** 
   * Enable pcap on OpenFlow channel. 
   * \param prefix The file prefix.
   */
  void EnableOpenFlowPcap (std::string prefix);

  /** 
   * Enable ascii trace on OpenFlow channel. 
   * \param prefix The file prefix.
   */
  void EnableOpenFlowAscii (std::string prefix);

  /** 
   * Enable internal ofsoftswitch13 logging.
   * \param level string representing library logging level.
   */
  void EnableDatapathLogs (std::string level);

  /**
   * Set an attribute for ns3::OFSwitch13NetDevice
   * \param n1 the name of the attribute to set.
   * \param v1 the value of the attribute to set.
   */
  void SetSwitchDeviceAttribute (std::string n1, const AttributeValue &v1);

  /** 
   * \return The CsmaHelper used to create the OpenFlow network. 
   */
  CsmaHelper GetCsmaHelper ();

  /** 
   * \return The NodeContainer with all OpenFlow switches nodes. 
   */
  NodeContainer GetSwitchNodes ();

  /** 
   * \return The NetDeviceContainer with all OFSwitch13NetDevice devices. 
   */
  NetDeviceContainer GetSwitchDevices ();

  /** 
   * \return The OpenFlow controller application. 
   */
  Ptr<OFSwitch13Controller> GetControllerApp ();

  /** 
   * \return The OpenFlow controller node. 
   */
  Ptr<Node> GetControllerNode ();
  
  /** 
   * \return Number of switches in the network. 
   */
  uint16_t GetNSwitches ();

  /**
   * Set the default statistics dump interval.
   * \param timeout The timeout value.
   */
  void SetDumpTimeout (Time timeout);

  /**
   * Connect all trace sinks used to monitor the network.
   * \attention This member function register a trace sink for each EPC
   * application (eNB and SgwPgw). Then, it must be called after scenario
   * topology creation.
   */
  void ConnectTraceSinks ();


  /**
   * Trace sink for packets dropped by meter bands. The tag will be read
   * from packet, and QoS stats updated.
   * \param context Output switch index.
   * \param packet The dropped packet.
   */
  void MeterDropPacket (std::string context, Ptr<const Packet> packet);

  /**
   * Trace sink for packets dropped by queues. 
   * \param context The queue context location.
   * \param packet The dropped packet.
   */
  void QueueDropPacket (std::string context, Ptr<const Packet> packet);
  
  /**
   * Dump EPC Pgw downlink/uplink traffic statistics.
   */
  void DumpPgwStatistics ();
  
  /**
   * Dump Opwnflow switch table statistics.
   */
  void DumpSwtStatistics ();

  /**
   * Dump network bandwidth usage.
   */
  void DumpBwdStatistics ();

  /**
   * Dump epc traffic statistics.
   * \param teid The application bearer teid.
   * \param desc The application description string.
   * \param uplink Dump statistics for uplink too.
   */
  void DumpEpcStatistics (uint32_t teid, std::string desc, 
                          bool uplink = false);

  /**
   * Reset epc traffic statistics.
   * \param teid The application bearer teid.
   */ 
  void ResetEpcStatistics (uint32_t teid);

  /** 
   * TracedCallback signature for QoS dump. 
   * \param description String describing this traffic.
   * \param teid Bearer TEID.
   * \param stats The QoS statistics.
   */
  typedef void (* QosTracedCallback)(std::string description, uint32_t teid, 
                                     Ptr<const QosStatsCalculator> stats);

  /** 
   * TracedCallback signature for Pgw traffic statistics.
   * \param downTraffic The average downlink traffic for last interval.
   * \param upTraffic The average uplink traffic for last interval.
   */
  typedef void (* PgwTracedCallback)(DataRate downTraffic, DataRate upTraffic);

  /** 
   * TracedCallback signature for switch Flow table rules statistics.
   * \param teid The number of TEID routing flow rules at each switch.
   */
  typedef void (* SwtTracedCallback)(std::vector<uint32_t> teid);
  
  /** 
   * TracedCallback signature for bandwidth usage statistics.
   * \param stats List of links and usage ratio.
   */
  typedef void (* BwdTracedCallback)(std::vector<BandwidthStats_t> stats);
  
protected:
  /**
   * Get the OFSwitch13NetDevice of a specific switch.
   * \param index The switch index.
   * \return The pointer to the switch OFSwitch13NetDevice.
   */
  Ptr<OFSwitch13NetDevice> GetSwitchDevice (uint16_t index);

  /**
   * Store the pair <node, switch index> for further use.
   * \param switchIdx The switch index in m_ofSwitches.
   * \param Ptr<Node> The node pointer.
   */
  void RegisterNodeAtSwitch (uint16_t switchIdx, Ptr<Node> node);

  /**
   * Store the switch index at which the gateway is connected.
   * \param switchIdx The switch index in m_ofSwitches.
   * \param Ptr<Node> The node pointer.
   */
  void RegisterGatewayAtSwitch (uint16_t switchIdx, Ptr<Node> node);

  /**
   * Retrieve the switch index for node pointer.
   * \param node Ptr<Node> The node pointer.
   * \return The switch index in m_ofSwitches.
   */
  uint16_t GetSwitchIdxForNode (Ptr<Node> node);

  /**
   * Retrieve the switch index for switch device.
   * \param dev The OpenFlow device pointer.
   * \return The switch index in m_ofSwitches.
   */
  uint16_t GetSwitchIdxForDevice (Ptr<OFSwitch13NetDevice> dev);

  /**
   * Retrieve the switch index at which the gateway is connected.
   * \return The switch index in m_ofSwitches.
   */
  uint16_t GetGatewaySwitchIdx ();

  /**
   * Retrieve the gateway node pointer.
   * \return The gateway node pointer.
   */
  Ptr<Node> GetGatewayNode ();

  /**
   * Set the OpenFlow controller for this network.
   * \param controller The controller application.
   */ 
  void SetController (Ptr<OpenFlowEpcController> controller);
  
  Ptr<OpenFlowEpcController>  m_ofCtrlApp;      //!< Controller application.
  Ptr<Node>                   m_ofCtrlNode;     //!< Controller node.
  NodeContainer               m_ofSwitches;     //!< Switch nodes.
  NetDeviceContainer          m_ofDevices;      //!< Switch devices.
  Ptr<OFSwitch13Helper>       m_ofHelper;       //!< OpenFlow helper.
  CsmaHelper                  m_ofCsmaHelper;   //!< Csma helper.
  std::vector<uint16_t>       m_eNbSwitchIdx;   //!< Switch index for each eNB.

private:
  /**
   * Trace sink for packets entering the EPC. The packet will get tagged for
   * QoS monitoring.
   * \param context Context information.
   * \param packet The packet.
   */
  void EpcInputPacket (std::string context, Ptr<const Packet> packet);

  /**
   * Trace sink for packets leaving the EPC. The tag will be read from packet,
   * and QoS stats updated.
   * \param context Context information.
   * \param packet The packet.
   */
  void EpcOutputPacket (std::string context, Ptr<const Packet> packet);

  /**
   * Trace sink for packets traversing the EPC packet gateway from/to the
   * Internet to/from the EPC.
   * \param context Context information.
   * \param packet The packet.
   */
  void PgwTraffic (std::string context, Ptr<const Packet> packet);
  
  /**
   * Retrieve the LTE EPC QoS statistics information for the GTP tunnel id.
   * When no information structure available, create it.
   * \param teid The GTP tunnel id.
   * \param isDown True for downlink stats, false for uplink.
   * \return The QoS information.
   */
  Ptr<QosStatsCalculator> GetQosStatsFromTeid (uint32_t teid, bool isDown);

  uint16_t                    m_gatewaySwitch;  //!< Gateway switch index.
  Ptr<Node>                   m_gatewayNode;    //!< Gateway node pointer.
  Time                        m_dumpTimeout;    //!< Dump stats timeout. 
  uint32_t                    m_pgwDownBytes;   //!< Pgw traffic downlink bytes.
  uint32_t                    m_pgwUpBytes;     //!< Pgw traffic uplink bytes.
  
  /** The EPC Pgw traffic trace source, fired at DumpPgwStatistics. */
  TracedCallback<DataRate, DataRate> m_pgwTrace;

  /** The LTE EPC QoS trace source, fired at DumpEpcStatistics. */
  TracedCallback<std::string, uint32_t, Ptr<const QosStatsCalculator> > m_epcTrace;

  /** The switch flow table rules trace source, fired at DumpSwtStatistics. */
  TracedCallback<std::vector<uint32_t> > m_swtTrace;

  /** The network bandwidth usage trace source, fired at DumpBwdStatistics. */
  TracedCallback<std::vector<BandwidthStats_t> > m_bwdTrace;
  
  /** A pair of QosStatsCalculator, for downlink and uplink statistics */
  typedef std::pair<Ptr<QosStatsCalculator>, Ptr<QosStatsCalculator> > QosStatsPair_t;
  
  /** Map saving <TEID / QoS stats > */
  typedef std::map<uint32_t, QosStatsPair_t> TeidQosMap_t;
  TeidQosMap_t        m_qosStats;         //!< TEID QoS statistics

  /** Map saving Node / Switch indexes. */
  typedef std::map<Ptr<Node>,uint16_t> NodeSwitchMap_t;  
  NodeSwitchMap_t     m_nodeSwitchMap;    //!< Registered nodes per switch idx.
};

};  // namespace ns3
#endif  // OPENFLOW_EPC_NETWORK_H
