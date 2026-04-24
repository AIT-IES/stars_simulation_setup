#ifndef AGC_MESSAGES_H
#define AGC_MESSAGES_H

#include <string>
#include <vector>

namespace AGCMessages {

    struct ControlBlock {
        uint64_t targetDeviceId;
        std::string targetName;
        double targetValue;
    };

    struct MeasurementSample {
        uint64_t sampleDeviceId;
        std::string sampleName;
        double sampleTime;
        double sampleValue;
    };

    typedef std::vector<ControlBlock> ControlSignal;
    typedef std::vector<MeasurementSample> Measurement;

    void storeControlSignal(uint32_t msg_id, const ControlSignal& cs);
    ControlSignal retrieveControlSignal(uint32_t id, bool remove=true);

    void storeMeasurement(uint32_t msg_id, const Measurement& m);
    Measurement retrieveMeasurement(uint32_t id, bool remove=true);

}


#endif // AGC_MESSAGES_H