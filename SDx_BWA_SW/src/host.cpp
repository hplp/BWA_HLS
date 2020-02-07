/********************************************************************************************
 * This host file will interface with the ksw_ext2 FPGA kernel via HBM and DRAM
 * Additionally, it will compare the results with the original C code
 * from here: https://github.com/lh3/bwa/blob/master/ksw.c
 *
 * One of the goals is to evaluate kernel performance, CPU vs FPGA
 * Another goal is to evaluate HBM performance
 *
 *  *****************************************************************************************/
#include "xcl2.hpp"

#include <stdio.h>

#include <vector>

#include <time.h>
#include <chrono>

extern "C" {
#include "bwa.h"
#include "bwa.c"
}

#define NUM_KERNEL 2

//Number of HBM Banks required
#define MAX_HBM_BANKCOUNT 32
#define BANK_NAME(n) n | XCL_MEM_TOPOLOGY

const int bank[MAX_HBM_BANKCOUNT] = {

BANK_NAME(0), BANK_NAME(1), BANK_NAME(2), BANK_NAME(3), BANK_NAME(4), BANK_NAME(5), BANK_NAME(6), BANK_NAME(7),

BANK_NAME(8), BANK_NAME(9), BANK_NAME(10), BANK_NAME(11), BANK_NAME(12), BANK_NAME(13), BANK_NAME(14), BANK_NAME(15),

BANK_NAME(16), BANK_NAME(17), BANK_NAME(18), BANK_NAME(19), BANK_NAME(20), BANK_NAME(21), BANK_NAME(22), BANK_NAME(23),

BANK_NAME(24), BANK_NAME(25), BANK_NAME(26), BANK_NAME(27), BANK_NAME(28), BANK_NAME(29), BANK_NAME(30), BANK_NAME(31) };

static const unsigned int qr_max = 256;
static const unsigned int db_max = 256;
static const unsigned int r_vars = 6;
static const unsigned int m = 5;
static const unsigned int smlen = m * m;

int getRandomNumber(int min, int max) {
	return (rand() % (max - min + 1)) + min;
}

uint8_t getRandomCharacter() {
	return (uint8_t) rand() % m;
}

struct ResultC {
	int max;
	int max_i;
	int max_j;
	int max_ie;
	int gscore;
	int max_off;
};

int main(int argc, char **argv) {
	if (argc != 2) {
		printf("Usage: %s <XCLBIN> \n", argv[0]);
		return EXIT_FAILURE;
	}

	std::string binaryFile = argv[1];
	cl_int err;

	// The get_xil_devices will return vector of Xilinx Devices
	auto devices = xcl::get_xil_devices();
	auto device = devices[0];

	// Creating Context and Command Queue for selected Device
	OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
	OCL_CHECK(err, cl::CommandQueue q(context, device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE, &err));

	OCL_CHECK(err, std::string device_name = device.getInfo<CL_DEVICE_NAME>(&err));
	std::cout << "Found Device=" << device_name.c_str() << std::endl;

	// Run read_binary_file() to find OpenCL binary file created using xocc compiler & return pointer to file buffer
	auto fileBuf = xcl::read_binary_file(binaryFile);

	// Manage Program and Kernel
	cl::Program::Binaries bins { { fileBuf.data(), fileBuf.size() } };
	devices.resize(1);
	OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

	// OCL_CHECK(err, cl::Kernel krnl_ksw_ext2(program, "ksw_ext2", &err));

	// Creating Kernel Vector of Compute Units
	std::string krnl_name = "ksw_ext2";
	std::vector<cl::Kernel> krnls(NUM_KERNEL);

	for (int i = 0; i < NUM_KERNEL; i++) {
		std::string cu_id = std::to_string(i + 1);
		std::string krnl_name_full = krnl_name + ":{" + "ksw_ext2_" + cu_id + "}";

		printf("Creating a kernel [%s] for CU(%d)\n", krnl_name_full.c_str(), i + 1);

		//Here Kernel object is created by specifying kernel name along with compute unit.
		//For such case, this kernel object can only access the specific Compute unit

		OCL_CHECK(err, krnls[i] = cl::Kernel(program, krnl_name_full.c_str(), &err));
	}

	// Vectors communicating with kernel
	std::vector<uint8_t, aligned_allocator<uint8_t>> source_q(qr_max);
	std::vector<uint8_t, aligned_allocator<uint8_t>> source_t(db_max);

	//std::vector<int, aligned_allocator<int>> result_m(r_vars);
	std::vector<int, aligned_allocator<int>> result_m[NUM_KERNEL];
	for (int i = 0; i < NUM_KERNEL; i++) {
		result_m[i].resize(r_vars);
		fill(result_m[i].begin(), result_m[i].end(), 0);
	}

	// Compute the size of array in bytes
//	size_t size_in_bytes_q = qr_max * sizeof(uint8_t);
//	size_t size_in_bytes_t = db_max * sizeof(uint8_t);
//	size_t size_in_bytes_m = r_vars * sizeof(uint32_t);

	// Use cl_mem_ext_ptr_t to allocate buffers to specific Global Memory Bank Banks
	std::vector<cl_mem_ext_ptr_t> BufExt_q(NUM_KERNEL);
	std::vector<cl_mem_ext_ptr_t> BufExt_t(NUM_KERNEL);
	std::vector<cl_mem_ext_ptr_t> BufExt_m(NUM_KERNEL);

	std::vector<cl::Buffer> buffer_q(NUM_KERNEL);
	std::vector<cl::Buffer> buffer_t(NUM_KERNEL);
	std::vector<cl::Buffer> buffer_m(NUM_KERNEL);

	bool all_tests_passed = true;
	unsigned int test_j = 0;
	for (test_j = 1; test_j <= 100; test_j++) {
		srand(test_j);
		int o_del = getRandomNumber(2, 5);
		int e_del = getRandomNumber(2, 5);
		int o_ins = getRandomNumber(2, 5);
		int e_ins = getRandomNumber(2, 5);
		int match = getRandomNumber(2, 5);
		int mismatch = getRandomNumber(1, 3); // will use the negative of this
		int ambiguous = getRandomNumber(1, 2); // will use the negative of this
		int end_bonus = getRandomNumber(3, 7);
		int qr_len = getRandomNumber(8, qr_max); // query length
		int db_len = getRandomNumber(qr_max, db_max); // target length

		int w = getRandomNumber(1, 100);
		int h0 = getRandomNumber(qr_max / 2, qr_max);

		uint8_t db_qr_element;

		// generate db
		printf("H %d db_len: ", db_len);
		uint8_t* db_c = new uint8_t[db_max];
		for (unsigned int i = 0; i < db_max; i++) {
			if (i < (unsigned int) db_len) {
				db_qr_element = getRandomCharacter();
			} else {
				db_qr_element = 0;
			}
			source_t[i] = db_qr_element;
			db_c[i] = db_qr_element;
			printf("%d ", source_t[i]);
		}
		printf("\n");

		// generate qr
		printf("Ht %d qr_len: ", qr_len);
		uint8_t* qr_c = new uint8_t[qr_max];
		for (unsigned int i = 0; i < qr_max; i++) {
			if (i < (unsigned int) qr_len) {
				db_qr_element = getRandomCharacter();
			} else {
				db_qr_element = 0;
			}
			source_q[i] = db_qr_element;
			qr_c[i] = db_qr_element;
			printf("%d ", source_q[i]);
		}
		printf("\n");

		//declare the similarity matrix
		int8_t* simat = new int8_t[smlen]; // similarity matrix pointer
		int ij = 0;
		for (unsigned int i = 0; i < m - 1; ++i) {
			for (unsigned int j = 0; j < m - 1; ++j) {
				simat[ij++] = i == j ? match : -mismatch;
			}
			simat[ij++] = -ambiguous; // ambiguous base
		}
		for (unsigned int j = 0; j < m; ++j) {
			simat[ij++] = -ambiguous;
		}

		// declare function parameters and execute first part of the algorithm on the CPU
		/**************************************************/
		// adjust $w if it is too large
		int i, max, max_ins, max_del, k = m * m, wh = w;
		for (i = 0, max = 0; i < k; ++i) // get the max score
			max = max > simat[i] ? max : simat[i];
		max_ins = (int) ((double) (qr_len * max + end_bonus - o_ins) / e_ins + 1.);
		max_ins = max_ins > 1 ? max_ins : 1;
		wh = wh < max_ins ? wh : max_ins;
		max_del = (int) ((double) (qr_len * max + end_bonus - o_del) / e_del + 1.);
		max_del = max_del > 1 ? max_del : 1;
		wh = wh < max_del ? wh : max_del; // TODO: is this necessary?
		/**************************************************/

		printf("test %d: db_len=%d qr_len=%d match=%d mismatch=%d ambiguous=%d o_del=%d e_del=%d o_ins=%d e_ins=%d w=%d end_bonus=%d\n", test_j, db_len, qr_len, match,
				-mismatch, -ambiguous, o_del, e_del, o_ins, e_ins, w, end_bonus);

		for (int i = 0; i < NUM_KERNEL; i++) {
			BufExt_q[i].obj = source_q.data();
			BufExt_q[i].param = 0;
			BufExt_q[i].flags = bank[i];

			BufExt_t[i].obj = source_t.data();
			BufExt_t[i].param = 0;
			BufExt_t[i].flags = bank[i];

			BufExt_m[i].obj = result_m[i].data();
			BufExt_m[i].param = 0;
			BufExt_m[i].flags = bank[i];
		}

		// These commands will allocate memory on the FPGA. The cl::Buffer objects can
		// be used to reference the memory locations on the device. The cl::Buffer
		// object cannot be referenced directly and must be passed to other OpenCL
		// functions.
		//OCL_CHECK(err, cl::Buffer buffer_q(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR, sizeof(uint32_t) * size, &inBufExt1, &err));
		for (int i = 0; i < NUM_KERNEL; i++) {
			printf("i=%d %d %d \n", i, (int) buffer_q.size(), (int) buffer_t.size());
			OCL_CHECK(err,
					buffer_q[i] = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR, qr_max * sizeof(uint8_t), &BufExt_q[i], &err));
			OCL_CHECK(err,
					buffer_t[i] = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR, db_max * sizeof(uint8_t), &BufExt_t[i], &err));
			OCL_CHECK(err,
					buffer_m[i] = cl::Buffer(context, CL_MEM_WRITE_ONLY | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR, r_vars * sizeof(uint32_t), &BufExt_m[i], &err));
		}

		// These commands will load the source_a and source_b vectors from the host
		// application and into the buffer_a and buffer_b cl::Buffer objects. The data
		// will be be transferred from system memory over PCIe to the FPGA on-board
		// DDR memory.
		for (int i = 0; i < NUM_KERNEL; i++) {
			OCL_CHECK(err, err = q.enqueueMigrateMemObjects( { buffer_q[i], buffer_t[i] }, 0 /* 0 means from host*/));
		}
		q.finish();

		for (int i = 0; i < NUM_KERNEL; i++) {
			//set the kernel Arguments
			int narg = 0;
			OCL_CHECK(err, err = krnls[i].setArg(narg++, (ushort ) qr_len));
			OCL_CHECK(err, err = krnls[i].setArg(narg++, buffer_q[i]));
			OCL_CHECK(err, err = krnls[i].setArg(narg++, (ushort ) db_len));
			OCL_CHECK(err, err = krnls[i].setArg(narg++, buffer_t[i]));
			OCL_CHECK(err, err = krnls[i].setArg(narg++, (ushort ) match));
			OCL_CHECK(err, err = krnls[i].setArg(narg++, (ushort ) mismatch));
			OCL_CHECK(err, err = krnls[i].setArg(narg++, (ushort ) ambiguous));
			OCL_CHECK(err, err = krnls[i].setArg(narg++, (ushort ) o_del));
			OCL_CHECK(err, err = krnls[i].setArg(narg++, (ushort ) e_del));
			OCL_CHECK(err, err = krnls[i].setArg(narg++, (ushort ) o_ins));
			OCL_CHECK(err, err = krnls[i].setArg(narg++, (ushort ) e_ins));
			OCL_CHECK(err, err = krnls[i].setArg(narg++, (ushort ) w));
			OCL_CHECK(err, err = krnls[i].setArg(narg++, (ushort ) h0));
			OCL_CHECK(err, err = krnls[i].setArg(narg++, buffer_m[i]));
		}

		const auto initStartTimeH = std::chrono::high_resolution_clock::now();
		//Launch the Kernel
		//q.enqueueTask(krnl_ksw_ext2);
		for (int i = 0; i < NUM_KERNEL; i++) {
			OCL_CHECK(err, err = q.enqueueTask(krnls[i]));
		}
		q.finish();

		// The result of the previous kernel execution will need to be retrieved in
		// order to view the results. This call will write the data from the
		// buffer_result cl_mem object to the source_results vector
		for (int i = 0; i < NUM_KERNEL; i++) {
			OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_m[i]}, CL_MIGRATE_MEM_OBJECT_HOST));
		}
		q.finish();

		const auto initEndTimeH = std::chrono::high_resolution_clock::now();
		const auto total_nsH = std::chrono::duration_cast<std::chrono::nanoseconds>(initEndTimeH - initStartTimeH).count();

		struct ResultC RezC;
		const auto initStartTime = std::chrono::high_resolution_clock::now();
		RezC.max = -h0
				+ ksw_extend2(qr_len, (uint8_t*) qr_c, db_len, (uint8_t*) db_c, m, (int8_t*) simat, o_del, e_del, o_ins, e_ins, w, end_bonus, 0, h0, &RezC.max_j,
						&RezC.max_i, &RezC.max_ie, &RezC.gscore, &RezC.max_off);
		const auto initEndTime = std::chrono::high_resolution_clock::now();
		const auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(initEndTime - initStartTime).count();
		delete[] db_c;
		delete[] qr_c;
		delete[] simat;

		printf("C++: max_j=%d max_i=%d max_ie=%d gscore=%d max_off=%d max=%d\n", RezC.max_j, RezC.max_i, RezC.max_ie, RezC.gscore, RezC.max_off, RezC.max);
		printf("HLS: max_j=%d max_i=%d max_ie=%d gscore=%d max_off=%d max=%d\n", result_m[0][0], result_m[0][1], result_m[0][2], result_m[0][3], result_m[0][4],
				result_m[0][5] - h0);
		std::cout << "C++ total_ns: " << total_ns << "\n";
		std::cout << "HLS total_ns: " << total_nsH << "\n";
		if ((RezC.max == (int) (result_m[0][5] - h0)) && (RezC.max_j == (int) result_m[0][0]) && (RezC.max_i == (int) result_m[0][1])
				&& (RezC.max_ie == (int) result_m[0][2]) && (RezC.gscore == (int) result_m[0][3]) && (RezC.max_off == (int) result_m[0][4])) {
			printf("test %d passed\n\n", test_j);
		} else {
			printf("test %d failed\n\n", test_j);
			all_tests_passed = false;
		}
	}
	if (all_tests_passed)
		printf("All %d tests have passed\n", test_j - 1);
	printf("_END_ \n\n");
	return (all_tests_passed ? EXIT_SUCCESS : EXIT_FAILURE);
}
