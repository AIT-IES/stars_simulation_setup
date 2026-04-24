#include "ns3/basic-simulation.h"
#include "ns3/topology-satellite-network.h"
#include "ns3/tcp-optimizer.h"
#include "ns3/arbiter-single-forward.h"
#include "ns3/ipv4-arbiter-routing-helper.h"
#include "ns3/gsl-if-bandwidth-helper.h"
#include "ns3/fmu-shared-device-factory.h"
#include "ns3/device-client-factory.h"
#include "ns3/point-to-point-laser-net-device.h"

#include "agc-central-controller.h"
#include "agc-device-logic.h"
#include "ipv4-receive-with-delay.h"
#include "receive-error-model.h"
#include "rerouting_arbiter.h"

using namespace std;
using namespace ns3;

namespace {
    template<class T>
    T getSetElementByIndex(const std::set<T>& s, size_t i) {
        return *next(s.begin(), i);
    }
}

int 
main(int argc, char *argv[]) {

    // No buffering of printf
    setbuf(stdout, nullptr);

    // Retrieve run directory
    CommandLine cmd;
    string run_dir = "";
    cmd.Usage("Usage: ./waf --run=\"main_agc_satnet --run_dir='<path/to/run/directory>'\"");
    cmd.AddValue("run_dir",  "Run directory", run_dir);
    cmd.Parse(argc, argv);
    if (run_dir.compare("") == 0) {
        printf("Usage: ./waf --run=\"main_agc_satnet --run_dir='<path/to/run/directory>'\"");
        return 0;
    }

    // Load basic simulation environment
    Ptr<BasicSimulation> basicSimulation = CreateObject<BasicSimulation>(run_dir);

    // Setting socket type
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::" + basicSimulation->GetConfigParamOrFail("tcp_socket_type")));

    // Optimize TCP
    TcpOptimizer::OptimizeBasic(basicSimulation);

    // Read topology, and install routing arbiters
    Ptr<TopologySatelliteNetwork> topology = CreateObject<TopologySatelliteNetwork>(basicSimulation, Ipv4ArbiterRoutingHelper());

    // Add receive callbacks with extra processing time to emulate real-world packet delays in all satellite nodes
    IPv4ReceiveWithDelay::ApplyToGSLNetDevices(
        topology->GetSatelliteNodes(),
        parse_positive_double(basicSimulation->GetConfigParamOrFail("gsl_processing_time_const_ns")),
        parse_positive_double(basicSimulation->GetConfigParamOrFail("gsl_processing_time_mean_ns")),
        parse_positive_double(basicSimulation->GetConfigParamOrFail("gsl_processing_time_std_dev_ns"))
    );

    int64_t nodeIdReroute = parse_positive_int64(basicSimulation->GetConfigParamOrFail("reroute_node_id"));
    Ptr<Node> nodeReroute = topology->GetNodes().Get(nodeIdReroute);

    // Add receive callbacks with extra processing time to emulate real-world packet delays in STARLINK ground station
    IPv4ReceiveWithDelay::ApplyToGSLNetDevices(
        nodeReroute,
        parse_positive_double(basicSimulation->GetConfigParamOrFail("starlink_gs_processing_time_const_ns")),
        parse_positive_double(basicSimulation->GetConfigParamOrFail("starlink_gs_processing_time_mean_ns")),
        parse_positive_double(basicSimulation->GetConfigParamOrFail("starlink_gs_processing_time_std_dev_ns"))
    );

    // Add error model (packet loss) to STARLINK ground station
    ReceiveErrorModel::ApplyToGSLNetDevices(
        NodeContainer(nodeReroute),
        parse_positive_double(basicSimulation->GetConfigParamOrFail("starlink_gs_fault_rate")),
        parse_positive_double(basicSimulation->GetConfigParamOrFail("starlink_gs_recovery_rate"))
    );

    ReroutingArbiterHelper arbiterHelper(basicSimulation, topology->GetNodes());
    GslIfBandwidthHelper gslIfBandwidthHelper(basicSimulation, topology->GetNodes());

    std::string configAgcBusses = basicSimulation->GetConfigParamOrFail("agc_busses");
    std::set<int64_t> agcBusses = parse_set_positive_int64(configAgcBusses);
    
    NS_ABORT_MSG_UNLESS(3 == agcBusses.size(), "This example expects exactly 3 endpoints attached to a shared FMU.");
    int64_t nodeIdAgc1 = getSetElementByIndex(agcBusses, 0);
    int64_t nodeIdAgc2 = getSetElementByIndex(agcBusses, 1);
    int64_t nodeIdAgc3 = getSetElementByIndex(agcBusses, 2);

    std::string configCtrlPeriodNs = basicSimulation->GetConfigParamOrFail("agc_central_ctrl_period_ns");
    string ctrlResultsFilename = basicSimulation->GetLogsDir() + "/" + "agc.csv";

    double agcKpF = parse_double(basicSimulation->GetConfigParamOrFail("agc_central_kp_f"));
    double agcKiF = parse_double(basicSimulation->GetConfigParamOrFail("agc_central_ki_f"));
    double agcKpQ = parse_double(basicSimulation->GetConfigParamOrFail("agc_central_kp_Q"));
    double agcKiQ = parse_double(basicSimulation->GetConfigParamOrFail("agc_central_ki_Q"));
    AgcCentralController ctrl(nodeIdAgc1, nodeIdAgc2, nodeIdAgc3, parse_int64(configCtrlPeriodNs), ctrlResultsFilename, agcKpF, agcKiF, agcKpQ, agcKiQ);

    // Schedule communication between clients and devices.
    DeviceClientFactory deviceClientFactory(basicSimulation, topology, 
        MakeCallback(&AgcCentralController::send_payload, &ctrl), 
        MakeCallback(&AgcCentralController::receive_payload, &ctrl)); // Requires enable_device_clients=true

    AgcDeviceLogic logic(nodeIdAgc1, {"f1_bs", "Q1_bs"}, nodeIdAgc2, {"Q2_bs"}, nodeIdAgc3, {"Q3_bs"});

    // Add devices attached to a shared FMU.
    FmuSharedDeviceFactory fmuDeviceFactory(basicSimulation, topology, 
        MakeCallback(&AgcDeviceLogic::init, &logic), 
        MakeCallback(&AgcDeviceLogic::do_step, &logic)); // Requires enable_fmu_attached_devices=true

    // Run simulation
    basicSimulation->Run();

    // // Write result
    // tcpFlowScheduler.WriteResults();

    // Write device communication results
    deviceClientFactory.WriteResults();

    // Collect utilization statistics
    topology->CollectUtilizationStatistics();

    // Finalize the simulation
    basicSimulation->Finalize();

    return 0;
}
