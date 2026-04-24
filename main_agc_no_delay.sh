#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo
    echo "Usage: main_agc_no_delay <hypatia-root-dir>"
    echo
    echo "The path to the Hypatia root directory can be relative to the current working directory or absolute."
    echo
    exit 1
fi

# Current working directory
CWD=${PWD} 

# Path containing this script
APP_DIR="$(dirname $(readlink -f $0))"

# Path to simulation run directory
RUN_DIR=${APP_DIR}/main_agc_no_delay_run_dir

# Path to log output  directory
LOG_DIR=${RUN_DIR}/logs_ns3

# Path to Hypatia root directory
if [[ "$1" = /* ]]; then
  # Absolute path
  HYPATIA_ROOT_DIR=$1
else
  # Relative path
  HYPATIA_ROOT_DIR=${CWD}/$1
fi

# Remove old logs and results
if [ -d "$LOG_DIR" ]; then 
  cd ${LOG_DIR}
  rm -rf *.*
else
  mkdir ${LOG_DIR}
fi

# Copy sources for the simulation main executable
cp -R ${APP_DIR}/src/main_agc_no_delay/ ${HYPATIA_ROOT_DIR}/ns3-sat-sim/simulator/scratch

# Run the simulation
cd ${HYPATIA_ROOT_DIR}/ns3-sat-sim/simulator; 
./waf --run="main_agc_no_delay --run_dir='${RUN_DIR}'" 2>&1 | tee ${LOG_DIR}/console.txt

# Change back to previous working directory
cd ${CWD}
