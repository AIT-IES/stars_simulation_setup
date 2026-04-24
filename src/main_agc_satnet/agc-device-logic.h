#ifndef AGC_DEVICE_LOGIC_H
#define AGC_DEVICE_LOGIC_H

#include "ns3/fmu-attached-device.h"
#include "ns3/payload.h"

#include <string>
#include <vector>

class AgcDeviceLogic {
public:

    AgcDeviceLogic(
        uint64_t node_id_agc1,
        std::vector<std::string> measurement_request_agc1,
        uint64_t node_id_agc2,
        std::vector<std::string> measurement_request_agc2,
        uint64_t node_id_agc3,
        std::vector<std::string> measurement_request_agc3
    );

    void
    init(
        ns3::Ptr<ns3::RefFMU> fmu, 
        uint64_t nodeId, 
        const std::string& modelIdentifier, 
        const double& startTime
    );

    ns3::Payload
    do_step(
        ns3::Ptr<ns3::RefFMU> fmu, 
        uint64_t nodeId, 
        const std::string& payload,
        uint32_t payloadId,
        bool isReply,
        const double& time,
        const double& commStepSize
    );

private:

    uint64_t m_node_id_agc1;
    uint64_t m_node_id_agc2;
    uint64_t m_node_id_agc3;

    std::vector<std::string> m_measurement_request_agc1;
    std::vector<std::string> m_measurement_request_agc2;
    std::vector<std::string> m_measurement_request_agc3;
};

#endif // AGC_DEVICE_LOGIC_H