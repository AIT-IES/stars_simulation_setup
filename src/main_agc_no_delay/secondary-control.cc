#include "secondary-control.h"

using namespace ns3;

SecondaryControl::SecondaryControl(
    double kp_f,
    double ki_f,
    double kp_q,
    double ki_q,
    double init_f_SM,
    double init_Q_bs1,
    double init_Q_bs2,
    double init_Q_bs3,
    double dt
) :
    m_in({init_f_SM, init_Q_bs1, init_Q_bs2, init_Q_bs3}),
    m_out({0., 0., 0., 0., 0., 0.}),
    m_pi_f(kp_f, ki_f, 0., 3., dt),
    m_pi_q_bs1(kp_q, ki_q, 0., 3., dt),
    m_pi_q_bs2(kp_q, ki_q, 0., 3., dt),
    m_pi_q_bs3(kp_q, ki_q, 0., 3., dt),
    m_step(0)
{}

void
SecondaryControl::step(
    const Time& currentCommunicationPoint,
    const Time& communicationPointStepSize
) {
    double dt = (currentCommunicationPoint + communicationPointStepSize).GetSeconds();
	uint64_t next = static_cast<uint64_t>(dt/DEFAULT_PI_STEP_SIZE);

    while (this->m_step < next)
	{
		// Step the model.
        this->oneStep();
    }

}

void
SecondaryControl::oneStep()
{
    double p_set_inv = m_pi_f.update(50., m_in.f_SM);
    m_out.P_set_inv1 = p_set_inv * 0.33333333333333331;
    m_out.P_set_inv2 = p_set_inv * 0.33333333333333331;
    m_out.P_set_inv3 = p_set_inv * 0.33333333333333331;

    double q_sum_shared = (m_in.Q_bs1 + m_in.Q_bs2 + m_in.Q_bs3) * 0.33333333333333331;
    m_out.V_ref_delta1 = m_pi_q_bs1.update(q_sum_shared, m_in.Q_bs1);
    m_out.V_ref_delta2 = m_pi_q_bs1.update(q_sum_shared, m_in.Q_bs2);
    m_out.V_ref_delta3 = m_pi_q_bs1.update(q_sum_shared, m_in.Q_bs3);

    ++m_step;
}
