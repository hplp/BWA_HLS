############################################################
## This file is generated automatically by Vivado HLS.
## Please DO NOT edit it.
## Copyright (C) 1986-2017 Xilinx, Inc. All Rights Reserved.
############################################################
open_project VHLS_BWA_SW
set_top ksw_ext2
add_files VHLS_BWA_SW/solution1/ksw_ext2.cpp
add_files -tb VHLS_BWA_SW/solution1/test_ksw_ext2.cpp
open_solution "solution1"
set_part {xcku060-ffva1156-2-e} -tool vivado
create_clock -period 3.6 -name default
set_clock_uncertainty 0.1
#source "./VHLS_BWA_SW/solution1/directives.tcl"
csim_design -clean -compiler gcc
csynth_design
cosim_design -trace_level all
export_design -rtl verilog -format ip_catalog
