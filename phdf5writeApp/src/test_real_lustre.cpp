/** \cond
 * test_real_lustre.cpp
 *
 *  Created on: 14 Feb 2012
 *      Author: up45
 */

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/foreach.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <exception>
#include <NDArray.h>
#include "ndarray_hdf5.h"

using namespace std;

typedef struct dims_t{
    int x;
    int y;
    int z;
} dims_t;

struct sim_config
{
    std::string filename;
    dims_t chunking;
    dims_t subframe;
    int num_frames;
    int num_chunks;
    int fill_value;
    bool extendible;
    bool ioposix;
    bool collective;
    void loadxml(const std::string &xmlfile);
};

void sim_config::loadxml(const std::string &xmlfile)
{

    // Create empty property tree object
    using boost::property_tree::ptree;
    ptree pt;

    // Load XML file and put its contents in property tree.
    read_xml(xmlfile, pt);

    // Get the output filename and store it.
    filename = pt.get<std::string>("phdftest.filename");

    // Get chunking settings
    chunking.x = pt.get("phdftest.chunking.x", 1);
    chunking.y = pt.get("phdftest.chunking.y", 1);
    chunking.z = pt.get("phdftest.chunking.z", 1);

    // get the subframe dimensions -i.e. the size of the stripe that the current process will write.
    // Each process will write with an offset in the Y dimension, depending on their mpi-rank (process ID)
    subframe.x = pt.get("phdftest.subframe.x", 1);
    subframe.y = pt.get("phdftest.subframe.y", 1);
    subframe.z = pt.get("phdftest.subframe.z", 1);

    // Get the number of frames to write
    num_frames = pt.get("phdftest.numframes", 1);

    // fill value
    fill_value = pt.get("phdftest.fillvalue", 0);

    // Do collective IO
    collective = (bool)pt.get("phdftest.collective", 1);

    // Do MPI + posix IO
    ioposix = (bool)pt.get("phdftest.ioposix", 0);

    // Use extendible dataset
    extendible = (bool)pt.get("phdftest.extendible", 1);

}


void util_fill_ndarr_dims(NDArray &ndarr, unsigned long int *sizes, int ndims)
{
    static short counter = 3;
    int i=0;
    ndarr.ndims = ndims;
    int num_elements = 1;
    for (i=0; i<ndims; i++) {
    	// Note: the NDArray dimensions are reverse order in relation to
    	//       the HDF5 dataset dimensionality. sigh....
        ndarr.initDimension(&(ndarr.dims[ndims-i-1]), sizes[i]);
        num_elements *= sizes[i];
    }
    ndarr.pData = calloc(num_elements, sizeof(short));
    short *ptrdata = (short*)ndarr.pData;
    ptrdata[0] = counter++; ptrdata[1] = counter++;
}


struct Fixture{
    std::string fname;
    std::vector<NDArray *> frames;
    unsigned long int sizes[2];
    unsigned long int chunks[3];
    unsigned long int dsetdims[3];
    unsigned long int i;
    int numframes;
    int fill_value;
    int zero;
    int yoffset;

    MPI_Comm mpi_comm;
    char mpi_name[100];
    int mpi_name_len;
    int mpi_size;
    int mpi_rank;
    sim_config config;


    Fixture(char *configxml)
    {
        cout << "MAIN: Setup Fixture" << endl;
        //fname = "smallframes.h5";
        config.loadxml(configxml);
        zero=0;
        mpi_size = 1;
        mpi_rank = 0;
        mpi_name_len = 100;
#ifdef H5_HAVE_PARALLEL

        mpi_comm = MPI_COMM_WORLD;

        MPI_Init(NULL, NULL);

        MPI_Comm_size(mpi_comm, &mpi_size);
        MPI_Comm_rank(mpi_comm, &mpi_rank);

        MPI_Get_processor_name(mpi_name, &mpi_name_len);
        cout << "MAIN: === TEST is parallel ==="<<endl;
        cout << "MAIN:   MPI size: " << mpi_size << " MPI rank: " << mpi_rank << " host: " << mpi_name <<endl;


#endif
        yoffset= config.subframe.y * mpi_rank;

        // output filename
        fname = config.filename;

        // Define the dimensions and number of frames to run here
        //numframes = NUM_FRAMES;
        numframes = config.num_frames;
        cout << "MAIN: num_frames: " << numframes<<endl;

        //sizes[0]=CHUNK_X; sizes[1]=CHUNK_Y * NUM_CHUNKS;
        sizes[0] = config.subframe.y;
        sizes[1] = config.subframe.x;

        //chunks[0]=CHUNK_X; chunks[1]=CHUNK_Y; chunks[2]=CHUNK_Z;
        chunks[0] = config.chunking.z;
        chunks[1] = config.chunking.y;
        chunks[2] = config.chunking.x;

        dsetdims[0]=numframes;
        dsetdims[1]=sizes[0]*mpi_size;
        dsetdims[2]=sizes[1];

        cout << "MAIN: dataset: " << dsetdims[0] << " " << dsetdims[1] << " " << dsetdims[2]<<endl;
        fill_value = config.fill_value;

        cout << "MAIN: chunks: " << chunks[0] << " " << chunks[1] << " " << chunks[2]<<endl;
        cout << "MAIN: fill value: " << fill_value<<endl;

        for (   i=0UL;
        		i<chunks[0];
        		i++)
        {
            NDArray *pnd = new NDArray();
            pnd->dataType = NDUInt16;
            pnd->pAttributeList->add("h5_chunk_size_0", "dimension 0", NDAttrUInt32, (void*)(chunks) );
            pnd->pAttributeList->add("h5_chunk_size_1", "dimension 1", NDAttrUInt32, (void*)(chunks+1) );
            pnd->pAttributeList->add("h5_chunk_size_2", "dimension 2", NDAttrUInt32, (void*)(chunks+2) );

            pnd->pAttributeList->add("h5_dset_size_0", "dset 0", NDAttrUInt32, (void*)(dsetdims) );
            pnd->pAttributeList->add("h5_dset_size_1", "dset 1", NDAttrUInt32, (void*)(dsetdims+1) );
            pnd->pAttributeList->add("h5_dset_size_2", "dset 2", NDAttrUInt32, (void*)(dsetdims+2) );

            pnd->pAttributeList->add("h5_roi_origin_0", "offset 0", NDAttrUInt32, (void*)&zero );
            pnd->pAttributeList->add("h5_roi_origin_1", "offset 1", NDAttrUInt32, (void*)&yoffset );
            pnd->pAttributeList->add("h5_roi_origin_2", "offset 2", NDAttrUInt32, (void*)&zero );

            pnd->pAttributeList->add("h5_fill_val", "fill value", NDAttrUInt32, (void*)(&fill_value) );
            frames.push_back( pnd );
            util_fill_ndarr_dims( *frames[i], sizes, 2);

            //pnd->report(11);
        }
    }

    ~Fixture()
    {
        cout << "MAIN: teardown Fixture" << endl;
        //std::vector<NDArray*>::const_iterator it;
        //NDArray* pnd;
        //for (it = frames.begin(); it != frames.end(); ++it)
        //{
        //    pnd = *it;
        //    if (pnd->pData !=NULL ) {
                //cout << "MAIN: Freeing address: " << pnd->pData << " from: " << &pnd << endl;
                //free(pnd->pData );
                //pnd->pData=NULL;
         //   }
        //}
        //frames.clear();

#ifdef H5_HAVE_PARALLEL
        cout << "MAIN: ==== MPI_Finalize  rank: " << mpi_rank << "=====" << endl;
        MPI_Finalize();
#endif

    }
};


int main(int argc, char *argv[])
{
    char * configfile = argv[1];
    cout << "MAIN: using config file: " << configfile << endl;
    struct Fixture fixt(configfile);
    NDArray * pndarray;


#ifdef H5_HAVE_PARALLEL

    cout << "MAIN: === TEST ParallelNoAttr is parallel ==="<<endl;
    //cout << "MAIN: Creating NDArrayToHDF5 object."<<endl;
    NDArrayToHDF5 ndh(fixt.mpi_comm, MPI_INFO_NULL);

#else
    cout << "MAIN: === TEST ParallelNoAttr is *not* parallel ==="<<endl;
    cout << "MAIN: Creating NDArrayToHDF5 object."<<endl;
    NDArrayToHDF5 ndh;
#endif

    ndh.h5_configure(*fixt.frames[0]);
    //fixt.frames[0]->report(100);
    ndh.get_conf_ref().io_collective(fixt.config.collective);
    ndh.get_conf_ref().io_mpiposix(fixt.config.ioposix);
    ndh.get_conf_ref().dset_extendible(fixt.config.extendible);
    cout << "MAIN: WriteConfig: " << ndh.get_conf_ref() << endl;
    //cout << "\n\tposix=" << fixt.config.ioposix << " collective=" << fixt.config.collective << " extendible=" << fixt.config.extendible << endl;


    cout << "MAIN: Open file: " << fixt.fname<<endl;
    ndh.h5_open(fixt.fname.c_str());

    int cacheframe = 0;

    for (int i = 0; i < fixt.numframes; i++) {

        cacheframe = i%fixt.frames.size();
        cout << "MAIN: ===== Writing frame[" << cacheframe << "] no: " << i << endl;
        pndarray = fixt.frames[cacheframe];
        pndarray->pAttributeList->remove("h5_roi_origin_0");
        pndarray->pAttributeList->add("h5_roi_origin_0", "offset 0", NDAttrUInt32, (void*)&i );
        //pndarray->pAttributeList->report(100);
        ndh.h5_write( *pndarray );



    }

    cout << "MAIN: Closing file" << endl;;
    ndh.h5_close();
}
/** \endcond */
