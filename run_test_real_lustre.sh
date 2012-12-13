#!/bin/bash

# Start up an MPI job with N processes.
# Argument: number of processes to start. Default: 2

# First some module work to setup the environment
# This is a RHEL6 configuration

module load dasc
module unload dasc
module load openmpi
module load phdf5
module list

echo $( which mpirun )
num_processes=2
if [[ $1 ]] ; then 
    num_processes=$1
fi

host_file="labhosts"
script_path=$(pwd -P)

time \
rederr \
$( which mpirun ) -np ${num_processes} \
    --nooversubscribe \
    --mca btl sm,self,tcp \
    --tag-output \
    --hostfile ${host_file} \
    --bynode \
    ${script_path}/bin/linux-x86_64/test_real_lustre \
    ${script_path}/phdf5writeApp/src/testconf.xml


