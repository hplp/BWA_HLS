// fix bug https://forums.xilinx.com/t5/Vivado-High-Level-Synthesis-HLS/Vivado-2015-3-HLS-Bug-gmp-h/td-p/661141
#include "/opt/Xilinx/SDx/2017.1/Vivado_HLS/include/gmp.h"
// Header for custom size variables for C++
// to declare variables as ap_uint<xy>
#include <ap_int.h>

const int qr_max = 256;
const int db_max = 256;
const ap_uint<4> m = 5;

typedef struct {
	ap_int<10> h, e;
} eh_t;

// Similarity function implementation
ap_int<10> similarity(ap_uint<4> a, ap_uint<4> b, ap_uint<10> match, ap_uint<10> mismatch, ap_uint<10> ambiguous) {
#pragma HLS inline
	if ((a == m - 1) || (b == m - 1)) {
		return 0 - ambiguous;
	} else if (a == b) {
		return match;
	} else
		return 0 - mismatch;
}

// ksw_extend2_hls algorithm
void ksw_ext2(ap_uint<10> qlen, ap_uint<4> query[qr_max], ap_int<10> tlen,
		ap_uint<4> target[db_max], ap_uint<10> match, ap_uint<10> mismatch,
		ap_uint<10> ambiguous, ap_uint<10> o_del, ap_uint<10> e_del,
		ap_uint<10> o_ins, ap_uint<10> e_ins, ap_uint<10> w, ap_uint<10> h0,
		ap_int<10> *_qle, ap_int<10> *_tle, ap_int<10> *_gtle,
		ap_int<10> *_gscore, ap_int<10> *_max_off, ap_int<10> *_max) {
#pragma HLS INTERFACe ap_memory port=query
#pragma HLS INTERFACe ap_memory port=target

	ap_uint<10> i, oe_del = o_del + e_del, oe_ins = o_ins + e_ins;
	ap_int<10> beg = 0, end = qlen, max = h0, max_i = -1, max_j = -1,
			max_ie = -1, gscore = -1, max_off = 0, eh_h_prev;
	bool break_i = false;

	// VivadoHLS synthesizes arrays as BRAM
	eh_t eh[qr_max + 1]; // score array

	// fill the first row
	eh[0].h = h0;
	eh[0].e = 0; // 0

	if (h0 > oe_ins) {
		eh_h_prev = h0 - oe_ins;
	} else {
		eh_h_prev = 0;
	}
	eh[1].h = eh_h_prev;
	eh[1].e = 0; // 1

	LOOP_eh: for (i = 2; i <= qlen; ++i) {
#pragma HLS dependence variable=eh inter false
#pragma HLS PIPELINE
		if (eh_h_prev > e_ins) {
			eh_h_prev = eh_h_prev - e_ins;
		} else {
			eh_h_prev = 0;
		}
		eh[i].h = eh_h_prev;
		eh[i].e = 0; // all other all the way to qlen
	}

	// DP loop
	LOOP_i: for (i = 0; (i < tlen) && (break_i == false); ++i) {
		ap_int<10> f = 0, h1, m = 0, mj = -1, j, abs_mji;

		// apply the band and the constraint (if provided)
		if (beg < i - w) beg = i - w;
		if (end > i + w + 1) end = i + w + 1;
		if (end > qlen) end = qlen;

		// compute the first column
		if (beg == 0) {
			h1 = h0 - (o_del + e_del * (i + 1));
			if (h1 < 0) h1 = 0;
		} else
			h1 = 0;

		LOOP_j: for (j = beg; j < end; ++j) {
#pragma HLS PIPELINE
#pragma HLS dependence variable=eh inter false

			// At the beginning of the loop: eh[j] = { H(i-1,j-1), E(i,j) }, f = F(i,j) and h1 = H(i,j-1)
			// Similar to SSE2-SW, cells are computed in the following order:
			//   H(i,j)   = max{H(i-1,j-1)+S(i,j), E(i,j), F(i,j)}
			//   E(i+1,j) = max{H(i,j)-gapo, E(i,j)} - gape
			//   F(i,j+1) = max{H(i,j)-gapo, F(i,j)} - gape
			eh_t tmp = eh[j];
			ap_int<10> h, t, q, M = tmp.h, e = tmp.e; // get H(i-1,j-1) and E(i-1,j)

#pragma HLS dependence variable=tmp inter false
#pragma HLS dependence variable=h inter false
#pragma HLS dependence variable=t inter false
#pragma HLS dependence variable=q inter false
#pragma HLS dependence variable=M inter false
#pragma HLS dependence variable=e inter false

			tmp.h = h1;          // set H(i,j-1) for the next row

			q = similarity(target[i], query[j], match, mismatch, ambiguous);
			if (M) {
				M = M + q;
			} else {
				M = 0;
			} // separating H and M to disallow a cigar like "100M3I3D20M"

			h = M > e ? M : e; // e and f are guaranteed to be non-negative, so h>=0 even if M<0
			h = h > f ? h : f;
			h1 = h;             // save H(i,j) to h1 for the next column
			if (m <= h) {
				mj = j;
			} // record the position where max score is achieved
			m = m > h ? m : h;   // m is stored at eh[mj+1]

			if (M > oe_del) {
				t = M - oe_del;
			} else {
				t = 0;
			}
			e -= e_del;
			e = e > t ? e : t;   // computed E(i+1,j)
			tmp.e = e;          // save E(i+1,j) for the next row
			if (M > oe_ins) {
				t = M - oe_ins;
			} else {
				t = 0;
			}
			f -= e_ins;
			f = f > t ? f : t;   // computed F(i,j+1)
			eh[j] = tmp;
			//printf("%d %d eh%d ee%d M%d e%d f%d t%d m%d mj%d hp%d h%d b%d en%d \n",(int)i,(int)j,(int)eh[j].h,(int)eh[j].e,(int)M,(int)e,(int)f,(int)t,(int)m,(int)mj,(int)h1,(int)h,(int)beg,(int)end);

		}
		eh[end].h = h1;
		eh[end].e = 0;

		if (j == qlen) {
			if (gscore <= h1) {
				max_ie = i;
				gscore = h1;
			}
		}

		if (m == 0)
			break_i = true;
		if (m > max) {
			max = m, max_i = i, max_j = mj;
			abs_mji = (mj - i > 0) ? (mj - i) : (i - mj);
			max_off = max_off > abs_mji ? max_off : abs_mji;
		}

		// update beg and end for the next round
		LOOP_beg: for (j = beg; (j < end) && eh[j].h == 0 && eh[j].e == 0; ++j) {
#pragma HLS dependence variable=eh inter false
#pragma HLS PIPELINE
		}
		beg = j;

		LOOP_end: for (j = end; (j >= beg) && eh[j].h == 0 && eh[j].e == 0; --j) {
#pragma HLS dependence variable=eh inter false
#pragma HLS PIPELINE
		}
		if (j + 2 < qlen) {
			end = j + 2;
		} else {
			end = qlen;
		}
		//beg = 0; end = qlen; // uncomment this line for debugging
	}

	*_qle = max_j + 1;
	*_tle = max_i + 1;
	*_gtle = max_ie + 1;
	*_gscore = gscore;
	*_max_off = max_off;
	*_max = max;
}
