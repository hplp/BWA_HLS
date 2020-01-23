#include "xcl2.hpp"

#include <stdio.h>
#include <ap_int.h>

#include <time.h>
#include <chrono>

extern "C" {
#include "bwa.h"
#include "bwa.c"
}

#include <vector>
using std::vector;

//Number of HBM Banks required
#define MAX_HBM_BANKCOUNT 32
#define BANK_NAME(n) n | XCL_MEM_TOPOLOGY
const int bank[MAX_HBM_BANKCOUNT] = {

BANK_NAME(0), BANK_NAME(1), BANK_NAME(2), BANK_NAME(3), BANK_NAME(4), BANK_NAME(5), BANK_NAME(6), BANK_NAME(7),

BANK_NAME(8), BANK_NAME(9), BANK_NAME(10), BANK_NAME(11), BANK_NAME(12), BANK_NAME(13), BANK_NAME(14), BANK_NAME(15),

BANK_NAME(16), BANK_NAME(17), BANK_NAME(18), BANK_NAME(19), BANK_NAME(20), BANK_NAME(21), BANK_NAME(22), BANK_NAME(23),

BANK_NAME(24), BANK_NAME(25), BANK_NAME(26), BANK_NAME(27), BANK_NAME(28), BANK_NAME(29), BANK_NAME(30), BANK_NAME(31) };

static const int qr_max = 256;
static const int db_max = 256;
static const int r_vars = 6;
static const int m = 5;
static const int smlen = m * m;

int getRandomNumber(int min, int max) {
	return (rand() % (max - min + 1)) + min;
}

int getRandomCharacter() {
	return rand() % m;
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
		std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << std::endl;
		return EXIT_FAILURE;
	}

	std::string binaryFile = argv[1];
	cl_int err;

	// Creates the vectors to be sent / returned
	vector<ap_uint<4>, aligned_allocator<ap_uint<4>>> source_q(qr_max);
	vector<ap_uint<4>, aligned_allocator<ap_uint<4>>> source_t(db_max);
	vector<short, aligned_allocator<short>> source_m(r_vars);

	// Compute the size of array in bytes
	size_t size_in_bytes_q = qr_max * sizeof(ap_uint<4> );
	size_t size_in_bytes_t = db_max * sizeof(ap_uint<4> );
	size_t size_in_bytes_m = r_vars * sizeof(short);

	// The get_xil_devices will return vector of Xilinx Devices
	auto devices = xcl::get_xil_devices();
	auto device = devices[0];

	//Creating Context and Command Queue for selected Device
	OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
	OCL_CHECK(err, cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
	OCL_CHECK(err, std::string device_name = device.getInfo<CL_DEVICE_NAME>(&err));
	std::cout << "Found Device=" << device_name.c_str() << std::endl;

	// read_binary() command will find the OpenCL binary file created using the
	// xocc compiler load into OpenCL Binary and return a pointer to file buffer
	// and it can contain many functions which can be executed on the
	// device.
	auto fileBuf = xcl::read_binary_file(binaryFile);
	cl::Program::Binaries bins { { fileBuf.data(), fileBuf.size() } };
	devices.resize(1);
	OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

	// These commands will allocate memory on the FPGA. The cl::Buffer objects can
	// be used to reference the memory locations on the device. The cl::Buffer
	// object cannot be referenced directly and must be passed to other OpenCL
	// functions.
	OCL_CHECK(err, cl::Buffer buffer_q(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, size_in_bytes_q, source_q.data(), &err));
	OCL_CHECK(err, cl::Buffer buffer_t(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, size_in_bytes_t, source_t.data(), &err));
	OCL_CHECK(err, cl::Buffer buffer_m(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, size_in_bytes_m, source_m.data(), &err));

	// This call will extract a kernel out of the program we loaded in the
	// previous line. A kernel is an OpenCL function that is executed on the
	// FPGA. This function is defined in the src/ksw_ext2.cl file.
	OCL_CHECK(err, cl::Kernel krnl_ksw_ext2(program, "ksw_ext2", &err));

	bool all_tests_passed = true;
	int test_j = 0;
	for (test_j = 1; test_j <= 333; test_j++) {
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

		int db_qr_element;

		// generate db
		uint8_t* db_c = new uint8_t[db_max];
		for (int i = 0; i < db_max; i++) {
			if (i < db_len) {
				db_qr_element = getRandomCharacter();
			} else {
				db_qr_element = 0;
			}
			source_t[i] = (ap_uint<4> ) db_qr_element;
			db_c[i] = (uint8_t) db_qr_element;
		}

		// generate qr
		uint8_t* qr_c = new uint8_t[qr_max];
		for (int i = 0; i < qr_max; i++) {
			if (i < qr_len) {
				db_qr_element = getRandomCharacter();
			} else {
				db_qr_element = 0;
			}
			source_q[i] = (ap_uint<4> ) db_qr_element;
			qr_c[i] = (uint8_t) db_qr_element;
		}

		//declare the similarity matrix
		int8_t* simat = new int8_t[smlen]; // similarity matrix pointer
		int ij = 0;
		for (int i = 0; i < m - 1; ++i) {
			for (int j = 0; j < m - 1; ++j) {
				simat[ij++] = i == j ? match : -mismatch;
			}
			simat[ij++] = -ambiguous; // ambiguous base
		}
		for (int j = 0; j < m; ++j) {
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

		printf("test %d: db_len=%d qr_len=%d w=%d match=%d mismatch=%d ambiguous=%d o_del=%d e_del=%d o_ins=%d e_ins=%d end_bonus=%d\n", test_j, db_len, qr_len, w,
				match, -mismatch, -ambiguous, o_del, e_del, o_ins, e_ins, end_bonus);

		// These commands will load the source_a and source_b vectors from the host
		// application and into the buffer_a and buffer_b cl::Buffer objects. The data
		// will be be transferred from system memory over PCIe to the FPGA on-board
		// DDR memory.
		OCL_CHECK(err, err = q.enqueueMigrateMemObjects( { buffer_q, buffer_t }, 0 /* 0 means from host*/));

		//set the kernel Arguments
		int narg = 0;
		krnl_ksw_ext2.setArg(narg++, (ushort) qr_len);
		krnl_ksw_ext2.setArg(narg++, buffer_q);
		krnl_ksw_ext2.setArg(narg++, (ushort) db_len);
		krnl_ksw_ext2.setArg(narg++, buffer_t);
		krnl_ksw_ext2.setArg(narg++, (ushort) match);
		krnl_ksw_ext2.setArg(narg++, (ushort) mismatch);
		krnl_ksw_ext2.setArg(narg++, (ushort) ambiguous);
		krnl_ksw_ext2.setArg(narg++, (ushort) o_del);
		krnl_ksw_ext2.setArg(narg++, (ushort) e_del);
		krnl_ksw_ext2.setArg(narg++, (ushort) o_ins);
		krnl_ksw_ext2.setArg(narg++, (ushort) e_ins);
		krnl_ksw_ext2.setArg(narg++, (ushort) w);
		krnl_ksw_ext2.setArg(narg++, (ushort) h0);
		krnl_ksw_ext2.setArg(narg++, buffer_m);

		const auto initStartTimeH = std::chrono::high_resolution_clock::now();
		//Launch the Kernel
		q.enqueueTask(krnl_ksw_ext2);
		OCL_CHECK(err, err = q.enqueueTask(krnl_ksw_ext2));

		// The result of the previous kernel execution will need to be retrieved in
		// order to view the results. This call will write the data from the
		// buffer_result cl_mem object to the source_results vector
		OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_m}, CL_MIGRATE_MEM_OBJECT_HOST));
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
		printf("HLS: max_j=%d max_i=%d max_ie=%d gscore=%d max_off=%d max=%d\n", source_m[0], source_m[1], source_m[2], source_m[3], source_m[4], source_m[5] - h0);
		std::cout << "C++ total_ns: " << total_ns << "\n";
		std::cout << "HLS total_ns: " << total_nsH << "\n";
		if ((RezC.max == (int) (source_m[5] - h0)) && (RezC.max_j == (int) source_m[0]) && (RezC.max_i == (int) source_m[1]) && (RezC.max_ie == (int) source_m[2])
				&& (RezC.gscore == (int) source_m[3]) && (RezC.max_off == (int) source_m[4])) {
			printf("test %d passed\n\n", test_j);
		} else {
			printf("test %d failed\n\n", test_j);
			all_tests_passed = false;
		}
	}
	if (all_tests_passed)
		printf("All %d tests have passed\n", test_j - 1);
	printf("_END_ \n\n");
	return (all_tests_passed ? EXIT_FAILURE : EXIT_SUCCESS);
}
