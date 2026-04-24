#include "ns3/abort.h"
#include "ns3/fmu-util.h"

#include "agc-device-logic.h"
#include "agc-messages.h"

using namespace std;
using namespace ns3;

AgcDeviceLogic::AgcDeviceLogic(
    uint64_t node_id_agc1,
    vector<string> measurement_request_agc1,
    uint64_t node_id_agc2,
    vector<string> measurement_request_agc2,
    uint64_t node_id_agc3,
    vector<string> measurement_request_agc3
) : m_node_id_agc1( node_id_agc1 ),
    m_node_id_agc2( node_id_agc2 ),
    m_node_id_agc3( node_id_agc3 ),
    m_measurement_request_agc1( measurement_request_agc1 ),
    m_measurement_request_agc2( measurement_request_agc2 ),
    m_measurement_request_agc3( measurement_request_agc3 )
{}

void
AgcDeviceLogic::init(
    Ptr<RefFMU> fmu,
    uint64_t nodeId,
    const string& modelIdentifier,
    const double& startTime
) {
    fmippStatus status = fmippFatal;

    // Instantiate the FMU model.
    status = fmu->instantiate(modelIdentifier + to_string(nodeId), 0., false, false);
    NS_ABORT_MSG_UNLESS(status == fmippOK, "instantiation of FMU failed");

    // Initialize the FMU model with the specified tolerance.
    status = fmu->initialize(startTime, false, numeric_limits<double>::max());
    NS_ABORT_MSG_UNLESS(status == fmippOK, "initialization of FMU failed");
}

ns3::Payload
AgcDeviceLogic::do_step(
    ns3::Ptr<ns3::RefFMU> fmu, 
    uint64_t nodeId, 
    const string& payload,
    uint32_t payloadId,
    bool isReply,
    const double& time,
    const double& commStepSize
) {
    // Call the default callback implementation for stepping the FMU model.
    FmuAttachedDevice::defaultDoStepCallbackImpl(fmu, nodeId, payload, payloadId, isReply, time, commStepSize);

    if ( true == isReply )
    {
        AGCMessages::ControlSignal controlSignal = AGCMessages::retrieveControlSignal(payloadId);

        for (const auto& controlBlock: controlSignal) {
            NS_ABORT_MSG_UNLESS(nodeId == controlBlock.targetDeviceId, "wrong device ID");

            fmippStatus status = fmu->setValue(controlBlock.targetName, controlBlock.targetValue);

            NS_ABORT_MSG_UNLESS(status == fmippOK, 
                "Setting value of parameter '" + controlBlock.targetName + "' in FMU model failed");
        }

        return Payload("ACK");
    } 
    else
    {
        vector<string> *measurementRequests;

        // Retrieve measurements.
        if (nodeId == m_node_id_agc1) {
            measurementRequests = &m_measurement_request_agc1;
        } else if (nodeId == m_node_id_agc2) {
            measurementRequests = &m_measurement_request_agc2;
        } else if (nodeId == m_node_id_agc3) {
            measurementRequests = &m_measurement_request_agc3;
        }

        AGCMessages::Measurement measurement;
        for (const auto& mr: *measurementRequests) {
            measurement.push_back({
                .sampleDeviceId=nodeId, .sampleName=mr, .sampleTime=time, .sampleValue=fmu->getRealValue(mr)
            });
        }

        Payload pl(150); // buffer size = 150 bytes
        AGCMessages::storeMeasurement(pl.GetId(), measurement);
        return pl;
    } 
}
