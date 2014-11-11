#!/bin/bash

# Start up an MPI job with N processes.
# Argument: number of processes to start. Default: 2

# First some module work to setup the environment
# This is a RHEL6 configuration

module load dasc
module unload dasc
module load openmpi
module load phdf5
#module load boost
module list
echo $( which mpirun )
num_processes=2
if [[ $1 ]] ; then 
    num_processes=$1
fi

./rederr \
$( which mpirun ) -np ${num_processes} \
    --mca btl sm,self,tcp \
	--tag-output \
	bin/linux-x86_64/test_mpi_ndarray_hdf5 \
	--log_level=test_suite --run_test=ParallelRun

