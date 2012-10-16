TOP=../..

include $(TOP)/configure/CONFIG

# Set some base paths to MPI, HDF5 and Boost
ifeq ($(EPICS_HOST_ARCH),linux-x86)
MPIDIR=/dls_sw/apps/openmpi/1.4.3/32
#MPIDIR=/dls_sw/work/tools/RHEL6/prefixes/openmpi-1.4.5
# parallel version of hdf5 -can be replaced with non parallel but why would you???
HDF5DIR=/dls_sw/apps/hdf5/1.8.7/openMPI/32
#HDF5DIR=/dls_sw/work/tools/RHEL6/prefixes/hdf5p-1.8.9
BOOSTDIR=/dls_sw/work/tools/RHEL5
#BOOSTDIR=/dls_sw/work/tools/RHEL6/prefixes/boost-1.48.0

else
ifeq ($(EPICS_HOST_ARCH),linux-x86_64)
MPIDIR=/dls_sw/work/tools/RHEL6/prefixes/openmpi-1.4.5
HDF5DIR=/dls_sw/work/tools/RHEL6/prefixes/hdf5p-1.8.9
BOOSTDIR=/dls_sw/work/tools/RHEL6/prefixes/boost-1.48.0
SZDIR=/dls_sw/work/tools/RHEL6/prefixes/szip-2.1
endif
endif

# Directoris where to find these MPI libraries.
mpi_cxx_DIR=$(MPIDIR)/lib
mpi_DIR=$(MPIDIR)/lib
open-rte_DIR=$(MPIDIR)/lib
open-pal_DIR=$(MPIDIR)/lib

# The EPICS support modules lib dir will be listed
# before any of these -L<dir> definitions, causing the linker to pick
# up the areaDetector (non-parallel) hdf5 libs. To work around this,
# the parallel hdf5 libs are installed with a filename prefix of 'p':
# libphdf5.a etc.
phdf5_DIR=$(HDF5DIR)/lib
sz_DIR=$(SZDIR)/lib

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
USR_INCLUDES += -I$(MPIDIR)/include -I/usr/include/libxml2

# Testing: this type of linker flag is defined *after* the support modules lib dirs.
#USR_LDFLAGS += -D__________________BLAH___________
#USR_LDFLAGS+= --library=$(HDF5DIR)/lib/libhdf5

# Install the parallel HDF5 libs directly into this modules lib dir to override
# the hdf5 libs which are supplied with areaDetector
#LIB_INSTALLS+=$(wildcard $(HDF5DIR)/lib/*.a)
#LIB_INSTALLS+=$(wildcard $(HDF5DIR)/lib/*.so)
#LIB_INSTALLS+=$(wildcard $(SZDIR)/lib/*.a)
#LIB_INSTALLS+=$(wildcard $(SZDIR)/lib/*.so)


# -----------------------------------------------------------------------
#  Build a unit test application if we have boost installed somewhere
# -----------------------------------------------------------------------
ifdef BOOSTDIR
boost_unit_test_framework_DIR=$(BOOSTDIR)/lib


PROD_IOC += test_dimension
test_dimension_INCLUDES = -I$(BOOSTDIR)/include
test_dimension_SRCS_DEFAULT += test_dimension.cpp
test_dimension_OBJS += dimension
test_dimension_LIBS += adtransferPlugin asyn ADBase NDPlugin Com
test_dimension_LIBS += boost_unit_test_framework
test_dimension_LDFLAGS += -L$(BOOSTDIR)/lib

PROD_IOC += test_writeconfig
test_writeconfig_INCLUDES = -I$(BOOSTDIR)/include
test_writeconfig_SRCS_DEFAULT += test_writeconfig.cpp
test_writeconfig_OBJS += dimension writeconfig
test_writeconfig_LIBS += adtransferPlugin asyn ADBase NDPlugin Com
test_writeconfig_LIBS += boost_unit_test_framework
test_writeconfig_LDFLAGS += -L$(BOOSTDIR)/lib

PROD_IOC += test_ndarray_hdf5
test_ndarray_hdf5_INCLUDES = -I$(BOOSTDIR)/include
test_ndarray_hdf5_SRCS_DEFAULT += test_ndarray_hdf5.cpp
test_ndarray_hdf5_OBJS += ndarray_hdf5 dimension writeconfig layout layoutxml profiling
test_ndarray_hdf5_LIBS += adtransferPlugin asyn ADBase NDPlugin Com
test_ndarray_hdf5_LIBS += boost_unit_test_framework
#test_ndarray_hdf5_LDFLAGS += -D__________BLUH____________
test_ndarray_hdf5_LDFLAGS += -L$(BOOSTDIR)/lib
test_ndarray_hdf5_LIBS += mpi_cxx mpi open-rte open-pal 
test_ndarray_hdf5_SYS_LIBS += xml2 dl nsl util m dl z
test_ndarray_hdf5_LIBS += phdf5 sz

PROD_IOC += test_mpi_ndarray_hdf5
test_mpi_ndarray_hdf5_INCLUDES = -I$(BOOSTDIR)/include
test_mpi_ndarray_hdf5_SRCS_DEFAULT += test_mpi_ndarray_hdf5.cpp
test_mpi_ndarray_hdf5_OBJS += ndarray_hdf5 dimension writeconfig layout layoutxml profiling
test_mpi_ndarray_hdf5_LIBS += adtransferPlugin asyn ADBase NDPlugin Com
test_mpi_ndarray_hdf5_LIBS += boost_unit_test_framework
test_mpi_ndarray_hdf5_LDFLAGS += -L$(BOOSTDIR)/lib
test_mpi_ndarray_hdf5_LIBS += mpi_cxx mpi open-rte open-pal 
test_mpi_ndarray_hdf5_SYS_LIBS += xml2 dl nsl util m dl z
test_mpi_ndarray_hdf5_LIBS += phdf5 sz

PROD_IOC += test_real_lustre
test_real_lustre_INCLUDES = -I$(BOOSTDIR)/include
test_real_lustre_SRCS_DEFAULT += test_real_lustre.cpp
test_real_lustre_OBJS += ndarray_hdf5 dimension writeconfig layout layoutxml profiling
test_real_lustre_LIBS += adtransferPlugin asyn ADBase NDPlugin Com
#test_real_lustre_LIBS += boost_unit_test_framework
#test_real_lustre_LDFLAGS += -L$(BOOSTDIR)/lib
test_real_lustre_LIBS += mpi_cxx mpi open-rte open-pal 
test_real_lustre_SYS_LIBS += xml2 dl nsl util m dl z
test_real_lustre_LIBS += phdf5 sz

PROD_IOC += test_layout
test_layout_INCLUDES = -I$(BOOSTDIR)/include
test_layout_SRCS_DEFAULT += test_layout.cpp
test_layout_OBJS += layout
test_layout_LIBS += boost_unit_test_framework
test_layout_LDFLAGS += -L$(BOOSTDIR)/lib

PROD_IOC += test_layoutxml
test_layoutxml_INCLUDES = -I$(BOOSTDIR)/include -I/usr/include/libxml2
test_layoutxml_SRCS_DEFAULT += test_layoutxml.cpp
test_layoutxml_OBJS += layoutxml layout
test_layoutxml_LIBS += boost_unit_test_framework
test_layoutxml_SYS_LIBS += xml2 m z
test_layoutxml_LDFLAGS += -L$(BOOSTDIR)/lib

PROD_IOC += test_profiling
test_profiling_INCLUDES = -I$(BOOSTDIR)/include
test_profiling_SRCS_DEFAULT += test_profiling.cpp
test_profiling_OBJS += profiling
test_profiling_LIBS += boost_unit_test_framework
test_profiling_LDFLAGS += -L$(BOOSTDIR)/lib

endif

# ------------------------
# Build The Application
# ------------------------

PROD_IOC += phdf5write

# These two lines are needed for non-vxWorks builds, such as Linux
phdf5write_SRCS_DEFAULT += phdf5writeMain.cpp

# Add locally compiled object code
phdf5write_SRCS += server.cpp
phdf5write_SRCS += dimension.cpp
phdf5write_SRCS += writeconfig.cpp
phdf5write_SRCS += ndarray_hdf5.cpp
phdf5write_SRCS += layout.cpp
phdf5write_SRCS += layoutxml.cpp
phdf5write_SRCS += profiling.cpp

# The following adds object code from base/src/vxWorks
phdf5write_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Link against the MPI and HDF5 libraries. This combination of libraries was found by running
# h5pcc -show
# mpicc -showme:compile
# mpicc -showme:link
phdf5write_LIBS += mpi_cxx mpi open-rte open-pal 
phdf5write_SYS_LIBS += xml2 dl nsl util m dl z
phdf5write_LIBS += phdf5 sz


# The adtransferPlugin has the ADTransferServer class
phdf5write_LIBS += adtransferPlugin
# AreaDetector libraries and dependencies to provide the NDArray class
phdf5write_LIBS += asyn ADBase NDPlugin

# We need only need to link this application against the EPICS Base Com library
phdf5write_LIBS += Com

# ---------------------------------------------------

include $(TOP)/configure/RULES
