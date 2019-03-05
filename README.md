# BWA_HLS
This is a test implementation of the Smith-Waterman algorithm, specifically the ksw_extend function as implemented in BWA, using Xilinx Vivado High-Level Synthesis.

Based on:
 - https://github.com/lh3/bwa
 - https://github.com/Xilinx/SDAccel_Examples

Folders:
 - VHLS_BWA_SW: Vivado HLS project, implementation of ksw_extend2 function
 - VHLS_BWA_SW_test: test matching C and HLS results, cosimulation does not compile
 - SDx_BWA_SW: SDAccel implementation

The implementation has been tested successfully on a local machine with ADM-PCIE-KU3 board and on an AWS F1 machine.

Resuming the project soon with full implementation
