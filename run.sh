MPIRUN=$(which mpirun)
script_path=$(pwd -P)

echo $MPIRUN
num_processes=6
if [[ $1 ]] ; then 
    num_processes=$1
fi

# Run the MPI processes ready to receive from excalibur
$script_path/rederr \
$MPIRUN -np ${num_processes} \
    --nooversubscribe --mca btl sm,self,tcp \
    --mca btl_tcp_if_include em1 \
    --tag-output \
    --hostfile targethosts \
    --bynode \
    bin/linux-x86_64/phdf5write -h

