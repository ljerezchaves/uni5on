#!/bin/bash

function PrintHelp () {
  echo "Usage: $0 -l load -u ues [-d dist] [ns3-attr=value, ...]"
  echo "Where  load     The UE load distribution among eNBs: bal (balanced) or unb (unbalanced)."
  echo "       ues      The total number of UEs in the topology."
  echo "       dist     The UE load distribution among eNBs in % for unbalanced load. Use a comma-separed list without spaces like \"10,20,50,20\" for 4 eNBs."
  echo "       ns3-attr EpcOf ns-3 topology attributes."
  exit 0
}

# ns-3 attributes with their default values
Enbs=1
NumSwitches=3
SwitchLinkDataRate="100Mb/s"
VoipTraffic=false
GbrLiveVideoTraffic=false
BufferedVideoTraffic=false
NonGbrLiveVideoTraffic=false
HttpTraffic=false
GbrLinkQuota=0.4
GbrSafeguard="5Mb/s"
NonGbrAdjustmentStep="5Mb/s"
NonGbrCoexistence=true
VoipQueue=true
Strategy="spo"
EnableShortDebar=false
EnableLongDebar=false
DebarIncStep=0.025

# Local arguments
load="bal"
ues=0
load_distribution=0

# Parsing command line arguments with getopt
ARGS=$(getopt -o l:u:d: --long \
Enbs:,\
NumSwitches:,\
SwitchLinkDataRate:,\
AllTraffic:,\
VoipTraffic:,\
GbrLiveVideoTraffic:,\
BufferedVideoTraffic:,\
NonGbrLiveVideoTraffic:,\
HttpTraffic:,\
GbrLinkQuota:,\
GbrSafeguard:,\
NonGbrAdjustmentStep:,\
NonGbrCoexistence:,\
VoipQueue:,\
Strategy:,\
EnableShortDebar:,\
EnableLongDebar:,\
DebarIncStep:\
 -- "$@")

eval set -- "$ARGS"
if [ $? != 0 ]; then 
  echo "Failed parsing options." >&2; 
  exit 1; 
fi

required=0;
has_distribution=0
while true ; do
  case "$1" in
    -l)
      let required=required+1;
      load=$2 ; shift 2 ;;
    -u)
      let required=required+1;
      ues=$2 ; shift 2 ;;
    -d)
      has_distribution=1
      load_distribution=$2 ; shift 2 ;;
    --Enbs)
      Enbs=$2 ; shift 2 ;;
    --NumSwitches)
      NumSwitches=$2 ; shift 2 ;;
    --SwitchLinkDataRate)
      SwitchLinkDataRate=$2 ; shift 2 ;;
    --AllTraffic)
      VoipTraffic=$2 ;
      GbrLiveVideoTraffic=$2 ;
      BufferedVideoTraffic=$2 ;
      NonGbrLiveVideoTraffic=$2 ;
      HttpTraffic=$2 ; shift 2 ;;
    --VoipTraffic)
      VoipTraffic=$2 ; shift 2 ;;
    --GbrLiveVideoTraffic)
      GbrLiveVideoTraffic=$2 ; shift 2 ;;
    --BufferedVideoTraffic)
      BufferedVideoTraffic=$2 ; shift 2 ;;
    --NonGbrLiveVideoTraffic)
      NonGbrLiveVideoTraffic=$2 ; shift 2 ;;
    --HttpTraffic)
      HttpTraffic=$2 ; shift 2 ;;
    --GbrLinkQuota)
      GbrLinkQuota=$2 ; shift 2 ;;
    --GbrSafeguard)
      GbrSafeguard=$2 ; shift 2 ;;
    --NonGbrAdjustmentStep)
      NonGbrAdjustmentStep=$2 ; shift 2 ;;
    --NonGbrCoexistence)
      NonGbrCoexistence=$2 ; shift 2 ;;
    --VoipQueue)
      VoipQueue=$2 ; shift 2 ;;
    --Strategy)
      Strategy=$2 ; shift 2 ;;
    --EnableShortDebar)
      EnableShortDebar=$2 ; shift 2 ;;
    --EnableLongDebar)
      EnableLongDebar=$2 ; shift 2 ;;
    --DebarIncStep)
      DebarIncStep=$2 ; shift 2 ;;
    --) shift ; break ;;
    *) echo "Internal error!" ; exit 1 ;;
  esac
done

# Check for required arguments
if [ ${required} -lt 2 ]; then
  echo "Missing required argument(s)."
  PrintHelp
fi;

case "${load}" in 
  bal)
    uesPerEnb=$(echo "${ues} / ${Enbs}" | bc)
    for ((i=0; ${i}<${Enbs}; i++)); do
      uesDistrib[${i}]=${uesPerEnb}
    done

    let uesMissed=ues-Enbs*uesPerEnb
    for ((i=0; ${i}<${uesMissed}; i++)); do
      let uesDistrib[${i}]=uesDistrib[${i}]+1
    done
  ;;

  unb)
    # Check for load distribution  
    if [ ${has_distribution} -eq 0 ]; then
      echo "No load distribution provided."
      PrintHelp
    fi;

    total=0
    for ((i=0; ${i}<${Enbs}; i++)); do
      let next=i+1
      factor=$(echo "$(echo "${load_distribution}" | cut -f${next} -d',')" | bc -l)
      uesDistrib[${i}]=$(echo "(${factor} * ${ues}) / 100" | bc)
      let total=total+uesDistrib[${i}]
    done
    let uesMissed=ues-total
    for ((i=0; ${i}<${uesMissed}; i++)); do
      let uesDistrib[${i}]=uesDistrib[${i}]+1
    done
  ;;
  
  *)
    echo "Invalid action"
    PrintHelp
  ;;
esac

echo "# This is a topology description file, used by SimulationScenario to set"
echo "# default attributes, and proper attach each eNB to correct OpenFlow switch"
echo "# index."
echo
echo "# At first, we must set the number of eNBs, OpenFlow switches, and any other"
echo "# desired attribute in the following format:"
echo "# 1st column: keywork 'set'"
echo "# 2nd column: the attribute path."
echo "# 3rd column: the attribute value inside \"\"."
echo
echo "set ns3::LteHexGridNetwork::Enbs \"${Enbs}\""
echo
echo "set ns3::RingNetwork::NumSwitches \"${NumSwitches}\""
echo "set ns3::RingNetwork::SwitchLinkDataRate \"${SwitchLinkDataRate}\""
echo
echo "set ns3::TrafficHelper::VoipTraffic \"${VoipTraffic}\""
echo "set ns3::TrafficHelper::GbrLiveVideoTraffic \"${GbrLiveVideoTraffic}\""
echo "set ns3::TrafficHelper::BufferedVideoTraffic \"${BufferedVideoTraffic}\""
echo "set ns3::TrafficHelper::NonGbrLiveVideoTraffic \"${NonGbrLiveVideoTraffic}\""
echo "set ns3::TrafficHelper::HttpTraffic \"${HttpTraffic}\""
echo
echo "set ns3::ConnectionInfo::GbrLinkQuota \"${GbrLinkQuota}\""
echo "set ns3::ConnectionInfo::GbrSafeguard \"${GbrSafeguard}\""
echo "set ns3::ConnectionInfo::NonGbrAdjustmentStep \"${NonGbrAdjustmentStep}\""
echo
echo "set ns3::OpenFlowEpcController::NonGbrCoexistence \"${NonGbrCoexistence}\""
echo "set ns3::OpenFlowEpcController::VoipQueue \"${VoipQueue}\""
echo
echo "set ns3::RingController::Strategy \"${Strategy}\""
echo "set ns3::RingController::EnableShortDebar \"${EnableShortDebar}\""
echo "set ns3::RingController::EnableLongDebar \"${EnableLongDebar}\""
echo "set ns3::RingController::DebarIncStep \"${DebarIncStep}\""
echo
echo "# Then, we can describe the topology in the following format (observe the"
echo "# strict increasing eNB order):"
echo "# 1st column: keywork 'topo'"
echo "# 2nd column: the eNB index (starting at 0)."
echo "# 3rd column: the number of UEs at this eNB."
echo "# 4th column: the switch index to attach this eNB to (starting at 1, as switch"
echo "#             0 is the one attached to gateway)."
echo
for ((i=0; ${i}<${Enbs}; i++)); do
  let swt=i+1
  echo "topo ${i} ${uesDistrib[${i}]} ${swt}" 
done


