/** \cond
 * benchmark.cpp
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

#include <log4cxx/logger.h>
#include <log4cxx/xml/domconfigurator.h>


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
    int alloc_time;
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

    // Define the dataset allocation time
    alloc_time = pt.get("phdftest.alloc_time", 0);

}


void util_fill_ndarr_dims(NDArray &ndarr, unsigned long int *sizes, int ndims)
{
	log4cxx::LoggerPtr log( log4cxx::Logger::getLogger("main") );
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
    LOG4CXX_TRACE(log, "Filling array with dummy data...");
    for (i=0; i<num_elements; i++) {
    	*((short*)ndarr.pData + i) = i;
    }

    short *ptrdata = (short*)ndarr.pData;
    ptrdata[0] = counter++; ptrdata[1] = counter++;
    LOG4CXX_TRACE(log, "Done filling array!");
}


struct Fixture{
	log4cxx::LoggerPtr log;
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


    Fixture(const string& configxml)
    {
    	log = log4cxx::Logger::getLogger("main");
    	LOG4CXX_DEBUG(log, "Setup Fixture");
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
        LOG4CXX_INFO(log, "==== MPI size: " << mpi_size << " MPI rank: " << mpi_rank << " host: " << mpi_name);


#endif
        yoffset= config.subframe.y * mpi_rank;

        // output filename
        fname = config.filename;

        // Define the dimensions and number of frames to run here
        //numframes = NUM_FRAMES;
        numframes = config.num_frames;
        LOG4CXX_INFO(log, "num_frames: " << numframes);

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

        LOG4CXX_INFO(log, "dataset: " << dsetdims[0] << " " << dsetdims[1] << " " << dsetdims[2]);
        fill_value = config.fill_value;

        LOG4CXX_INFO(log, "chunks: " << chunks[0] << " " << chunks[1] << " " << chunks[2]);
        LOG4CXX_INFO(log, "fill value: " << fill_value);

        for (   i=0UL;
        		i<chunks[0];
        		i++)
        {
            NDArray *pnd = new NDArray();
            pnd->dataType = NDUInt16;
            pnd->pAttributeList["h5_chunk_size_0"] = new NDAttribute(std::string("dimension 0"),
            		std::string(""), NDAttrSourceDriver, std::string(""),NDAttrUInt32, (void*)(chunks));
            pnd->pAttributeList["h5_chunk_size_1"] = new NDAttribute(std::string("dimension 1"),
            		std::string(""), NDAttrSourceDriver, std::string(""), NDAttrUInt32, (void*)(chunks+1) );
            pnd->pAttributeList["h5_chunk_size_2"] = new NDAttribute(std::string("dimension 2"),
            		std::string(""), NDAttrSourceDriver, std::string(""), NDAttrUInt32, (void*)(chunks+2) );

            pnd->pAttributeList["h5_dset_size_0"] = new NDAttribute(std::string("dset 0"),
            		std::string(""), NDAttrSourceDriver, std::string(""), NDAttrUInt32, (void*)(dsetdims) );
            pnd->pAttributeList["h5_dset_size_1"] = new NDAttribute(std::string("dset 1"),
            		std::string(""), NDAttrSourceDriver, std::string(""), NDAttrUInt32, (void*)(dsetdims+1) );
            pnd->pAttributeList["h5_dset_size_2"] = new NDAttribute(std::string("dset 2"),
            		std::string(""), NDAttrSourceDriver, std::string(""), NDAttrUInt32, (void*)(dsetdims+2) );

            pnd->pAttributeList["h5_roi_origin_0"] = new NDAttribute(std::string("offset 0"),
            		std::string(""), NDAttrSourceDriver, std::string(""), NDAttrUInt32, (void*)&zero );
            pnd->pAttributeList["h5_roi_origin_1"] = new NDAttribute(std::string("offset 1"),
            		std::string(""), NDAttrSourceDriver, std::string(""), NDAttrUInt32, (void*)&yoffset );
            pnd->pAttributeList["h5_roi_origin_2"] = new NDAttribute(std::string("offset 2"),
            		std::string(""), NDAttrSourceDriver, std::string(""), NDAttrUInt32, (void*)&zero );

            pnd->pAttributeList["h5_fill_val"] = new NDAttribute(std::string("fill value"),
            		std::string(""), NDAttrSourceDriver, std::string(""), NDAttrUInt32, (void*)(&fill_value) );
            frames.push_back( pnd );
            util_fill_ndarr_dims( *frames[i], sizes, 2);

            //pnd->report(11);
        }
    }

    ~Fixture()
    {
    	LOG4CXX_DEBUG(log, "teardown Fixture");
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
    	LOG4CXX_DEBUG(log, "==== MPI_Finalize  rank: " << mpi_rank << "=====");
        MPI_Finalize();
#endif

    }
};


int main(int argc, char *argv[])
{
    if (argc < 4) {
		cerr << "Not enough arguments supplied. " << endl
		     << "Must provide 3 configuration files: " << endl
		     << "    testconfig.xml layout.xml and log4cxxconfig.xml" << endl;
		exit(-1);
	}
    string configfile(argv[1]);
    string layoutfile(argv[2]);
    string logconfigfile(argv[3]);

    log4cxx::xml::DOMConfigurator::configure(logconfigfile);
    log4cxx::LoggerPtr log( log4cxx::Logger::getLogger("main") );

    LOG4CXX_INFO(log, "Using config file: " << configfile );
    struct Fixture fixt(configfile);
    NDArray * pndarray;
    unsigned long nbytes = sizeof(short) * fixt.numframes * fixt.sizes[0]  * fixt.sizes[1];

    Profiling opentime;
    opentime.reset(nbytes);
#ifdef H5_HAVE_PARALLEL

    LOG4CXX_DEBUG(log, " === TEST ParallelNoAttr is parallel === ");
    //LOG4CXX_DEBUG(log, "Creating NDArrayToHDF5 object.");
    NDArrayToHDF5 ndh(fixt.mpi_comm, MPI_INFO_NULL);

#else
    LOG4CXX_DEBUG(log, "=== TEST ParallelNoAttr is *not* parallel ===");
    LOG4CXX_DEBUG(log, "Creating NDArrayToHDF5 object.");
    NDArrayToHDF5 ndh;
#endif
    ndh.load_layout_xml(layoutfile);

    ndh.h5_configure(*fixt.frames[0]);
    //fixt.frames[0]->report(100);
    ndh.get_conf_ref().io_collective(fixt.config.collective);
    ndh.get_conf_ref().io_mpiposix(fixt.config.ioposix);
    ndh.get_conf_ref().dset_extendible(fixt.config.extendible);
    ndh.get_conf_ref().set_alloc_time(fixt.config.alloc_time);

    LOG4CXX_DEBUG(log, "WriteConfig: " << ndh.get_conf_ref()._str_());
    LOG4CXX_INFO(log, "posix=" << fixt.config.ioposix << " collective=" << fixt.config.collective << " extendible=" << fixt.config.extendible);


    LOG4CXX_INFO(log, "Open file: " << fixt.fname);
    ndh.h5_open(fixt.fname.c_str());
    double dt_open = opentime.stamp_now();

    Profiling writetime;
    writetime.reset(nbytes);

    int cacheframe = 0;
    Profiling update_timer;
    update_timer.reset(0);
    for (int i = 0; i < fixt.numframes; i++) {

        cacheframe = i%fixt.frames.size();
        LOG4CXX_DEBUG(log, "===== Writing frame[" << cacheframe << "] no: " << i);
        pndarray = fixt.frames[cacheframe];
        pndarray->pAttributeList.erase(pndarray->pAttributeList.find("h5_roi_origin_0"));
        pndarray->pAttributeList["h5_roi_origin_0"] = new NDAttribute(std::string("offset 0"),
        		std::string(""), NDAttrSourceDriver, std::string(""), NDAttrUInt32, (void*)&i );
        //pndarray->pAttributeList->report(100);
        ndh.h5_write( *pndarray );
        if (update_timer.stamp_now() > 5.0) {
            LOG4CXX_INFO( log, "Frame #" << i);
            update_timer.reset(0);
        }
    }
    double dt_write = writetime.stamp_now();

    // Testing the H5close functions to shut down the hdf library
//    herr_t hdfreturn;
//    LOG4CXX_INFO(log, "Closing down HDF5 library");
//    hdfreturn = H5close();
//    LOG4CXX_INFO(log, "Closed down HDF5 library: " << hdfreturn );
//    LOG4CXX_INFO(log, "Reopening HDF5 library");
//    hdfreturn = H5open();
//    LOG4CXX_INFO(log, "Opened HDF5 library: " << hdfreturn );

    LOG4CXX_DEBUG(log, "Closing file");
    Profiling closetime;

    closetime.reset(nbytes);
    ndh.h5_close();
    double dt_close = closetime.stamp_now();

    double writerate = (nbytes/(1024. * 1024.)) / dt_write;
    LOG4CXX_INFO(log, "    Open  time = " << dt_open << "s");
    LOG4CXX_INFO(log, "    Write time = " << dt_write << "s" << " [" << writerate << "MB/s]");
    LOG4CXX_INFO(log, "    Close  time = " << dt_close << "s");
}
/** \endcond */
