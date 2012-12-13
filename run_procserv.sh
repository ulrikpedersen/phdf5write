#!/bin/bash

startup_script="run_test_multinode.sh"
if [[ $1 ]] ; then
    startup_script=$1
fi

num_processes=2
if [[ $2 ]] ; then
    num_processes=$2
fi

script_path=$( pwd -P )
telnet_port=9000

# For RHEL6 we need to load a module
module load procServ

procServ \
    --chdir ${script_path} \
	-d -w --noautorestart \
	${telnet_port} \
	/bin/bash ${startup_script} ${num_processes}

