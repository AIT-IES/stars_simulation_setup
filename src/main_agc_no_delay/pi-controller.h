#ifndef PI_CONTROLLER_H
#define PI_CONTROLLER_H

#include <functional>

#define DEFAULT_PI_STEP_SIZE 1e-4

namespace ns3
{

class PIController
{
public:

    PIController(double kp, double ki, double u_lim_low, double u_lim_up, double dt=DEFAULT_PI_STEP_SIZE);

    double update(double setpoint, double measurement);

    void reset();

private:

    double m_kp;
    double m_ki;
    double m_u_lim_low;
    double m_u_lim_up;
    double m_dt;

    // Represents the accumulated integral value used by the PIController in the ns3 namespace.
    // This member variable stores the sum of past errors for the integral component of the controller.
    double m_integral;
};

}

#endif // PI_CONTROLLER_H
