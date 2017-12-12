COMMON_REPO := ../
#Common Includes
include $(COMMON_REPO)/utility/boards.mk
include $(COMMON_REPO)/libs/xcl2/xcl2.mk
include $(COMMON_REPO)/libs/opencl/opencl.mk

# Host Application
host_SRCS=./src/host.cpp $(xcl2_SRCS)
host_HDRS=$(xcl2_HDRS)
host_CXXFLAGS=-I./src/ $(xcl2_CXXFLAGS) $(opencl_CXXFLAGS)
host_LDFLAGS=$(opencl_LDFLAGS)

# Kernel
ksw_ext2_SRCS=./src/ksw_ext2.cpp
ksw_ext2_CLFLAGS=-k ksw_ext2

XOS=ksw_ext2

ksw_ext2_XOS=ksw_ext2

EXES=host
XCLBINS=ksw_ext2

# check
check_EXE=host
check_XCLBINS=ksw_ext2

CHECKS=check

include $(COMMON_REPO)/utility/rules.mk
