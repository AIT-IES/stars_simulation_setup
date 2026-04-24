#include "ns3/basic-simulation.h"

#include "secondary-control.h"
typedef ns3::ExtU_secondary_control_T ExtU_secondary_control_T;
typedef ns3::ExtY_secondary_control_T ExtY_secondary_control_T;

#include <import/base/include/FMUCoSimulation_v2.h>

#include <iostream>
#include <iomanip> 
#include <fstream>
#include <string>

int 
main(int argc, char *argv[])
{
    // Retrieve run directory
    ns3::CommandLine cmd;
    std::string run_dir = "";
    cmd.Usage("Usage: ./waf --run=\"main_agc_no_delay --run_dir='<path/to/run/directory>'\"");
    cmd.AddValue("run_dir",  "Run directory", run_dir);
    cmd.Parse(argc, argv);
    if (run_dir.compare("") == 0) {
        std::cout << "Usage: ./waf --run=\"main_agc_no_delay --run_dir='<path/to/run/directory>'\"" << std::endl;
        return 0;
    }

    const std::string outputFileName_ctrl = run_dir + "/agc_9busSystem_ctrl_out.csv";
    std::ofstream outputFile_ctrl(outputFileName_ctrl);
    if (!outputFile_ctrl.is_open()) {
        std::cerr << "[ERROR] Could not open output file.\n";
        return 1;
    }
    outputFile_ctrl << "time,v_ref_delta1,v_ref_delta2,v_ref_delta3,p_set_inv1,p_set_inv2,p_set_inv3\n";

    const std::string outputFileName_ps = run_dir + "/agc_9busSystem_out.csv";
    std::ofstream outputFile_ps(outputFileName_ps);
    if (!outputFile_ps.is_open()) {
        std::cerr << "[ERROR] Could not open output file.\n";
        return 1;
    }
    outputFile_ps << "time,f1_bs,f2_bs,f3_bs,P1_bs,P2_bs,P3_bs,I1_bs,I2_bs,I3_bs,V1_ref_delta,V2_ref_delta,V3_ref_delta,P1_set_inv,P2_set_inv,P3_set_inv,Q1_bs,Q2_bs,Q3_bs\n";

    double start_time = 0.;
    double stop_time = 25.;
    double time = start_time;

    ExtU_secondary_control_T sc_in;

    // Initialize the controller model
    // STABLE: kp_f: 0.0, ki_f: 1.0, kp_V: 0.1, ki_V: 0.3
    ns3::SecondaryControl sc(0., 1., 0.1, 0.3);
    // // UNSTABLE: kp_f: 5.0, ki_f: 10.0, kp_V: 1.0, ki_V: 3.0
    // ns3::SecondaryControl sc(5., 10., 1., 3.);

    fmi_2_0::FMUCoSimulation ps_fmu(
        "file://" + run_dir + "/../power_system_model_fmu_extracted",
		"agc_9busSystem_noSecondaryCtrl", true, 1e-6
    );
    
    fmippStatus status = fmippFatal;
    
    // Instantiate the FMU model.
    status = ps_fmu.instantiate("AGC_9BUSSYSTEM_NOSECONDARYCTRL", 0., false, false);
    NS_ABORT_MSG_UNLESS(status == fmippOK, "instantiation of FMU failed");
    
    // Initialize the FMU model.
    status = ps_fmu.initialize(0., false, std::numeric_limits<double>::max());
    NS_ABORT_MSG_UNLESS(status == fmippOK, "initialization of FMU failed");
    
    fmi2ValueReference ps_in_vr[6] = {0, 1, 2, 3, 4, 5};
    fmi2Real ps_in[6] = {0., 0., 0., 0., 0., 0., };

    fmi2ValueReference ps_out_vr[12] = {1003, 1009, 1010, 1011, 1006, 1007, 1008, 1015, 1016, 1017, 1004, 1005};
    fmi2Real ps_out[12] = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};

    uint16_t n_steps = 0;

    while (time < stop_time) {

        if (0 == n_steps%10) {
            const ExtY_secondary_control_T& sc_out = sc.getOutputs();

            outputFile_ctrl << std::fixed << std::setprecision(3) << time << "," << std::scientific << std::setprecision(8)
                << sc_out.V_ref_delta1 << "," << sc_out.V_ref_delta2 << "," << sc_out.V_ref_delta3 << ","
                << sc_out.P_set_inv1 << "," << sc_out.P_set_inv2 << "," << sc_out.P_set_inv3 << std::endl;

            ps_in[0] = sc_out.V_ref_delta1; ps_in[1] = sc_out.V_ref_delta2; ps_in[2] = sc_out.V_ref_delta3;
            ps_in[3] = sc_out.P_set_inv1; ps_in[4] = sc_out.P_set_inv2; ps_in[5] = sc_out.P_set_inv3;

            ps_fmu.setValue(ps_in_vr, ps_in, 6);
        }

        ps_fmu.doStep(time, 1e-4, fmi2False);

        ps_fmu.getValue(ps_out_vr, ps_out, 12);

        if (0 == n_steps%10) {
            sc_in.f_SM = ps_out[0]; sc_in.Q_bs1 = ps_out[1]; sc_in.Q_bs2 = ps_out[2]; sc_in.Q_bs3 = ps_out[3];
            sc.setInputs(sc_in);

            sc.step(ns3::Seconds(time), ns3::Seconds(1e-3));
        }

        outputFile_ps << std::fixed << std::setprecision(4) << time << "," << std::scientific << std::setprecision(8)
            << ps_out[0] << "," << ps_out[10] << "," << ps_out[11] << ","
            << ps_out[4] << "," << ps_out[5] << "," << ps_out[6] << ","
            << ps_out[7] << "," << ps_out[8] << "," << ps_out[9] << ","
            << ps_in[0] << "," << ps_in[1] << "," << ps_in[2] << ","
            << ps_in[3] << "," << ps_in[4] << "," << ps_in[5] << ","
            << ps_out[1] << "," << ps_out[2] << "," << ps_out[3] << std::endl;

        ++n_steps;
        time += 1e-4;
    }

    outputFile_ctrl.close();

    return 0;
}
