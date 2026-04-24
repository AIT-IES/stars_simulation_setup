#ifndef SECONDARY_CONTROL_H
#define SECONDARY_CONTROL_H

#include <cstdint>

#include <ns3/nstime.h>

#include "pi-controller.h"

#define F_SM_DEFAULT 50.0
#define Q_BS1_DEFAULT 0.0
#define Q_BS2_DEFAULT 0.0
#define Q_BS3_DEFAULT 0.0
#define Q_BS3_DEFAULT 0.0
#define STEP_SIZE_DEFAULT 1e-4

namespace ns3
{

    // Controller inputs.
    struct ExtU_secondary_control_T
    {
        double f_SM;
        double Q_bs1;
        double Q_bs2;
        double Q_bs3;
    };

    // Controller outputs.
    struct ExtY_secondary_control_T
    {
        double P_set_inv1;
        double P_set_inv2;
        double P_set_inv3;
        double V_ref_delta1;
        double V_ref_delta2;
        double V_ref_delta3;
    };

    class SecondaryControl
    {
    public:
        SecondaryControl(
            double kp_f,
            double ki_f,
            double kp_q,
            double ki_q,
            double init_f_SM = F_SM_DEFAULT,
            double init_Q_bs1 = Q_BS1_DEFAULT,
            double init_Q_bs2 = Q_BS2_DEFAULT,
            double init_Q_bs3 = Q_BS3_DEFAULT,
            double dt = STEP_SIZE_DEFAULT);

        void step(
            const Time &currentCommunicationPoint,
            const Time &communicationPointStepSize);

        void setInputs(const ExtU_secondary_control_T &in)
        {
            this->m_in = in;
        }

        const ExtY_secondary_control_T &getOutputs() const
        {
            return this->m_out;
        }

    private:
        void oneStep();

        ExtU_secondary_control_T m_in;
        ExtY_secondary_control_T m_out;

        PIController m_pi_f;
        PIController m_pi_q_bs1;
        PIController m_pi_q_bs2;
        PIController m_pi_q_bs3;

        uint64_t m_step;
    };

}

#endif // SECONDARY_CONTROL_H
