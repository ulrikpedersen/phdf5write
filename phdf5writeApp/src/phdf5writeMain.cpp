/* phdf5writeMain.cpp */
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <getopt.h>

#include <hdf5.h>

#include <log4cxx/logger.h>
#include <log4cxx/xml/domconfigurator.h>
#include <log4cxx/ndc.h>

#include <osiSock.h>

#include "ByteBuffer2.h"
#include "PVDataTiny.h"
#include "PrintHandler.h"
#include "Channel.h"

#include "server.hpp"
#include "ndarray_hdf5.h"

#include "dimension.h"

using namespace std;

const char *program_name;

typedef struct phdf5_options_t
{
    char* config_xml;
    int base_port;
    int debugger;
    int verbose_level;
    char* log_config_xml;
} phdf5_options_t;

static const struct option phdf5_longopts[] =
{
		{ "help",     no_argument,        NULL, 'h' },
		{ "port",     required_argument,  NULL, 'p' },
		{ "xml",      required_argument,  NULL, 'x' },
		{ "debugger", no_argument,        NULL, 'd' },
		{ "verbose",  no_argument,        NULL, 'v' },
		{ NULL, 0, NULL, 0 }
};

static void phdf5_print_help()
{
    printf("\n Usage: %s [OPTIONS]\n\n", program_name);
    printf(" Run service to receive a stream of image frames and write in parallel to a HDF5 file\n");
    printf("\n"
            "    -h, --help          This help text\n"
            "    -v, --verbose       Print lots of stuff doing operation\n"
    		"    -p, --port=PORT     Use PORT number as the base port\n"
    		"    -x, --xml=FILE      Load XML configuration from FILE\n"
    		"    -d, --debugger      Wait for GDB to connect and set gdb_read=1\n"
    		"    -l, --logger=FILE   Load log4cxx XML configuration from FILE\n"
            );
}

static void phdf5_parse_options(int argc, char *argv[], phdf5_options_t *ptr_options)
{
    int optc;
    int lose = 0;

    /* set some default options */
    ptr_options->config_xml = NULL;
    ptr_options->base_port = 9101;
    ptr_options->debugger = 0;
    ptr_options->verbose_level = 0;
    ptr_options->log_config_xml = NULL;

    while ((optc = getopt_long (argc, argv, "hp:x:dvl:", phdf5_longopts, NULL)) != -1)
        switch (optc)
        {
        case 'h':
        	phdf5_print_help ();
            exit (EXIT_SUCCESS);
            break;
        case 'p':
            ptr_options->base_port = atoi(optarg);
            break;
        case 'x':
            ptr_options->config_xml = (char*)calloc(strlen(optarg)+1, sizeof(char));
            strncpy(ptr_options->config_xml, optarg, strlen(optarg));
            break;
        case 'd':
        	ptr_options->debugger = 1;
        	break;
        case 'v':
        	ptr_options->verbose_level++;
        	break;
        case 'l':
            ptr_options->log_config_xml = (char*)calloc(strlen(optarg)+1, sizeof(char));
            strncpy(ptr_options->log_config_xml, optarg, strlen(optarg));
            break;
        case '?':
            printf("Unknown option -%c. Will be ignored.\n", optopt);
            break;
        default:
            lose = 1;
            break;
        }

    if (lose)
    {
        /* Print error message and exit.  */
        if (optind < argc)
            fprintf (stderr, "%s: extra operand: %s\n",
                    program_name, argv[optind]);
        fprintf (stderr, "Try `%s --help' for more information.\n",
                program_name);
        exit (EXIT_FAILURE);
    }
}

void phdf5_run_writer(phdf5_options_t *ptr_options)
{
	log4cxx::LoggerPtr log( log4cxx::Logger::getLogger("main") );
    int mpi_rank    = 0;
    int mpi_size    = 0;
    int gdb_ready   = 0;
    char hostname[256];
    gethostname(hostname, sizeof(hostname));

#ifdef H5_HAVE_PARALLEL
    LOG4CXX_INFO(log, "Hurrah! We are parallel!");
    int mpi_namelen = 0;
    char mpi_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Info info = MPI_INFO_NULL;

    MPI_Init(0,NULL);
    MPI_Comm_size(MPI_COMM_WORLD,&mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD,&mpi_rank);
    MPI_Get_processor_name(mpi_name,&mpi_namelen);

    LOG4CXX_INFO(log, "MPI info: size=" << mpi_size
                      << " rank=" << mpi_rank
                      <<" cpu=" << mpi_name);

    /* Enable (optional) easy attachment of GDB: Process 0 will loop until a GDB has been attached
     * and set var gdb_ready = 1.
     * The remaining processes will wait at an MPI barrier until then.*/
    if (ptr_options->debugger != 0) {
		if (mpi_rank == 0) {
			LOG4CXX_INFO(log, "PID "<< getpid() << " on "<< hostname
                              <<" ready for attaching GDB");
			LOG4CXX_INFO(log, "Once GDB is attached do: (gdb) set var gdb_ready = 1");
			while (gdb_ready == 0) {
				sleep(2);
			}
		} else {
			LOG4CXX_INFO(log, "Waiting for GDB to attach to process 0 and do \"set var gdb_ready = 1\"");
		}
		MPI_Barrier(MPI_COMM_WORLD );
	}

    NDArrayToHDF5 h5writer(comm,info);
    if (ptr_options->config_xml != NULL) {
        h5writer.load_layout_xml(ptr_options->config_xml);
    }

#else
    LOG4CXX_WARN(log, "Sadly, we are not parallel");

    // Wait for GDB to connect and set gdb_ready=1
    if (ptr_options->debugger != 0) {
		LOG4CXX_INFO(log, "PID "<< getpid() << " on "<< hostname
                          <<" ready for attaching GDB");
		LOG4CXX_INFO(log, "Once GDB is attached do: (gdb) set var gdb_ready = 1");
		while (gdb_ready == 0) {
			sleep(2);
		}
    }
    NDArrayToHDF5 h5writer;
#endif

    int port = ptr_options->base_port + mpi_rank;

    LOG4CXX_INFO(log, "=== Starting server on port: " << port);
    Server server(port);
    server.register_hdf_writer( &h5writer );

    server.run();

#ifdef H5_HAVE_PARALLEL
    LOG4CXX_INFO(log, "Waiting at MPI barrier");
    MPI_Barrier(MPI_COMM_WORLD);
    LOG4CXX_INFO(log, "Finalizing...");
    MPI_Finalize();
#endif

}

int main(int argc,char *argv[])
{
    program_name = argv[0];
    phdf5_options_t options;
    phdf5_parse_options(argc, argv, &options);
    if (options.log_config_xml != NULL) {
    	log4cxx::xml::DOMConfigurator::configure(options.log_config_xml);
    }
    log4cxx::LoggerPtr log( log4cxx::Logger::getLogger("main") );
    //log4cxx::LoggerPtr logi( log4cxx::Logger::getLogger("NDArrayToHDF5") );
    LOG4CXX_DEBUG(log, "Logging configured using: " << options.log_config_xml);
    //LOG4CXX_TRACE(logi, "Logi configured using: " << options.log_config_xml);
    phdf5_run_writer(&options);
    return 0;
}
