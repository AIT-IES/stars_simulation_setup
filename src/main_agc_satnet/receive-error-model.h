#ifndef RECEIVE_ERROR_MODEL
#define RECEIVE_ERROR_MODEL

#include "ns3/config.h"
#include "ns3/error-model.h"
#include "ns3/gsl-net-device.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"

namespace ns3
{

// Error model according to:
// G. Hasslinger and O. Hohlfeld, "The Gilbert-Elliott Model for Packet Loss in Real Time Services on the Internet",
// 14th GI/ITG Conference - Measurement, Modelling and Evalutation of Computer and Communication Systems, Dortmund, Germany, 2008, pp. 1-15.
//
// Transition matrix:
//   from 0 (Ok): 0 (Ok) -> 1 (Fault) with prob m_fault_rate, 0 (Ok) -> 0 (Ok) with prob 1-m_fault_rate
//   from 1 (Fault): 1 (Fault) -> 0 (Ok) with prob m_recovery_rate, 1 (Fault) -> 1 (Fault) with prob 1-m_recovery_rate
class ReceiveErrorModel : public ErrorModel
{

    public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId(void);

    ReceiveErrorModel();
    virtual ~ReceiveErrorModel() {}

    static void ApplyToGSLNetDevices(
        const NodeContainer &nodes,
        const double& fault_rate,
        const double& recovery_rate
    );

private:

    virtual bool DoCorrupt(Ptr<Packet> p);
    virtual void DoReset(void);

    enum State { 
        Ok = 0, 
        Fault = 1 
    };

    State step();

    uint64_t m_node_id; //!< Node identifier.

    State m_state;
    double m_fault_rate; // 0 (Ok) -> 1 (Fault)
    double m_recovery_rate; // 1 (Fault) -> 0 (Ok)

    Ptr<RandomVariableStream> m_ranvar; //!< rng stream
};

}

#endif // RECEIVE_ERROR_MODEL
