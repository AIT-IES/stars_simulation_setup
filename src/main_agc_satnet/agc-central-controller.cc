#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "ns3/exp-util.h"

#include <sstream>

#include "agc-central-controller.h"
#include "agc-messages.h"

using namespace std;
using namespace ns3;

AgcCentralController::AgcCentralController(
    uint64_t node_id_agc1,
    uint64_t node_id_agc2,
    uint64_t node_id_agc3,
    uint64_t ctrl_period_in_ns,
    std::string res_filename,
    double kp_f,
    double ki_f,
    double kp_Q,
    double ki_Q
) : m_nodeIdAgc1(node_id_agc1),
    m_nodeIdAgc2(node_id_agc2),
    m_nodeIdAgc3(node_id_agc3),
    m_agc1LatestTimestamp(0.),
    m_agc2LatestTimestamp(0.),
    m_agc3LatestTimestamp(0.),
    m_ctrl(kp_f, ki_f, kp_Q, ki_Q),
    m_ctrlPeriod(ns3::NanoSeconds(ctrl_period_in_ns)),
    m_f1_bs(50.),
    m_q1_bs(0.),
    m_q2_bs(0.),
    m_q3_bs(0.),
    m_v1_ref_delta(0.),
    m_v2_ref_delta(0.),
    m_v3_ref_delta(0.),
    m_p1_set_inv(0.),
    m_p2_set_inv(0.),
    m_p3_set_inv(0.),
    m_resFilename(res_filename)
{
    std::cout<<"AGC Central Controller initialized for nodes: "
             << node_id_agc1 << ", "
             << node_id_agc2 << ", "
             << node_id_agc3 << std::endl;

    remove_file_if_exists(m_resFilename);

    // Schedule the first controller call.
    m_callCtrlEvent = ns3::Simulator::Schedule(ns3::Simulator::Now(), &AgcCentralController::call_ctrl, this);
}

void
AgcCentralController::call_ctrl()
{
    const ExtY_secondary_control_T& ctrl_out = m_ctrl.getOutputs();
    m_v1_ref_delta = ctrl_out.V_ref_delta1;
    m_v2_ref_delta = ctrl_out.V_ref_delta2;
    m_v3_ref_delta = ctrl_out.V_ref_delta3;
    m_p1_set_inv = ctrl_out.P_set_inv1;
    m_p2_set_inv = ctrl_out.P_set_inv2;
    m_p3_set_inv = ctrl_out.P_set_inv3;

    ExtU_secondary_control_T ctrl_in;
    ctrl_in.f_SM = m_f1_bs;
    ctrl_in.Q_bs1 = m_q1_bs;
    ctrl_in.Q_bs2 = m_q2_bs;
    ctrl_in.Q_bs3 = m_q3_bs;

    m_ctrl.setInputs(ctrl_in);

    m_ctrl.step(ns3::Simulator::Now(), m_ctrlPeriod);

    // Write to CSV output file.
    write_to_csv(
        {"time", "f1_bs", "q1_bs", "q2_bs", "q3_bs", "v1_ref_delta", "p1_set_inv", "v2_ref_delta", "p2_set_inv", "v3_ref_delta", "p3_set_inv"},
        {ns3::Simulator::Now().GetSeconds(), m_f1_bs, m_q1_bs, m_q2_bs, m_q3_bs, m_v1_ref_delta, m_p1_set_inv, m_v2_ref_delta, m_p2_set_inv, m_v3_ref_delta, m_p3_set_inv}
    );

    // Schedule the next controller call.
    m_callCtrlEvent = ns3::Simulator::Schedule(m_ctrlPeriod, &AgcCentralController::call_ctrl, this);
}

ns3::Payload
AgcCentralController::send_payload(
    uint64_t from, 
    int64_t to
) {
    // Node ID must be valid (>0), otherwise it is not possible to send data ...
    NS_ABORT_MSG_UNLESS(to > 0, "Invalid node ID");
    uint64_t validTo = static_cast<uint64_t>(to);

    AGCMessages::ControlSignal cs;
    if (validTo == m_nodeIdAgc1)
    {
        cs.push_back({
            .targetDeviceId=validTo, .targetName="V1_ref_delta", .targetValue=m_v1_ref_delta
        });
        cs.push_back({
            .targetDeviceId=validTo, .targetName="P1_set_inv", .targetValue=m_p1_set_inv
        });
    }
    else if (validTo == m_nodeIdAgc2)
    {
        cs.push_back({
            .targetDeviceId=validTo, .targetName="V2_ref_delta", .targetValue=m_v2_ref_delta
        });
        cs.push_back({
            .targetDeviceId=validTo, .targetName="P2_set_inv", .targetValue=m_p2_set_inv
        });
    }
    else if (validTo == m_nodeIdAgc3)
    {
        cs.push_back({
            .targetDeviceId=validTo, .targetName="V3_ref_delta", .targetValue=m_v3_ref_delta
        });
        cs.push_back({
            .targetDeviceId=validTo, .targetName="P3_set_inv", .targetValue=m_p3_set_inv
        });
    }
    else
    {
        NS_ABORT_MSG("unknown device ID: " << validTo);
    }

    Payload pl(150); // buffer size = 150 bytes
    AGCMessages::storeControlSignal(pl.GetId(), cs);
    return pl;
}

void
AgcCentralController::receive_payload(
    std::string payload,
    uint32_t payloadId,
    bool isReply,
    uint64_t from, 
    int64_t to
) {
    if (true == isReply)
    {
        NS_ABORT_MSG_UNLESS(payload.find("ACK") != string::npos, "Invalid response (acknowledgment missing)");
    } 
    else
    {
        AGCMessages::Measurement measurement = AGCMessages::retrieveMeasurement(payloadId);
        for (auto m: measurement)
        {
            uint64_t deviceId = m.sampleDeviceId;
            string sampleName = m.sampleName;
            double sampleTime = m.sampleTime;

            // Retrieve measurements.
            if ((deviceId == m_nodeIdAgc1) && (sampleName == "f1_bs")) {
                if (sampleTime >= m_agc1LatestTimestamp) {
                    m_f1_bs = m.sampleValue;
                    m_agc1LatestTimestamp = sampleTime;
                }
            } else if ((deviceId == m_nodeIdAgc1) && (sampleName == "Q1_bs")) {
                if (sampleTime >= m_agc1LatestTimestamp) {
                    m_q1_bs = m.sampleValue;
                    m_agc1LatestTimestamp = sampleTime;
                }
            } else if ((deviceId == m_nodeIdAgc2) && (sampleName == "Q2_bs")) {
                if (sampleTime>=m_agc2LatestTimestamp) {
                    m_q2_bs = m.sampleValue;
                    m_agc2LatestTimestamp = sampleTime;
                }
            } else if ((deviceId == m_nodeIdAgc3) && (sampleName == "Q3_bs")) {
                if (sampleTime >= m_agc3LatestTimestamp) {
                    m_q3_bs = m.sampleValue;
                    m_agc3LatestTimestamp = sampleTime;
                }
            } else {
                NS_ABORT_MSG("unknown device ID / sample name: " << deviceId << " / " << sampleName);
            }   
        }
    }
}


void 
AgcCentralController::write_to_csv(
    const vector<string>& varnames, 
    const vector<double>& data) const
{
    // Open the file in append mode.
    ofstream file(m_resFilename, ios::app);

    if (!file.is_open()) { NS_FATAL_ERROR ("Failed to open file: " << m_resFilename); }

    // If the file is empty, write the column names.
    file.seekp(0, ios::end);
    if (0 == file.tellp()) {
        // Names of all other columns.
        for (size_t i = 0; i < varnames.size(); ++i) {
            file << varnames[i];
            if (i < varnames.size() - 1) { file << ","; }
        }
        file << "\n";
    }

    // Write all other rows.
    for (size_t i = 0; i < varnames.size(); ++i)
    {
        file << data[i];
        if (i < varnames.size() - 1) { file << ","; }
    }
    file << "\n";

    // Close the file.
    file.close();
}
