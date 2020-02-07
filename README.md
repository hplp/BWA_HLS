# BWA_HLS
This is a test implementation of the Smith-Waterman algorithm, specifically the ksw_extend function as implemented in BWA, using Xilinx Vivado High-Level Synthesis. Moreover, implementations in SDAccel targeting the AWS-F1 FPGA accelerator card and the Alveo U280 FPGA accelerator card are provided.

Based on:
 - https://github.com/lh3/bwa
 - https://github.com/Xilinx/SDAccel_Examples

Folders:
 - VHLS_BWA_SW: Vivado HLS project, implementation of ksw_extend2 function
 - VHLS_BWA_SW_test: test matching C and HLS results, cosimulation does not compile
 - SDx_BWA_SW: SDAccel implementation targeting Alveo U280 and HBM use
 - AWSF1_SDx_BWA_SW: SDAccel implementation targeting the AWS-F1

The implementation has been tested successfully on a local machine with ADM-PCIE-KU3 board, on an AWS F1 machine, and on a local machine with the Alveo U280 card.

Note: The Alveo U280 implementation requires a few Xilinx SDAccel from https://github.com/Xilinx/SDAccel_Examples/tree/master/libs.
