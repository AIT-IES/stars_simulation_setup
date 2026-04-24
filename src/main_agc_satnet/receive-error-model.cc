#include "ns3/double.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

#include "receive-error-model.h"

using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED(ReceiveErrorModel);

ReceiveErrorModel::ReceiveErrorModel() : m_state(Ok) {}

TypeId
ReceiveErrorModel::GetTypeId(void)
{
  static TypeId tid = TypeId("ReceiveErrorModel")
      .SetParent<ErrorModel>()
      // .SetGroupName("Network")
      .AddConstructor<ReceiveErrorModel>()
      .AddAttribute("NodeID", "The node identifier",
          UintegerValue(0),
          MakeUintegerAccessor(&ReceiveErrorModel::m_node_id),
          MakeUintegerChecker<uint64_t>())
      .AddAttribute("FaultRate", "The fault rate.",
          DoubleValue(0.0),
          MakeDoubleAccessor(&ReceiveErrorModel::m_fault_rate),
          MakeDoubleChecker<double>())
      .AddAttribute("RecoveryRate", "The recovery rate.",
          DoubleValue(1.0),
          MakeDoubleAccessor(&ReceiveErrorModel::m_recovery_rate),
          MakeDoubleChecker<double>())
      .AddAttribute("RanVar", "The decision variable attached to this error model.",
          StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"),
          MakePointerAccessor(&ReceiveErrorModel::m_ranvar),
          MakePointerChecker<RandomVariableStream>());
  return tid;
}

bool
ReceiveErrorModel::DoCorrupt(Ptr<Packet> p)
{
  if (!IsEnabled()) { return false; }

  return (Fault == step());
}

void
ReceiveErrorModel::DoReset(void)
{
  m_state = Ok;
}

// Advance one step and return the new state.
ReceiveErrorModel::State
ReceiveErrorModel::step() {
  double u = m_ranvar ->GetValue();
  if (m_state == Ok) { // currently in Ok state
    m_state = (u < m_fault_rate) ? Fault : Ok;
  } else { // currently in Fault state
    m_state = (u < m_recovery_rate) ? Ok : Fault;
  }
  return m_state;
}

void
ReceiveErrorModel::ApplyToGSLNetDevices(
  const NodeContainer &nodes,
  const double& fault_rate,
  const double& recovery_rate
) {
  ObjectFactory factory;
  factory.SetTypeId("ReceiveErrorModel");
  factory.Set("FaultRate", DoubleValue(fault_rate));
  factory.Set("RecoveryRate", DoubleValue(recovery_rate));
  
  // Iterate over all nodes
  for (NodeContainer::Iterator itNode = nodes.Begin(); itNode != nodes.End(); ++itNode)
  {
    // For each node, iterate over all network interfaces
    Ptr<Ipv4> ipv4 = (*itNode)->GetObject<Ipv4>();
    uint32_t nInterfaces = ipv4->GetNInterfaces();
    for (uint32_t iInterface = 0; iInterface < nInterfaces; ++iInterface)
    {
      Ptr<GSLNetDevice> device = ipv4->GetNetDevice(iInterface)->GetObject<GSLNetDevice>();
      
      // Check if this is a GSLNetDevice:
      // - if it is a satellite node, this is a network interface connected with a ground station
      // - if it is a ground station node, this is a network interface connected with a satellite
      if (device != 0)
      {
        factory.Set("NodeID", UintegerValue((*itNode)->GetId()));
        Ptr<ErrorModel> em = factory.Create<ErrorModel>();
        device->SetReceiveErrorModel(em);
      }
    }
  }
}

