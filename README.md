# STARS simulation setup

## About

This repository contains the source code for running the simulations described in [https://doi.org/10.1145/3744255.3811737]:

For understanding the effects of latencies and packet loss in ICT systems on the reliability and performance of control applications for power systems, it is necessary to model both domains in detail to describe the behavior of the overall system.
To this end, [Hypatia](https://github.com/snkas/hypatia) has been extended to enable a co-simulation of the underlying discrete-event ICT network simulator [ns-3](https://www.nsnam.org/) with a continuous-time power system model exported from [MATLAB/Simulink](https://www.mathworks.com/products/simulink.html).
The coupling is achieved via the [FMI standard](https://fmi-standard.org/) (version~2.0), by exporting the power system model to a standalone co-simulation component--a so-called *FMU for Co-Simulation*--and importing it via a [dedicated ns-3 module](https://github.com/AIT-IES/hypatia-fmu-attached-device).
This ns-3 module implements application layer models that use a common shared FMU to compute their internal states.
In addition, it allows to define controllers via callback functions.

Based on this, a co-simulation setup has been implemented that combines the power system model, the LEO satellite network model and the power grid restoration controller.
The setup represents a realistic use case for transmission grid restoration after a blackout in Austria.
It comprises a control center located in the city of Vienna, close to one of the main facilities of the Austrian transmission system operator.
Three controlled substations are located near the cities of Kaprun, Linz, and Graz, which represent critical nodes in the Austrian transmission grid.
The control center and the substations are connected to the LEO satellite network via user terminals and a ground station, the latter located near Usingen, Germany, as observed in the measurement campaign.
Communication between the control center and the substations is assumed to follow the *IEC 61850* standard, with control signals and measurements mapped to *GOOSE* and *SV* messages, respectively.
The related messages are modelled with a typical datagram size of 150 bytes.
Following the communication requirements in IEC TR 61850-90-12 for *WAMPAC* applications related to frequency stability, a communication interval of 20 milliseconds (50 Hz) for both measurements and control signals is used.
In case of packet loss or reordering, the controller uses the latest measurement values received to compute a new control signal. 

## Installation (Ubuntu 20.04)

Start from a [clean Hypatia installation](https://github.com/snkas/hypatia?tab=readme-ov-file#getting-started), then follow the instructions found [here](https://github.com/AIT-IES/hypatia-fmu-attached-device?tab=readme-ov-file#quick-start-ubuntu-2004).

## Running AGC (Automatic Generation Control) Simulations

### Without Communication

For reference, you can run AGC simulations without the impact of communication networks.
From the root folder of this repository, run the following command:

``` sh
./main_agc_no_delay <hypatia-root-dir>
```

Results will be stored in folder [main_agc_no_delay_run_dir](./main_agc_no_delay_run_dir/).

### With LEO-based Communication

Run AGC simulations impacted by a LEO-based communication network.
From the root folder of this repository, run the following command:

``` sh
./main_agc_satnet <hypatia-root-dir>
```

Results will be stored in folder [main_agc_satnet_run_dir](./main_agc_satnet_run_dir/).

## Content of the Repository

+ Folder [src](./src/): contains the source code for running the Hypatia simulations
+ Folder [stars_agc_scenario](./stars_agc_scenario/): contains the LEO satellite setup used for the simulations, with ground stations corresponding to the control center, the substations, and the STARLINK ground station.
+ Folder [power_system_model_fmu_extracted](./power_system_model_fmu_extracted/): contains the extracted FMU for the power system model

## Funding acknowledgement

<img alt="Logo of the Austrian Research Promotion Agency FFG" src="https://upload.wikimedia.org/wikipedia/en/d/d5/Austrian_FFG_lgo.svg" align="left" style="margin-right: 10px" height="57"/> This work has been funded by the [Austrian Research Promotion Agency FFG](https://www.ffg.at) as part of the **STARS** project under grant agreement FO999914870.
