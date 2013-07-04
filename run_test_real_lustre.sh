#!/bin/bash

# Start up an MPI job with N processes.
# Argument: number of processes to start. Default: 2

# First some module work to setup the environment
# This is a RHEL6 configuration

MPIRUN=$(which mpirun)
script_path=$(pwd -P)

echo $MPIRUN
num_processes=2
if [[ $1 ]] ; then 
    num_processes=$1
fi

#host_file="labhosts"
host_file="targethosts"
#host_file="localhosts"
#host_file=officehosts

time \
$script_path/rederr \
$MPIRUN -np ${num_processes} \
    --nooversubscribe \
    --mca btl sm,self,tcp \
    --mca btl_tcp_if_include em1 \
    --tag-output \
    --hostfile ${host_file} \
    --bynode \
    ${script_path}/bin/linux-x86_64/test_real_lustre \
    ${script_path}/phdf5writeApp/src/testconf.xml


