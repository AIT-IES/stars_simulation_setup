#ifndef AGC_DEVICE_CONTROLLER_H
#define AGC_DEVICE_CONTROLLER_H

#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/payload.h"

#include "secondary-control.h"

#include <fstream>
#include <string>
#include <vector>
#include <map>

class AgcCentralController {
public:

    AgcCentralController(
        uint64_t node_id_agc1,
        uint64_t node_id_agc2,
        uint64_t node_id_agc3,
        uint64_t ctrl_period_in_ns,
        std::string res_filename,
        double kp_f = 0.0, 
        double ki_f = 1.0,
        double kp_Q = 0.1,
        double ki_Q = 0.3
    );

    ns3::Payload
    send_payload(
        uint64_t from, 
        int64_t to
    );

    void
    receive_payload(
        std::string payload,
        uint32_t payloadId,
        bool isReply,
        uint64_t from, 
        int64_t to
    );

private:

    void call_ctrl();

    void write_to_csv(
        const std::vector<std::string>& varnames, 
        const std::vector<double>& data
    ) const;

    uint64_t m_nodeIdAgc1;
    uint64_t m_nodeIdAgc2;
    uint64_t m_nodeIdAgc3;

    double m_agc1LatestTimestamp;
    double m_agc2LatestTimestamp;
    double m_agc3LatestTimestamp;

    ns3::SecondaryControl m_ctrl;
    ns3::Time m_ctrlPeriod;

    double m_f1_bs;
    double m_q1_bs;
    double m_q2_bs;
    double m_q3_bs;

    double m_v1_ref_delta;
    double m_v2_ref_delta;
    double m_v3_ref_delta;
    double m_p1_set_inv;
    double m_p2_set_inv;
    double m_p3_set_inv;

    ns3::EventId m_callCtrlEvent; //!< Event to call the controller.

    std::string m_resFilename;

    std::ifstream m_inputFile;

};

#endif // AGC_DEVICE_CONTROLLER_H