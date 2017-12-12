// fix bug https://forums.xilinx.com/t5/Vivado-High-Level-Synthesis-HLS/Vivado-2015-3-HLS-Bug-gmp-h/td-p/661141
#include "/opt/Xilinx/SDx/2017.1/Vivado_HLS/include/gmp.h"
#include <stdio.h>
#include <ap_int.h>

const int qr_max = 256;
const int db_max = 256;
const int m = 5;
const int smlen = m * m;

void ksw_ext2(ap_uint<10> qlen, ap_uint<4> query[qr_max], ap_int<10> tlen,
		ap_uint<4> target[db_max], ap_uint<10> match, ap_uint<10> mismatch,
		ap_uint<10> ambiguous, ap_uint<10> o_del, ap_uint<10> e_del,
		ap_uint<10> o_ins, ap_uint<10> e_ins, ap_uint<10> w, ap_uint<10> h0,
		ap_int<10> *_qle, ap_int<10> *_tle, ap_int<10> *_gtle,
		ap_int<10> *_gscore, ap_int<10> *_max_off, ap_int<10> *_max);

int getRandomNumber(int min, int max) {
	return (rand() % (max - min + 1)) + min;
}

int getRandomCharacter() {
	return rand() % m;
}

int main() {
	for (int test_j = 1; test_j <= 33; test_j++) {
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
		ap_uint<4> db[db_max];
		for (int i = 0; i < db_max; i++) {
			if (i < db_len) {
				db_qr_element = getRandomCharacter();
			} else {
				db_qr_element = 0;
			}
			db[i] = (ap_uint<4> ) db_qr_element;
		}

		// generate qr
		ap_uint<4> qr[qr_max];
		for (int i = 0; i < qr_max; i++) {
			if (i < qr_len) {
				db_qr_element = getRandomCharacter();
			} else {
				db_qr_element = 0;
			}
			qr[i] = (ap_uint<4> ) db_qr_element;
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
		max_ins = (int) ((double) (qr_len * max + end_bonus - o_ins) / e_ins
				+ 1.);
		max_ins = max_ins > 1 ? max_ins : 1;
		wh = wh < max_ins ? wh : max_ins;
		max_del = (int) ((double) (qr_len * max + end_bonus - o_del) / e_del
				+ 1.);
		max_del = max_del > 1 ? max_del : 1;
		wh = wh < max_del ? wh : max_del; // TODO: is this necessary?
		/**************************************************/

		printf("test %d: db_len=%d qr_len=%d w=%d match=%d mismatch=%d ambiguous=%d o_del=%d e_del=%d o_ins=%d e_ins=%d end_bonus=%d\n",
				test_j, db_len, qr_len, w, match, -mismatch, -ambiguous, o_del, e_del, o_ins, e_ins, end_bonus);

		ap_int<10> _qle;
		ap_int<10> _tle;
		ap_int<10> _gtle;
		ap_int<10> _gscore;
		ap_int<10> _max_off;
		ap_int<10> _max;
		ksw_ext2((ap_uint<10> ) qr_len, qr, (ap_uint<10> ) db_len, db,
				(ap_uint<10> ) match, (ap_uint<10> ) mismatch,
				(ap_uint<10> ) ambiguous, (ap_uint<10> ) o_del,
				(ap_uint<10> ) e_del, (ap_uint<10> ) o_ins,
				(ap_uint<10> ) e_ins, (ap_uint<10> ) wh, (ap_uint<10> ) h0,
				&_qle, &_tle, &_gtle, &_gscore, &_max_off, &_max);
		printf("HLS: max_j=%d max_i=%d max_ie=%d gscore=%d max_off=%d max=%d\n", (int)_qle, (int)_tle, (int)_gtle, (int)_gscore, (int)_max_off, (int) _max - h0);

		delete[] simat;
	}
	return 0;
}
