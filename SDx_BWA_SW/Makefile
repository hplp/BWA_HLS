# This file was developed by Xilinx
# Original:
# https://github.com/Xilinx/SDAccel_Examples/blob/master/getting_started/host/hbm_bandwidth/Makefile
# copy 

.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make all TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform>"
	$(ECHO) "      Command to generate the design for specified Target and Device."
	$(ECHO) ""
	$(ECHO) "  make clean "
	$(ECHO) "      Command to remove the generated non-hardware files."
	$(ECHO) ""
	$(ECHO) "  make cleanall"
	$(ECHO) "      Command to remove all the generated files."
	$(ECHO) ""
	$(ECHO) "  make check TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform>"
	$(ECHO) "      Command to run application in emulation."
	$(ECHO) ""
	$(ECHO) "  make build TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform>"
	$(ECHO) "      Command to build xclbin application."
#	$(ECHO) ""
#	$(ECHO) "  make run_nimbix DEVICE=<FPGA platform>"
#	$(ECHO) "      Command to run application on Nimbix Cloud. [untested]"
#	$(ECHO) ""
#	$(ECHO) "  make aws_build DEVICE=<FPGA platform>"
#	$(ECHO) "      Command to build AWS xclbin application on AWS Cloud. [untested]"
#	$(ECHO) ""

# Points to Utility Directory
COMMON_REPO = .
ABS_COMMON_REPO = $(shell readlink -f $(COMMON_REPO))

TARGETS := hw
TARGET := $(TARGETS)
DEVICE := $(DEVICES)
XCLBIN := ./xclbin

include ./utils.mk

DSA := $(call device2dsa, $(DEVICE))
BUILD_DIR := ./_x.$(TARGET).$(DSA)

BUILD_DIR_ksw_ext2 = $(BUILD_DIR)/ksw_ext2

CXX := $(XILINX_SDX)/bin/xcpp
XOCC := $(XILINX_SDX)/bin/xocc

#Include Libraries
include $(ABS_COMMON_REPO)/libs/opencl/opencl.mk
include $(ABS_COMMON_REPO)/libs/xcl2/xcl2.mk
CXXFLAGS += $(xcl2_CXXFLAGS)
LDFLAGS += $(xcl2_LDFLAGS)
HOST_SRCS += $(xcl2_SRCS)
CXXFLAGS += $(opencl_CXXFLAGS) -Wall -O0 -g -std=c++14
LDFLAGS += $(opencl_LDFLAGS)

HOST_SRCS += src/host.cpp

# Host compiler global settings
CXXFLAGS += -fmessage-length=0
LDFLAGS += -lrt -lstdc++ 

# Kernel compiler global settings
CLFLAGS += -j 8 -t $(TARGET) --platform $(DEVICE) --save-temps --xp prop:solution.kernel_compiler_margin=0.1 --kernel_frequency 400

# Kernel linker flags
#LDCLFLAGS += --sp ksw_ext2_1.m_axi_gmem:HBM[0] --sp ksw_ext2_2.m_axi_gmem:HBM[1] --sp ksw_ext2_3.m_axi_gmem:HBM[2] --sp ksw_ext2_4.m_axi_gmem:HBM[3] --sp ksw_ext2_5.m_axi_gmem:HBM[4] --sp ksw_ext2_6.m_axi_gmem:HBM[5] --sp ksw_ext2_7.m_axi_gmem:HBM[6] --sp ksw_ext2_8.m_axi_gmem:HBM[7] --sp ksw_ext2_9.m_axi_gmem:HBM[8] --sp ksw_ext2_10.m_axi_gmem:HBM[9] --sp ksw_ext2_11.m_axi_gmem:HBM[10] --sp ksw_ext2_12.m_axi_gmem:HBM[11] --sp ksw_ext2_13.m_axi_gmem:HBM[12] --sp ksw_ext2_14.m_axi_gmem:HBM[13] --sp ksw_ext2_15.m_axi_gmem:HBM[14] --sp ksw_ext2_16.m_axi_gmem:HBM[15] --sp ksw_ext2_17.m_axi_gmem:HBM[16] --sp ksw_ext2_18.m_axi_gmem:HBM[17] --sp ksw_ext2_19.m_axi_gmem:HBM[18] --sp ksw_ext2_20.m_axi_gmem:HBM[19] --sp ksw_ext2_21.m_axi_gmem:HBM[20] --sp ksw_ext2_22.m_axi_gmem:HBM[21] --sp ksw_ext2_23.m_axi_gmem:HBM[22] --sp ksw_ext2_24.m_axi_gmem:HBM[23] --sp ksw_ext2_25.m_axi_gmem:HBM[24] --sp ksw_ext2_26.m_axi_gmem:HBM[25] --sp ksw_ext2_27.m_axi_gmem:HBM[26] --sp ksw_ext2_28.m_axi_gmem:HBM[27] --sp ksw_ext2_29.m_axi_gmem:HBM[28] --sp ksw_ext2_30.m_axi_gmem:HBM[29] --sp ksw_ext2_31.m_axi_gmem:HBM[30] --sp ksw_ext2_32.m_axi_gmem:HBM[31] --sp ksw_ext2_33.m_axi_gmem:HBM[0] --sp ksw_ext2_34.m_axi_gmem:HBM[1] --sp ksw_ext2_35.m_axi_gmem:HBM[2] --sp ksw_ext2_36.m_axi_gmem:HBM[3] --sp ksw_ext2_37.m_axi_gmem:HBM[4] --sp ksw_ext2_38.m_axi_gmem:HBM[5] --sp ksw_ext2_39.m_axi_gmem:HBM[6] --sp ksw_ext2_40.m_axi_gmem:HBM[7] --sp ksw_ext2_41.m_axi_gmem:HBM[8] --sp ksw_ext2_42.m_axi_gmem:HBM[9] --sp ksw_ext2_43.m_axi_gmem:HBM[10] --sp ksw_ext2_44.m_axi_gmem:HBM[11] --sp ksw_ext2_45.m_axi_gmem:HBM[12] --sp ksw_ext2_46.m_axi_gmem:HBM[13] --sp ksw_ext2_47.m_axi_gmem:HBM[14] --sp ksw_ext2_48.m_axi_gmem:HBM[15] --sp ksw_ext2_49.m_axi_gmem:HBM[16] --sp ksw_ext2_50.m_axi_gmem:HBM[17] --sp ksw_ext2_51.m_axi_gmem:HBM[18] --sp ksw_ext2_52.m_axi_gmem:HBM[19] --sp ksw_ext2_53.m_axi_gmem:HBM[20] --sp ksw_ext2_54.m_axi_gmem:HBM[21] --sp ksw_ext2_55.m_axi_gmem:HBM[22] --sp ksw_ext2_56.m_axi_gmem:HBM[23] --sp ksw_ext2_57.m_axi_gmem:HBM[24] --sp ksw_ext2_58.m_axi_gmem:HBM[25] --sp ksw_ext2_59.m_axi_gmem:HBM[26] --sp ksw_ext2_60.m_axi_gmem:HBM[27]
LDCLFLAGS += --sp ksw_ext2_1.m_axi_gmem:HBM[0] --sp ksw_ext2_2.m_axi_gmem:HBM[1] --sp ksw_ext2_3.m_axi_gmem:HBM[2] --sp ksw_ext2_4.m_axi_gmem:HBM[3] --sp ksw_ext2_5.m_axi_gmem:HBM[4] --sp ksw_ext2_6.m_axi_gmem:HBM[5] --sp ksw_ext2_7.m_axi_gmem:HBM[6] --sp ksw_ext2_8.m_axi_gmem:HBM[7] --sp ksw_ext2_9.m_axi_gmem:HBM[8] --sp ksw_ext2_10.m_axi_gmem:HBM[9] --sp ksw_ext2_11.m_axi_gmem:HBM[10] --sp ksw_ext2_12.m_axi_gmem:HBM[11] --sp ksw_ext2_13.m_axi_gmem:HBM[12] --sp ksw_ext2_14.m_axi_gmem:HBM[13] --sp ksw_ext2_15.m_axi_gmem:HBM[14] --sp ksw_ext2_16.m_axi_gmem:HBM[15] --sp ksw_ext2_17.m_axi_gmem:HBM[16] --sp ksw_ext2_18.m_axi_gmem:HBM[17] --sp ksw_ext2_19.m_axi_gmem:HBM[18] --sp ksw_ext2_20.m_axi_gmem:HBM[19] --sp ksw_ext2_21.m_axi_gmem:HBM[20] --sp ksw_ext2_22.m_axi_gmem:HBM[21] --sp ksw_ext2_23.m_axi_gmem:HBM[22] --sp ksw_ext2_24.m_axi_gmem:HBM[23] --sp ksw_ext2_25.m_axi_gmem:HBM[24] --sp ksw_ext2_26.m_axi_gmem:HBM[25] --sp ksw_ext2_27.m_axi_gmem:HBM[26] --sp ksw_ext2_28.m_axi_gmem:HBM[27] --sp ksw_ext2_29.m_axi_gmem:HBM[28] --sp ksw_ext2_30.m_axi_gmem:HBM[29] --sp ksw_ext2_31.m_axi_gmem:HBM[30] --sp ksw_ext2_32.m_axi_gmem:HBM[31] 
#LDCLFLAGS += --sp ksw_ext2_1.m_axi_gmem:HBM[0] --sp ksw_ext2_2.m_axi_gmem:HBM[1]

EXECUTABLE = host
CMD_ARGS = $(XCLBIN)/ksw_ext2.$(TARGET).$(DSA).xclbin

EMCONFIG_DIR = $(XCLBIN)/$(DSA)

BINARY_CONTAINERS += $(XCLBIN)/ksw_ext2.$(TARGET).$(DSA).xclbin
BINARY_CONTAINER_ksw_ext2_OBJS += $(XCLBIN)/ksw_ext2.$(TARGET).$(DSA).xo

CP = cp -rf

.PHONY: all clean cleanall docs emconfig
all: check-devices $(EXECUTABLE) $(BINARY_CONTAINERS) emconfig

.PHONY: exe
exe: $(EXECUTABLE)

.PHONY: build
build: $(BINARY_CONTAINERS)

# Building kernel
$(XCLBIN)/ksw_ext2.$(TARGET).$(DSA).xo: src/ksw_ext2.cl
	mkdir -p $(XCLBIN)
	$(XOCC) $(CLFLAGS) --temp_dir $(BUILD_DIR_ksw_ext2) -c -k ksw_ext2 -I'$(<D)' -o'$@' '$<'
$(XCLBIN)/ksw_ext2.$(TARGET).$(DSA).xclbin: $(BINARY_CONTAINER_ksw_ext2_OBJS)
	mkdir -p $(XCLBIN)
	$(XOCC) $(CLFLAGS) --temp_dir $(BUILD_DIR_ksw_ext2) -l $(LDCLFLAGS) --nk ksw_ext2:32 -o'$@' $(+)

# Building Host
$(EXECUTABLE): check-xrt $(HOST_SRCS) $(HOST_HDRS)
	$(CXX) $(CXXFLAGS) $(HOST_SRCS) $(HOST_HDRS) -o '$@' $(LDFLAGS)

emconfig:$(EMCONFIG_DIR)/emconfig.json
$(EMCONFIG_DIR)/emconfig.json:
	emconfigutil --platform $(DEVICE) --od $(EMCONFIG_DIR)

check: all
ifeq ($(findstring xilinx_u200_xdma, $(DEVICE)), xilinx_u200_xdma)
$(error Nothing to be done for make)
endif
ifeq ($(findstring xilinx_u50_xdma, $(DEVICE)), xilinx_u50_xdma)
$(error Nothing to be done for make)
endif
ifeq ($(findstring xilinx_u200_qdma, $(DEVICE)), xilinx_u200_qdma)
$(error Nothing to be done for make)
endif
ifeq ($(findstring xilinx_u250_xdma, $(DEVICE)), xilinx_u250_xdma)
$(error Nothing to be done for make)
endif
ifeq ($(findstring xilinx_u250_qdma, $(DEVICE)), xilinx_u250_qdma)
$(error Nothing to be done for make)
endif
ifeq ($(findstring xilinx_vcu1525_dynamic, $(DEVICE)), xilinx_vcu1525_dynamic)
$(error Nothing to be done for make)
endif
ifeq ($(findstring xilinx_aws-vu9p-f1-04261818, $(DEVICE)), xilinx_aws-vu9p-f1-04261818)
$(error Nothing to be done for make)
endif
ifeq ($(findstring xilinx:u200:xdma, $(DEVICE)), xilinx:u200:xdma)
$(error Nothing to be done for make)
endif
ifeq ($(findstring xilinx:u50:xdma, $(DEVICE)), xilinx:u50:xdma)
$(error Nothing to be done for make)
endif
ifeq ($(findstring xilinx:u200:qdma, $(DEVICE)), xilinx:u200:qdma)
$(error Nothing to be done for make)
endif
ifeq ($(findstring xilinx:u250:xdma, $(DEVICE)), xilinx:u250:xdma)
$(error Nothing to be done for make)
endif
ifeq ($(findstring xilinx:u250:qdma, $(DEVICE)), xilinx:u250:qdma)
$(error Nothing to be done for make)
endif
ifeq ($(findstring xilinx:vcu1525:dynamic, $(DEVICE)), xilinx:vcu1525:dynamic)
$(error Nothing to be done for make)
endif
ifeq ($(findstring xilinx:aws-vu9p-f1-04261818:dynamic, $(DEVICE)), xilinx:aws-vu9p-f1-04261818:dynamic)
$(error Nothing to be done for make)
endif

ifeq ($(TARGET),$(filter $(TARGET),sw_emu hw_emu))
	$(CP) $(EMCONFIG_DIR)/emconfig.json .
	XCL_EMULATION_MODE=$(TARGET) ./$(EXECUTABLE) $(XCLBIN)/ksw_ext2.$(TARGET).$(DSA).xclbin
else
	 ./$(EXECUTABLE) $(XCLBIN)/ksw_ext2.$(TARGET).$(DSA).xclbin
endif
	sdx_analyze profile -i profile_summary.csv -f html

#run_nimbix: all
#	$(COMMON_REPO)/utility/nimbix/run_nimbix.py $(EXECUTABLE) $(CMD_ARGS) $(DSA)

#aws_build: check-aws_repo $(BINARY_CONTAINERS)
#	$(COMMON_REPO)/utility/aws/run_aws.py $(BINARY_CONTAINERS)

# Cleaning stuff
clean:
	-$(RMDIR) $(EXECUTABLE) $(XCLBIN)/{*sw_emu*,*hw_emu*} 
	-$(RMDIR) profile_* TempConfig system_estimate.xtxt *.rpt *.csv 
	-$(RMDIR) src/*.ll _xocc_* .Xil emconfig.json dltmp* xmltmp* *.log *.jou *.wcfg *.wdb

cleanall: clean
	-$(RMDIR) $(XCLBIN)
	-$(RMDIR) _x.*

