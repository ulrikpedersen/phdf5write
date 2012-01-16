TOP=../..

include $(TOP)/configure/CONFIG

MPIDIR=/dls_sw/apps/openmpi/1.4.3/32
HDF5DIR=/dls_sw/apps/hdf5/1.8.7/openMPI/32
BOOSTDIR=/scratch/boost/1_48_0

# C++ compiler flags found by running the command: h5pcc -show
USR_CXXFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
HOST_OPT=NO

# Because areaDetector also supplies HDF5 header files -but not the parallel ones
# we need to bring the HDF5 include forward in the command line arguments so it will
# take precedece over the areaDetector includes.
# This can be forced by using USR_CXXFLAGS instead of USR_INCLUDES
#USR_INCLUDES += -I$(HDF5DIR)/include
USR_CXXFLAGS += -I$(HDF5DIR)/include

# Point to MPI header files
USR_INCLUDES += -I$(MPIDIR)/include


# -------------------------------
# Build a Diamond Support Module
# -------------------------------

#LIBRARY_IOC += xxx

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxx.h

# xxxRecord.dbd will be installed into <top>/dbd
#DBD += xxx.dbd

# The following are compiled and added to the support library
#xxx_SRCS += xxxCodeA.c
#xxx_SRCS += xxxCodeB.c

# We need to link against the EPICS Base libraries
#xxx_LIBS += $(EPICS_BASE_IOC_LIBS)


# -----------------------------------------------------------------------
#  Build a unit test application if we have boost installed somewhere
# -----------------------------------------------------------------------
ifdef BOOSTDIR
PROD_IOC += test_dimension
test_dimension_INCLUDES = -I$(BOOSTDIR)/include
test_dimension_SRCS_DEFAULT += test_dimension.cpp
test_dimension_OBJS += dimension
test_dimension_LIBS += adtransferPlugin asyn ADBase NDPlugin Com
test_dimension_LIBS += boost_unit_test_framework
boost_unit_test_framework_DIR=$(BOOSTDIR)/lib
test_dimension_LDFLAGS += -L$(BOOSTDIR)/lib

PROD_IOC += test_writeconfig
test_writeconfig_INCLUDES = -I$(BOOSTDIR)/include
test_writeconfig_SRCS_DEFAULT += test_writeconfig.cpp
test_writeconfig_OBJS += dimension writeconfig
test_writeconfig_LIBS += adtransferPlugin asyn ADBase NDPlugin Com
test_writeconfig_LIBS += boost_unit_test_framework
boost_unit_test_framework_DIR=$(BOOSTDIR)/lib
test_writeconfig_LDFLAGS += -L$(BOOSTDIR)/lib

endif

# ------------------------
# Build an Application
# ------------------------

PROD_IOC += phdf5write

# These two lines are needed for non-vxWorks builds, such as Linux
phdf5write_SRCS_DEFAULT += phdf5writeMain.cpp

# Add locally compiled object code
phdf5write_SRCS += server.cpp
phdf5write_SRCS += dimension.cpp
phdf5write_SRCS += writeconfig.cpp
phdf5write_SRCS += ndarray_hdf5.cpp

# The following adds object code from base/src/vxWorks
phdf5write_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Link against the MPI and HDF5 libraries. This combination of libraries was found by running
# h5pcc -show
# mpicc -showme:compile
# mpicc -showme:link
phdf5write_LIBS += mpi_cxx mpi open-rte open-pal 
phdf5write_SYS_LIBS += dl nsl util m dl z
phdf5write_LIBS += hdf5 sz

# Directoris where to find these MPI and HDF5 libraries.
# TODO: this is currently only looking at the 32bit parallel libraries but
#       must eventually use the 64 bit ones as well
mpi_cxx_DIR=$(MPIDIR)/lib
mpi_DIR=$(MPIDIR)/lib
open-rte_DIR=$(MPIDIR)/lib
open-pal_DIR=$(MPIDIR)/lib
hdf5_DIR=$(HDF5DIR)/lib
sz_DIR=$(HDF5DIR)/lib

# The adtransferPlugin has the ADTransferServer class
phdf5write_LIBS += adtransferPlugin
# AreaDetector libraries and dependencies to provide the NDArray class
phdf5write_LIBS += asyn ADBase NDPlugin


# We need only need to link this application against the EPICS Base Com library
#phdf5write_LIBS += $(EPICS_BASE_IOC_LIBS)
phdf5write_LIBS += Com

# ---------------------------------------------------

include $(TOP)/configure/RULES
