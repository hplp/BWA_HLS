// Configure the QR and DB max space on the FPGA
#define QR_MAX 256
#define DB_MAX 256
// Configure size of the similarity matrix
__constant ushort m = 5;

typedef struct {
	short h, e;
} eh_t;

// Similarity function implementation
short similarity(ushort a, ushort b, ushort match, ushort mismatch, ushort ambiguous) {
#pragma HLS inline
	if ((a == m - 1) || (b == m - 1)) {
		return 0 - ambiguous;
	} else if (a == b) {
		return match;
	} else
		return 0 - mismatch;
}

kernel __attribute__((reqd_work_group_size(1, 1, 1)))
// ksw_extend2_hls algorithm
void ksw_ext2(const ushort qlen, global const uchar* source_q, const ushort tlen, global const uchar* source_t, const ushort match, const ushort mismatch,
		const ushort ambiguous, const ushort o_del, const ushort e_del, const ushort o_ins, const ushort e_ins, const ushort w, const ushort h0, global int* buffer_m) {

	uchar query[QR_MAX];
	uchar target[DB_MAX];

//	printf("CL %d query: ", qlen);
	readQ: for (ushort i = 0; i < qlen; i++) {
		query[i] = source_q[i];
//		printf("%d ", query[i]);
	}
//	printf("\n");
//	printf("CL %d target: ", tlen);
	readT: for (ushort i = 0; i < tlen; i++) {
		target[i] = source_t[i];
//		printf("%d ", target[i]);
	}
//	printf("CLt \n");
//	printf("match=%d mismatch=-%d ambiguous=-%d o_del=%d e_del=%d o_ins=%d e_ins=%d w=%d h0=%d\n", match, mismatch, ambiguous, o_del, e_del, o_ins, e_ins, w, h0);

	ushort i, oe_del = o_del + e_del, oe_ins = o_ins + e_ins;
	short beg = 0, end = qlen, max = h0, max_i = -1, max_j = -1, max_ie = -1, gscore = -1, max_off = 0, eh_h_prev;
	bool break_i = false;

	// VivadoHLS synthesizes arrays as BRAM
	eh_t eh[QR_MAX + 1]; // score array

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
		short f = 0, h1, m = 0, mj = -1, j, abs_mji;

		// apply the band and the constraint (if provided)
		if (beg < i - w)
			beg = i - w;
		if (end > i + w + 1)
			end = i + w + 1;
		if (end > qlen)
			end = qlen;

		// compute the first column
		if (beg == 0) {
			h1 = h0 - (o_del + e_del * (i + 1));
			if (h1 < 0)
				h1 = 0;
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
			short h, t, q, M = tmp.h, e = tmp.e; // get H(i-1,j-1) and E(i-1,j)

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
	buffer_m[0] = max_j + 1;
	buffer_m[1] = max_i + 1;
	buffer_m[2] = max_ie + 1;
	buffer_m[3] = gscore;
	buffer_m[4] = max_off;
	buffer_m[5] = max;
	//printf("%d %d %d %d %d %d \n", buffer_m[0], buffer_m[1], buffer_m[2],
	//		buffer_m[3], buffer_m[4], buffer_m[5]);
}
