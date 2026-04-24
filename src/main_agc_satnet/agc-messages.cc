#include "agc-messages.h"

#include "ns3/abort.h"

#include <map>

using namespace std;

namespace AGCMessages {

map<uint32_t, ControlSignal> _control_signals;
map<uint32_t, Measurement> _measurements;

void storeControlSignal(uint32_t msg_id, const ControlSignal& cs)
{
    _control_signals[msg_id] = cs;
}

ControlSignal retrieveControlSignal(uint32_t msg_id, bool remove)
{
    map<uint32_t, ControlSignal>::iterator it_find = _control_signals.find(msg_id);

    NS_ABORT_MSG_UNLESS(it_find != _control_signals.end(), "no control signal found with ID=" + to_string(msg_id));

    if (true == remove) {
        ControlSignal result = move(it_find->second);
        _control_signals.erase(it_find);
        return result;
    } else {
        return it_find->second;
    }
}

void storeMeasurement(uint32_t msg_id, const Measurement& m)
{
    _measurements[msg_id] = m;
}

Measurement retrieveMeasurement(uint32_t msg_id, bool remove)
{
    map<uint32_t, Measurement>::iterator it_find = _measurements.find(msg_id);

    NS_ABORT_MSG_UNLESS(it_find != _measurements.end(), "no measurement found with ID=" + to_string(msg_id));

    if (true == remove) {
        Measurement result = move(it_find->second);
        _measurements.erase(it_find);
        return result;
    } else {
        return it_find->second;
    }
}

}
