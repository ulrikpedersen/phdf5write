/* phdf5writeMain.cpp */

#include <cstdlib>
#include <iostream>
#include <vector>

#include <hdf5.h>

#include "server.hpp"
#include "ndarray_hdf5.h"

#include "dimension.h"

using namespace std;


void test_mpi_simple()
{
    int mpi_rank    = 0;
    int mpi_size    = 0;

#ifdef H5_HAVE_PARALLEL
    cout << "Hurrah! We are parallel!" << endl;
    int mpi_namelen = 0;
    char mpi_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Info info = MPI_INFO_NULL;

    MPI_Init(0,NULL);
    MPI_Comm_size(MPI_COMM_WORLD,&mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD,&mpi_rank);
    MPI_Get_processor_name(mpi_name,&mpi_namelen);

    cout << "MPI info: size=" << mpi_size;
    cout << " rank=" << mpi_rank;
    cout << " cpu=" << mpi_name;
    cout << endl;

    NDArrayToHDF5 h5writer(comm,info);
#else
    cout << "Sadly, we are not parallel" << endl;
    NDArrayToHDF5 h5writer;
#endif
    int baseport = 8001;
    int port = baseport + mpi_rank;

    cout << "=== Starting server on port: " << port << endl;
    Server server(port);
    server.register_hdf_writer( &h5writer );

    server.run();

#ifdef H5_HAVE_PARALLEL
    cout << "Waiting at MPI barrier" << endl;
    MPI_Barrier(MPI_COMM_WORLD);
    cout << "Finalizing..." << endl;
    MPI_Finalize();
#endif

}

int main(int argc,char *argv[])
{
    cout << "==== Parallel HDF5 writer ====" << endl;
    //Server server(8001);
    //server.run();
    //test_dimensiondesc_simple();
    test_mpi_simple();

    return 0;
}
