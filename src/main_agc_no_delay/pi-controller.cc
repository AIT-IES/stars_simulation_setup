#include "pi-controller.h"

using namespace ns3;

PIController::PIController(
    double kp, double ki, double u_lim_low, double u_lim_up, double dt
) : m_kp(kp), m_ki(ki), m_u_lim_low(u_lim_low), m_u_lim_up(u_lim_up), m_dt(dt), m_integral(0.0) {}

double
PIController::update(
    double setpoint, double measurement
) {
    double error = setpoint - measurement;

    double u0 = m_kp * error + m_integral;

    m_integral += m_ki * error * m_dt;

    if (u0 > m_u_lim_up) {
        return m_u_lim_up;
    } else if (u0 < m_u_lim_low) {
        return m_u_lim_low;
    } else {
        return u0;
    }
}

void
PIController::reset()
{
    m_integral = 0.0;
}
