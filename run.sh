# Run the six MPI processes ready to receive from excalibur
./rederr /dls_sw/prod/tools/RHEL6-x86_64/openmpi/1-4-5/prefix/bin/mpirun \
    -np 6 --nooversubscribe --mca btl sm,self,tcp \
    --mca btl_tcp_if_include em1 \
    --tag-output \
    --hostfile targethosts \
    --bynode \
    bin/linux-x86_64/phdf5write

