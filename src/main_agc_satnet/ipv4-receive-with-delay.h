#ifndef IPV4_RECEIVE_WITH_DELAY
#define IPV4_RECEIVE_WITH_DELAY

#include "ns3/ipv4-l3-protocol.h"
#include "ns3/net-device.h"
#include "ns3/processing-time.h"

class IPv4ReceiveWithDelay : public Object
{
public:
    IPv4ReceiveWithDelay(
        Ptr<Node> node, 
        Ptr<NetDevice> device, 
        Time ptConst, 
        Time ptMean, 
        Time ptStdDev
    ) : m_device(device), 
        m_protocol(node->GetObject<Ipv4L3Protocol>()),
        m_processingTime(CreateObject<ProcessingTime>(ptConst, ptMean, ptStdDev))
    {
        NS_ABORT_MSG_UNLESS(m_protocol != 0, "protocol not supported");
    }

    bool
    Receive(
        Ptr<NetDevice> device, 
        Ptr<const Packet> packet, 
        uint16_t protocol, 
        const Address &from
    ) {
        if ((m_device == device) && (0x0800 == protocol)) // only support IPv4
        {
            Simulator::Schedule(
                m_processingTime->GetValue(),
                &Ipv4L3Protocol::Receive, m_protocol,
                device, packet, protocol, from, m_device->GetAddress(), NetDevice::PacketType(0)
            );
            return true;
        }  
        return false;
    }

    static void ApplyToGSLNetDevices(
        const NodeContainer& nodes,
        double proc_time_const_ns,
        double proc_time_mean_ns,
        double proc_time_std_dev_ns
    ) {
        // Iterate over all nodes
        for (NodeContainer::Iterator itNode = nodes.Begin(); itNode != nodes.End(); ++itNode) {
            // For each node, iterate over all network interfaces
            Ptr<Ipv4> ipv4 = (*itNode)->GetObject<Ipv4>();
            uint32_t nInterfaces = ipv4->GetNInterfaces();
            for (uint32_t iInterface = 0; iInterface < nInterfaces; ++iInterface) {
                Ptr<GSLNetDevice> device = ipv4->GetNetDevice(iInterface)->GetObject<GSLNetDevice>();

                // Check if this is a GSLNetDevice:
                // - if it is a satellite node, this is a network interface connected with a ground station
                // - if it is a ground station node, this is a network interface connected with a satellite
                if (device != 0) { 
                    // Add receive callbacks with extra processing time to emulate real-world packet delays
                    Ptr<IPv4ReceiveWithDelay> cb = CreateObject<IPv4ReceiveWithDelay>(
                        *itNode, device, 
                        NanoSeconds(proc_time_const_ns), NanoSeconds(proc_time_mean_ns), NanoSeconds(proc_time_std_dev_ns)
                    );
                    device->SetReceiveCallback (MakeCallback (&IPv4ReceiveWithDelay::Receive, cb));
                }
            }
        }
    }

private:
    Ptr<NetDevice> m_device;
    Ptr<Ipv4L3Protocol> m_protocol;
    Ptr<ProcessingTime> m_processingTime;
};

#endif // IPV4_RECEIVE_WITH_DELAY
