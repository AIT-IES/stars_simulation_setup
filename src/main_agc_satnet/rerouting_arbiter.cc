#include "ns3/ipv4-arbiter-routing-helper.h"

#include "rerouting_arbiter.h"

using namespace ns3;
using namespace std;

ReroutingArbiter::ReroutingTable ReroutingArbiter::m_rerouting_table;

NS_OBJECT_ENSURE_REGISTERED (ReroutingArbiter);
TypeId ReroutingArbiter::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::ReroutingArbiter")
            .SetParent<ArbiterSingleForward> ()
            .SetGroupName("BasicSim")
    ;
    return tid;
}

tuple<int32_t, int32_t, int32_t> ReroutingArbiter::TopologySatelliteNetworkDecide(
        int32_t source_node_id,
        int32_t target_node_id,
        Ptr<const Packet> pkt,
        Ipv4Header const &ipHeader,
        bool is_request_for_source_ip_so_no_next_header
) {
    uint64_t  pkt_id = pkt->GetUid();
    bool reroute = false;

    ReroutingTable::iterator find = m_rerouting_table.find(pkt_id);
    if (find == m_rerouting_table.end()) {
        m_rerouting_table[pkt_id] = true;
        reroute = true;
    } else if (m_node_id == m_reroute_node_id) {
        m_rerouting_table[pkt_id] = false;
    } else {
        reroute = find->second;
    }

    tuple<int32_t, int32_t, int32_t> routing;
    if (true == reroute) {
        routing = ArbiterSingleForward::TopologySatelliteNetworkDecide(
            source_node_id, m_reroute_node_id, pkt, ipHeader, is_request_for_source_ip_so_no_next_header
        );        
    } else {
        routing = ArbiterSingleForward::TopologySatelliteNetworkDecide(
            source_node_id, target_node_id, pkt, ipHeader, is_request_for_source_ip_so_no_next_header
        );
    }

    int32_t next_node_id = get<0>(routing);
    if (next_node_id == target_node_id) {
        m_rerouting_table.erase(pkt_id);
    }

    // cout << "[ReroutingArbiter::TopologySatelliteNetworkDecide] packet ID: " << pkt_id << 
    //     " - node ID: " << m_node_id << " - next node ID: " << next_node_id << endl;

    return routing;
}



ReroutingArbiterHelper::ReroutingArbiterHelper (Ptr<BasicSimulation> basicSimulation, NodeContainer nodes) {
    cout << "SETUP CUSTOM ROUTING" << endl;
    m_basicSimulation = basicSimulation;
    m_nodes = nodes;

    // Read in initial forwarding state
    cout << "  > Create initial forwarding state" << endl;
    vector<vector<tuple<int32_t, int32_t, int32_t>>> initial_forwarding_state = InitialEmptyForwardingState();
    basicSimulation->RegisterTimestamp("Create initial single forwarding state");

    int32_t reroute_node_id = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("reroute_node_id"));

    // Set the routing arbiters
    cout << "  > Setting the routing arbiter on each node" << endl;
    for (size_t i = 0; i < m_nodes.GetN(); i++) {
        Ptr<ReroutingArbiter> arbiter = CreateObject<ReroutingArbiter>(m_nodes.Get(i), reroute_node_id, m_nodes, initial_forwarding_state[i]);
        m_arbiters.push_back(arbiter);
        m_nodes.Get(i)->GetObject<Ipv4>()->GetRoutingProtocol()->GetObject<Ipv4ArbiterRouting>()->SetArbiter(arbiter);
    }
    basicSimulation->RegisterTimestamp("Setup routing arbiter on each node");

    // Load first forwarding state
    m_dynamicStateUpdateIntervalNs = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("dynamic_state_update_interval_ns"));
    cout << "  > Forward state update interval: " << m_dynamicStateUpdateIntervalNs << "ns" << endl;
    cout << "  > Perform first forwarding state load for t=0" << endl;
    UpdateForwardingState(0);
    basicSimulation->RegisterTimestamp("Create initial single forwarding state");

    cout << endl;
}

vector<vector<tuple<int32_t, int32_t, int32_t>>>
ReroutingArbiterHelper::InitialEmptyForwardingState() {
    vector<vector<tuple<int32_t, int32_t, int32_t>>> initial_forwarding_state;
    for (size_t i = 0; i < m_nodes.GetN(); i++) {
        vector <tuple<int32_t, int32_t, int32_t>> next_hop_list;
        for (size_t j = 0; j < m_nodes.GetN(); j++) {
            next_hop_list.push_back(make_tuple(-2, -2, -2)); // -2 indicates an invalid entry
        }
        initial_forwarding_state.push_back(next_hop_list);
    }
    return initial_forwarding_state;
}

void ReroutingArbiterHelper::UpdateForwardingState(int64_t t) {

    // Filename
    ostringstream res;
    res << m_basicSimulation->GetRunDir() << "/";
    res << m_basicSimulation->GetConfigParamOrFail("satellite_network_routes_dir") << "/fstate_" << t << ".txt";
    string filename = res.str();

    // Check that the file exists
    if (!file_exists(filename)) {
        throw runtime_error(format_string("File %s does not exist.", filename.c_str()));
    }

    // Open file
    string line;
    ifstream fstate_file(filename);
    if (fstate_file) {

        // Go over each line
        size_t line_counter = 0;
        while (getline(fstate_file, line)) {

            // Split on ,
            vector<string> comma_split = split_string(line, ",", 5);

            // Retrieve identifiers
            int64_t current_node_id = parse_positive_int64(comma_split[0]);
            int64_t target_node_id = parse_positive_int64(comma_split[1]);
            int64_t next_hop_node_id = parse_int64(comma_split[2]);
            int64_t my_if_id = parse_int64(comma_split[3]);
            int64_t next_if_id = parse_int64(comma_split[4]);

            // Check the node identifiers
            NS_ABORT_MSG_IF(current_node_id < 0 || current_node_id >= m_nodes.GetN(), "Invalid current node id.");
            NS_ABORT_MSG_IF(target_node_id < 0 || target_node_id >= m_nodes.GetN(), "Invalid target node id.");
            NS_ABORT_MSG_IF(next_hop_node_id < -1 || next_hop_node_id >= m_nodes.GetN(), "Invalid next hop node id.");

            // Drops are only valid if all three values are -1
            NS_ABORT_MSG_IF(
                    !(next_hop_node_id == -1 && my_if_id == -1 && next_if_id == -1)
                    &&
                    !(next_hop_node_id != -1 && my_if_id != -1 && next_if_id != -1),
                    "All three must be -1 for it to signify a drop."
            );

            // Check the interfaces exist
            NS_ABORT_MSG_UNLESS(my_if_id == -1 || (my_if_id >= 0 && my_if_id + 1 < m_nodes.Get(current_node_id)->GetObject<Ipv4>()->GetNInterfaces()), "Invalid current interface");
            NS_ABORT_MSG_UNLESS(next_if_id == -1 || (next_if_id >= 0 && next_if_id + 1 < m_nodes.Get(next_hop_node_id)->GetObject<Ipv4>()->GetNInterfaces()), "Invalid next hop interface");

            // Node id and interface id checks are only necessary for non-drops
            if (next_hop_node_id != -1 && my_if_id != -1 && next_if_id != -1) {

                // It must be either GSL or ISL
                bool source_is_gsl = m_nodes.Get(current_node_id)->GetObject<Ipv4>()->GetNetDevice(1 + my_if_id)->GetObject<GSLNetDevice>() != 0;
                bool source_is_isl = m_nodes.Get(current_node_id)->GetObject<Ipv4>()->GetNetDevice(1 + my_if_id)->GetObject<PointToPointLaserNetDevice>() != 0;
                NS_ABORT_MSG_IF((!source_is_gsl) && (!source_is_isl), "Only GSL and ISL network devices are supported");

                // If current is a GSL interface, the destination must also be a GSL interface
                NS_ABORT_MSG_IF(
                    source_is_gsl &&
                    m_nodes.Get(next_hop_node_id)->GetObject<Ipv4>()->GetNetDevice(1 + next_if_id)->GetObject<GSLNetDevice>() == 0,
                    "Destination interface must be attached to a GSL network device"
                );

                // If current is a p2p laser interface, the destination must match exactly its counter-part
                NS_ABORT_MSG_IF(
                    source_is_isl &&
                    m_nodes.Get(next_hop_node_id)->GetObject<Ipv4>()->GetNetDevice(1 + next_if_id)->GetObject<PointToPointLaserNetDevice>() == 0,
                    "Destination interface must be an ISL network device"
                );
                if (source_is_isl) {
                    Ptr<NetDevice> device0 = m_nodes.Get(current_node_id)->GetObject<Ipv4>()->GetNetDevice(1 + my_if_id)->GetObject<PointToPointLaserNetDevice>()->GetChannel()->GetDevice(0);
                    Ptr<NetDevice> device1 = m_nodes.Get(current_node_id)->GetObject<Ipv4>()->GetNetDevice(1 + my_if_id)->GetObject<PointToPointLaserNetDevice>()->GetChannel()->GetDevice(1);
                    Ptr<NetDevice> other_device = device0->GetNode()->GetId() == current_node_id ? device1 : device0;
                    NS_ABORT_MSG_IF(other_device->GetNode()->GetId() != next_hop_node_id, "Next hop node id across does not match");
                    NS_ABORT_MSG_IF(other_device->GetIfIndex() != 1 + next_if_id, "Next hop interface id across does not match");
                }

            }

            // Add to forwarding state
            m_arbiters.at(current_node_id)->SetSingleForwardState(
                    target_node_id,
                    next_hop_node_id,
                    1 + my_if_id,   // Skip the loop-back interface
                    1 + next_if_id  // Skip the loop-back interface
            );

            // Next line
            line_counter++;

        }

        // Close file
        fstate_file.close();

    } else {
        throw runtime_error(format_string("File %s could not be read.", filename.c_str()));
    }

    // Given that this code will only be used with satellite networks, this is okay-ish,
    // but it does create a very tight coupling between the two -- technically this class
    // can be used for other purposes as well
    if (!parse_boolean(m_basicSimulation->GetConfigParamOrDefault("satellite_network_force_static", "false"))) {

        // Plan the next update
        int64_t next_update_ns = t + m_dynamicStateUpdateIntervalNs;
        if (next_update_ns < m_basicSimulation->GetSimulationEndTimeNs()) {
            Simulator::Schedule(NanoSeconds(m_dynamicStateUpdateIntervalNs), &ReroutingArbiterHelper::UpdateForwardingState, this, next_update_ns);
        }
    }
}