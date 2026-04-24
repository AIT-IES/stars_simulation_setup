#ifndef REROUTING_ARBITER_H
#define REROUTING_ARBITER_H

#include "ns3/basic-simulation.h"
#include "ns3/arbiter-single-forward.h"

namespace ns3
{

class ReroutingArbiter : public ArbiterSingleForward
{
public:

    typedef std::map<uint64_t, bool> ReroutingTable;

    ReroutingArbiter(
        Ptr<Node> this_node,
        int32_t reroute_node_id,
        NodeContainer nodes,
        std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list
    ) : 
        ArbiterSingleForward(this_node, nodes, next_hop_list), 
        m_reroute_node_id(reroute_node_id) 
    {}

    static TypeId GetTypeId (void);

    // Single forward next-hop implementation
    std::tuple<int32_t, int32_t, int32_t> TopologySatelliteNetworkDecide(
            int32_t source_node_id,
            int32_t target_node_id,
            ns3::Ptr<const ns3::Packet> pkt,
            ns3::Ipv4Header const &ipHeader,
            bool is_socket_request_for_source_ip
    );

protected:
    int32_t m_reroute_node_id;
    static ReroutingTable m_rerouting_table;
};

class ReroutingArbiterHelper
{
public:

    ReroutingArbiterHelper(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes);

private:

    std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> InitialEmptyForwardingState();
    void UpdateForwardingState(int64_t t);

    // Parameters
    Ptr<BasicSimulation> m_basicSimulation;
    NodeContainer m_nodes;
    int64_t m_dynamicStateUpdateIntervalNs;
    std::vector<Ptr<ReroutingArbiter>> m_arbiters;

};

}

#endif // REROUTING_ARBITER_H
