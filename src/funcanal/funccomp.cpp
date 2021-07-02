//
//	Fuzzy Function Comparison Logic
//
//
//	Fuzzy Function Analyzer
//	for the Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

//
//	The Edit Script is defined as follows:
//
//		During comparison an optimal edit script is calculated and is stored as a CStringArray.  Each
//			entry is a string of the following format, which is similar to the Diff format except
//			that each entry is unique rather than specifying ranges:
//				xxxCyyy
//
//					Where:
//						xxx = Left side index
//						yyy = Right side index
//						C is one of the following symbols:
//							'>' - Delete xxx from left at point yyy in right or
//									insert xxx from left at point yyy in right
//							'-' - Replace xxx in left with yyyy in right
//							'<' - Insert yyy from right at left point xxx or
//									delete yyy from right at point xxx in left
//

#include "funcdesc.h"
#include <errmsgs.h>
#include <stringhelp.h>

#include <math.h>
#include <limits.h>
#include <float.h>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <utility>

#include <assert.h>

#define DEBUG_OES_SCRIPT 0			// Set to '1' (or non-zero) to debug OES generation code

static CStringArray g_EditScript;
static bool g_bEditScriptValid = false;


double CompareFunctions(FUNC_COMPARE_METHOD nMethod,
											const CFuncDescFile &file1, std::size_t nFile1FuncNdx,
											const CFuncDescFile &file2, std::size_t nFile2FuncNdx,
											bool bBuildEditScript)
{
	double nRetVal = 0;
	CStringArray zFunc1;
	CStringArray zFunc2;
	char arrTemp[30];

	g_bEditScriptValid = false;
	g_EditScript.clear();

	assert(nFile1FuncNdx < file1.GetFuncCount());
	const CFuncDesc &function1 = file1.GetFunc(nFile1FuncNdx);
	assert(nFile2FuncNdx < file2.GetFuncCount());
	const CFuncDesc &function2 = file2.GetFunc(nFile2FuncNdx);

	function1.ExportToDiff(zFunc1);
	function2.ExportToDiff(zFunc2);
	if (zFunc1.empty() || zFunc2.empty())return nRetVal;

	if ((bBuildEditScript) && (nMethod == FCM_DYNPROG_XDROP)) {
		// Note: XDROP Method currently doesn't support building
		//		of edit scripts, so if caller wants to build an
		//		edit script and has selected this method, replace
		//		it with the next best method that does support
		//		edit scripts:
		nMethod = FCM_DYNPROG_GREEDY;
	}

	switch (nMethod) {
		case FCM_DYNPROG_XDROP:
		{
			//
			//	The following algorithm comes from the following source:
			//			"A Greedy Algorithm for Aligning DNA Sequences"
			//			Zheng Zhang, Scott Schwartz, Lukas Wagner, and Webb Miller
			//			Journal of Computational Biology
			//			Volume 7, Numbers 1/2, 2000
			//			Mary Ann Liebert, Inc.
			//			Pp. 203-214
			//
			//			p. 205 : Figure 2 : "A dynamic-programming X-drop algorithm"
			//
			//		T' <- T <- S(0,0) <- 0
			//		k <- L <- U <- 0
			//		repeat {
			//			k <- k + 1
			//			for i <- ceiling(L) to floor(U)+1 in steps of 1/2 do {
			//				j <- k - i
			//				if i is an integer then {
			//					S(i, j) <- Max of:
			//								S(i-1/2, j-1/2) + mat/2		if L <= i-1/2 <= U and a(i) = b(j)
			//								S(i-1/2, j-1/2) + mis/2		if L <= i-1/2 <= U and a(i) != b(j)
			//								S(i, j-1) + ind				if i <= U
			//								S(i-1, j) + ind				if L <= i-1
			//				} else {
			//					S(i, j) <- S(i-1/2, j-1/2)
			//								+ mat/2	if a(i+1/2) = b(j+1/2)
			//								+ mis/2	if a(i+1/2) != b(j+1/2)
			//				}
			//				T' <- max{T', S(i, j)}
			//				if S(i, j) < (T - X) then S(i, j) <- -∞
			//			}
			//			L <- min{i : S(i, k-i) > -∞ }
			//			U <- max{i : S(i, k-i) > -∞ }
			//			L <- max{L, k+1-N}				<<< Should be: L <- max{L, k+1/2-N}
			//			U <- min{U, M-1}				<<< Should be: U <- min(U, M-1/2}
			//			T <- T'
			//		} until L > U+1
			//		report T'
			//
			//	Given:
			//		arrays: a(1..M), b(1..N) containing the two strings to compare
			//		mat : >0 : Weighting of a match
			//		mis : <0 : Weighting of a mismatch
			//		ind : <0 : Weighting of an insertion/deletion
			//		X = Clipping level : If scoring falls more than X below the
			//				best computed score thus far, then we don't consider
			//				additional extensions for that alignment.  Should
			//				be >= 0 or -1 for infinity.
			//
			//	Returning:
			//		T' = Composite similarity score
			//
			//	For programmatic efficiency, all S indexes have been changed from
			//		0, 1/2 to even/odd and the i loop runs as integers of even/odd
			//		instead of 1/2 increments:
			//
			//	Testing has also been done and it has been proven that the order of
			//		the two arrays has no effect on the outcome.
			//
			//	In the following, we will define ∞ as DBL_MAX and -∞ as -DBL_MAX.
			//
			//	To hold to the non-Generalized Greedy Algorithm requirements, set
			//		ind = mis - mat/2
			//

			CStringArray &a = zFunc1;
			CStringArray &b = zFunc2;
			double Tp, T;
			double **S;
			int i, j, k, L, U;
			double nTemp;
			int M = a.size();
			int N = b.size();
			const double mat = 2;
			const double mis = -2;
			const double ind = -3;
			const double X = -1;

			// Allocate Memory:
			S = new double*[((M+1)*2)];
			if (S == nullptr) {
				THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_OUT_OF_MEMORY);
				return nRetVal;
			}
			for (i=0; i<((M+1)*2); i++) {
				S[i] = new double[((N+1)*2)];
				if (S[i] == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_OUT_OF_MEMORY);
			}

			// Initialize:
			for (i=0; i<((M+1)*2); i++) {
				for (j=0; j<((N+1)*2); j++) {
					S[i][j] = -DBL_MAX;
				}
			}

			// Algorithm:
			Tp = T = S[0][0] = 0;
			k = L = U = 0;

			do {
				k = k + 2;
				for (i = L+((L & 0x1) ? 1 : 0); i <= (U - ((U & 0x1) ? 1 : 0) + 2); i++) {
					j = k - i;
					assert(i >= 0);
					assert(i < ((M+1)*2));
					assert(j >= 0);
					assert(j < ((N+1)*2));
					if ((i&1) == 0) {
						nTemp = -DBL_MAX;
						if ((L <= (i-1)) &&
							((i-1) <= U)) {
							// TODO : Improve A/B Comparison:
							if (compareNoCase(a.at((i/2)-1), b.at((j/2)-1)) == 0) {
								nTemp = std::max(nTemp, S[i-1][j-1] + mat/2);
							} else {
								nTemp = std::max(nTemp, S[i-1][j-1] + mis/2);
							}
						}
						if (i <= U) {
							nTemp = std::max(nTemp, S[i][j-2] + ind);
						}
						if (L <= (i-2)) {
							nTemp = std::max(nTemp, S[i-2][j] + ind);
						}
						S[i][j] = nTemp;
					} else {
						// TODO : Improve A/B Comparison:
						if (compareNoCase(a.at(((i+1)/2)-1), b.at(((j+1)/2)-1)) == 0) {
							S[i][j] = S[i-1][j-1] + mat/2;
						} else {
							S[i][j] = S[i-1][j-1] + mis/2;
						}
					}
					Tp = std::max(Tp, S[i][j]);
					if ((X>=0) && (S[i][j] < (T-X))) S[i][j] = -DBL_MAX;
				}

				for (L = 0; L < ((M+1)*2); L++) {
					if ((k - L) < 0) continue;
					if ((k - L) >= ((N+1)*2)) continue;
					if (S[L][k-L] > -DBL_MAX) break;
				}
				if (L == ((M+1)*2)) L=INT_MAX;

				for (U=((M+1)*2)-1; U>=0; U--) {
					if ((k - U) < 0) continue;
					if ((k - U) >= ((N+1)*2)) continue;
					if (S[U][k-U] > -DBL_MAX) break;
				}
				if (U == -1) U=INT_MIN;

				L = std::max(L, k + 1 - (N*2));
				U = std::min(U, (M*2) - 1);
				T = Tp;
			} while (L <= U+2);

			// If the two PrimaryLabels at the function's address don't match, decrement match by 1*mat.
			//		This helps if there are two are more functions that the user has labeled that
			//		are identical except for the labels:
			if (compareNoCase(file1.GetPrimaryLabel(CFuncDescFile::MEMORY_TYPE::MT_ROM, function1.GetMainAddress()),
								file2.GetPrimaryLabel(CFuncDescFile::MEMORY_TYPE::MT_ROM, function2.GetMainAddress())) != 0) Tp = std::max(0.0, Tp - mat);

			// Normalize it:
			nRetVal = Tp/(std::max(M,N)*mat);

			// Deallocate Memory:
			for (i=0; i<((M+1)*2); i++) delete[] (S[i]);
			delete[] S;
		}
		break;

		case FCM_DYNPROG_GREEDY:
		{
			//
			//	The following algorithm comes from the following source:
			//			"A Greedy Algorithm for Aligning DNA Sequences"
			//			Zheng Zhang, Scott Schwartz, Lukas Wagner, and Webb Miller
			//			Journal of Computational Biology
			//			Volume 7, Numbers 1/2, 2000
			//			Mary Ann Liebert, Inc.
			//			Pp. 203-214
			//
			//			p. 209 : Figure 4 : "Greedy algorithm that is equivalent to the algorithm
			//								of Figure 2 if ind = mis - mat/2."
			//
			//		i <- 0
			//		while i < min{M, N} and a(i+1) = b(i+1) do i <- i + 1
			//		R(0, 0) <- i
			//		T' <- T[0] <- S'(i+i, 0)
			//		d <- L <- U <- 0
			//		repeat
			//			d <- d + 1
			//			d' <- d - floor( (X + mat/2)/(mat - mis) ) - 1
			//			for k <- L - 1 to U + 1 do
			//				i <- max of:
			//							R(d-1, k-1) + 1		if L < k
			//							R(d-1, k) + 1		if L <= k <= U
			//							R(d-1, k+1)			if k < U
			//				j <- i - k
			//				if i > -∞ and S'(i+j, d) >= T[d'] - X then
			//					while i<M, j<N, and a(i+1) = b(j+1) do
			//						i <- i + 1;  j <- j + 1
			//					R(d, k) <- i
			//					T' <- max{T', S'(i+j, d)}
			//				else R(d, k) <- -∞
			//			T[d] <- T'
			//			L <- min{k : R(d, k) > -∞}
			//			U <- max{k : R(d, k) > -∞}
			//			L <- max{L, max{k : R(d, k) = N + k} + 2}	<<< Should be: L <- max{L, max{k : R(d, k) = N + k} } *** See Below
			//			U <- min{U, min{k : R(d, k) = M } - 2}		<<< Should be: U <- min{U, min{k : R(d, k) = M } }
			//		until L > U + 2
			//		report T'
			//
			//	Given:
			//		arrays: a(1..M), b(1..N) containing the two strings to compare
			//		mat : >0 : Weighting of a match
			//		mis : <0 : Weighting of a mismatch
			//		ind : <0 : Weighting of an insertion/deletion
			//		(Satisfying: ind = mis - mat/2)
			//		X = Clipping level : If scoring falls more than X below the
			//				best computed score thus far, then we don't consider
			//				additional extensions for that alignment.  Should
			//				be >= 0 or -1 for infinity.
			//		S'(i+j, d) = (i+j)*mat/2 - d*(mat-mis)
			//		or S'(k, d) = k*mat/2 - d*(mat-mis)
			//
			//	Returning:
			//		T' = Composite similarity score
			//
			//	Testing has also been done and it has been proven that the order of
			//		the two arrays has no effect on the outcome.
			//
			//	In the following it will be assumed that the number of mismatches
			//	(i.e. array bounds) can't exceed M+N since the absolute maximum
			//	edit script to go from an array of M objects to an array of N
			//	objects is to perform M deletions and N insertions.  However,
			//	these differences can be tracked either forward or backward, so
			//	the memory will be allocated for the full search field.
			//
			//	We are also going to define -∞ as being -2 since no index can be
			//	lower than 0.  The reason for the -2 instead of -1 is to allow
			//	for the i=R(d,k)+1 calculations to still be below 0.  That is
			//	to say so that -∞ + 1 = -∞
			//
			//
			//	*** The L and U computations above with the +2/-2 is part of the diagonal pruning
			//		in the original algorithm as noted in the paper:
			//
			//		"But pruning does not affect the computed result. In particular, consider lines
			//		21–22 of Figure 4, which handle situations when the grid boundary is reached.
			//		If extension down diagonal k reaches i = M , then there is no need to consider
			//		diagonals larger than i, since positions searched at a later phase (larger d)
			//		will have smaller antidiagonal index, and hence smaller score. Thus we set
			//		U ← k - 2, so that when diagonals with index at most U + 1 are considered in
			//		the next phase, unnecessary work will be avoided. L is handled similarly."
			//
			//		HOWEVER, while pruning doesn't affect the computed match percentage result,
			//		this optimization causes the "dbest" and "kbest" calculated for the entire
			//		set to not be complete in the terminal nodes which wreaks havoc on the
			//		optimal edit script generation logic, since it ends up starting at the wrong
			//		kbest, which shifts the entire tree off into the weeds.  Originally, I had
			//		discovered this and reduced it to +/-1, which helped in some scenrios, but
			//		was not the complete solution.  The only final solution was to remove this
			//		optimization entirely and extend L and U to the full range of 'k'.  Thus,
			//		we have this 'should be' change to the algorithm.
			//

			CStringArray &a = zFunc1;
			CStringArray &b = zFunc2;
			double Tp;				// T' = Overall max for entire comparison
			double Tpp;				// T'' = Overall max for current d value
			double *T;
			double nTemp;
			int **Rm;
			int *Rvisitmin;			// Minimum k-index of R visited for a particular d (for speed)
			int *Rvisitmax;			// Maximum k-index of R visited for a particular k (for speed)
			int i, j, k, L, U;
			int d, dp;
			int dbest, kbest;
			const int M = a.size();
			const int N = b.size();
			const int dmax = ((M+N)*2)+1;
			const int kmax = (M+N+1);
			const int rksize = (kmax*2)+1;
			const double mat = 2;
			const double mis = -2;
			const double X = -1;
			const int floored_d_offset = (int)((X+(mat/2))/(mat-mis));
			#define Sp(x, y) ((double)((x)*(mat/2) - ((y)*(mat-mis))))
			#define R(x, y) (Rm[(x)][(y)+kmax])		// Allow for negative indices of +/- kmax

			// Allocate Memory:
			T = new double[dmax];
			if (T == nullptr) {
				THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_OUT_OF_MEMORY);
				return nRetVal;
			}

			Rvisitmin = new int[dmax];
			if (Rvisitmin == nullptr) {
				delete[] T;
				THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_OUT_OF_MEMORY);
				return nRetVal;
			}

			Rvisitmax = new int[dmax];
			if (Rvisitmax == nullptr) {
				delete[] T;
				delete[] Rvisitmin;
				THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_OUT_OF_MEMORY);
				return nRetVal;
			}

			Rm = new int*[dmax];
			if (Rm == nullptr) {
				delete[] T;
				delete[] Rvisitmin;
				delete[] Rvisitmax;
				THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_OUT_OF_MEMORY);
				return nRetVal;
			}
			for (i=0; i<dmax; i++) {
				Rm[i] = new int[rksize];
				if (Rm[i] == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_OUT_OF_MEMORY);
			}

			// Initialize:
			for (i=0; i<dmax; i++) {
				T[i] = 0;
				Rvisitmin[i] = kmax+1;
				Rvisitmax[i] = -kmax-1;
				for (j=0; j<rksize; j++) {
					Rm[i][j] = -2;
				}
			}

			// Algorithm:
			i=0;
			// TODO : Improve A/B Comparison:
			while ((i<std::min(M, N)) && (compareNoCase(a.at(i), b.at(i)) == 0)) i++;
			R(0, 0) = i;
			dbest = kbest = 0;
			Tp = T[0] = Sp(i+i, 0);
			d = L = U = 0;
			Rvisitmin[0] = 0;
			Rvisitmax[0] = 0;

#if DEBUG_OES_SCRIPT
printf("\n");
#endif

			if ((i != M) || (i != N)) {
				do {
					d++;
					dp = d - floored_d_offset - 1;
					Tpp = -DBL_MAX;
					for (k=(L-1); k<=(U+1); k++) {
						assert(d > 0);
						assert(d < dmax);
						assert(abs(k) <= kmax);
						i = -2;
						if (L < k)			i = std::max(i, R(d-1, k-1)+1);
						if ((L <= k) &&
							(k <= U))		i = std::max(i, R(d-1, k)+1);
						if (k < U)			i = std::max(i, R(d-1, k+1));
						j = i - k;
						if ((i >= 0) && (j >= 0) && ((X<0) || (Sp(i+j, d) >= (((dp >= 0) ? T[dp] : 0) - X)))) {
							// TODO : Improve A/B Comparison:
							while ((i<M) && (j<N) && (compareNoCase(a.at(i), b.at(j)) == 0)) {
								i++; j++;
							}

							R(d, k) = i;
							if (Rvisitmin[d] > k) Rvisitmin[d] = k;
							if (Rvisitmax[d] < k) Rvisitmax[d] = k;
							nTemp = Sp(i+j, d);
							Tp = std::max(Tp, nTemp);

#if DEBUG_OES_SCRIPT
printf("d=%2d : k=%2d, i=%2d, j=%2d, M=%2d, N=%2d, T=%2d, Tp=%2d, Tpp=%2d", d, k, i, j, M, N, (int)nTemp, (int)Tp, (int)Tpp);
#endif

							if (nTemp > Tpp) {
								Tpp = nTemp;
								dbest = d;
								kbest = k;

#if DEBUG_OES_SCRIPT
printf(" * Best (%2d)", (int)Tpp);
#endif

							}

#if DEBUG_OES_SCRIPT
printf("\n");
#endif

						} else {
							R(d, k) = -2;
							if (Rvisitmin[d] == k) Rvisitmin[d]++;
							if (Rvisitmax[d] >= k) Rvisitmax[d] = k-1;
						}
					}
					T[d] = Tp;

					L = Rvisitmin[d];
					U = Rvisitmax[d];
					for (k=Rvisitmax[d]+1; k>=Rvisitmin[d]-1; k--) if (R(d, k) == (N+k)) break;
					if (k<Rvisitmin[d]-1) k = INT_MIN;
					L = std::max(L, k);
					for (k=Rvisitmin[d]-1; k<=Rvisitmax[d]+1; k++) if (R(d, k) == M) break;
					if (k>Rvisitmax[d]+1) k = INT_MAX;
					U = std::min(U, k);
				} while (L <= U+2);
			}

			// If the two PrimaryLabels at the function's address don't match, decrement match by 1*mat.
			//		This helps if there are two are more functions that the user has labeled that
			//		are identical except for the labels:
			if (compareNoCase(file1.GetPrimaryLabel(CFuncDescFile::MEMORY_TYPE::MT_ROM, function1.GetMainAddress()),
								file2.GetPrimaryLabel(CFuncDescFile::MEMORY_TYPE::MT_ROM, function2.GetMainAddress())) != 0) Tp = std::max(0.0, Tp - mat);

			// Normalize it:
			nRetVal = Tp/(std::max(M,N)*mat);

			// Build Edit Script:
			if (bBuildEditScript) {
				if (dbest > 0) {
					// Note: dbest will always be the number of OES operations
					//	required unless the very last one is a no-op case, in
					//	which it will be one over (see bDeleteEntry below)
					g_EditScript.resize(dbest);
					k = kbest;

#if DEBUG_OES_SCRIPT
printf("\n%s with %s:\n", function1.GetMainName().c_str(), function2.GetMainName().c_str());
#endif

					for (d=dbest-1; d>=0; d--) {	// Use (d-1) for loop so all calls to R() don't below don't have to subtract 1
						bool bDeleteEntry = false;			// True if this entry should get deleted

						// The curR pair will be the current operation with the
						//		current adjusted 'R' value being tested:
						//	Operation will be:
						//		-1 for deletion    (>)
						//		0  for replacement (-)
						//		1  for insertion   (<)
						std::pair<int, int> curR(0, (R(d, k) + 1));				// Default to '-' (replacement)
						if ((R(d, k-1) + 1) > curR.second) {
							curR = std::pair<int, int>(1, (R(d, k-1) + 1));		// Check for '>' (deletion)
						}
						if ((R(d, k+1)) > curR.second) {
							curR = std::pair<int, int>(-1, (R(d, k+1)));		// Check for '<' (insertion)
						}

						i = curR.second;
						j = i-k;

#if DEBUG_OES_SCRIPT
printf("(%3d, %3d) : %3d(%5d), %3d(%5d), %3d(%5d) :", d, k,
						(R(d, k-1) + 1), (int)Sp((R(d, k-1))*2-k+1, d),
						(R(d, k) + 1), (int)Sp((R(d, k))*2-k, d),
						(R(d, k+1)), (int)Sp((R(d, k+1))*2-k-1, d));

for (int ktemp=Rvisitmin[dbest-1]-1; ktemp<=Rvisitmax[dbest-1]+1; ktemp++) {
	if (ktemp == k-1) printf("("); else printf(" ");
	if (R(d,ktemp)<0) printf("   "); else printf("%3d", R(d, ktemp));
	if (ktemp == k+1) printf(")"); else printf(" ");
}
printf("\n");
#endif

						if (curR.first == 1) {
							std::sprintf(arrTemp, "%d>%d", i-1, j);
							k--;
						} else if (curR.first == -1) {
							std::sprintf(arrTemp, "%d<%d", i, j-1);
							k++;
						} else {	// curR.first == 0
							std::sprintf(arrTemp, "%d-%d", i-1, j-1);
							int cur_i = i-1;
							int cur_j = j-1;
							// k=k;

							if ((cur_i < M) && (cur_j >= N)) {
#if DEBUG_OES_SCRIPT
printf(" - becomes > : ");
#endif
								std::sprintf(arrTemp, "%d>%d", cur_i, N);
								//k--;  // ?? these are always last entries, so no need to update k
								curR.first = 1;		// But update curR so we have a consistent operation value
							} else if ((cur_i >= M) && (cur_j < N)) {
#if DEBUG_OES_SCRIPT
printf(" - becomes < : ");
#endif
								std::sprintf(arrTemp, "%d<%d", M, cur_j);
								//k++;  // ?? these are always last entries, so no need to update k
								curR.first = -1;	// But update curR so we have a consistent operation value
							} else if ((cur_i >= M) && (cur_j >= N)) {
								bDeleteEntry = true;
							}
						}
						g_EditScript[d] = arrTemp;

#if DEBUG_OES_SCRIPT
printf("%d : %s", d, arrTemp);
#endif

						// The following delete is needed because our very last entry will
						//		either be a "- → >" or "- → <" conversion above or it will
						//		be a "-" entry for one beyond both function arrays.  In
						//		other words, the last entry is either:
						//				LastElement > End	(handled above)
						//				End < LastElement	(handled above)
						//				End - End			(needs to be deleted here)
						if (bDeleteEntry) {
							// This is always a last entry and where both: cur_i==M and cur_j==N
							g_EditScript.erase(g_EditScript.begin() + d);
#if DEBUG_OES_SCRIPT
printf("  *** erasing: %d", d);
#endif
						}

#if DEBUG_OES_SCRIPT
printf("\n");
#endif

					}
				}


				// Note: if the two are identical, array stays empty:
				g_bEditScriptValid = true;
			}

			// Deallocate Memory:
			delete[] T;
			delete[] Rvisitmin;
			delete[] Rvisitmax;
			for (i=0; i<dmax; i++) delete[] (Rm[i]);
			delete[] Rm;
		}
		break;

		default:
			break;
	}

	return nRetVal;
}

bool GetLastEditScript(CStringArray &anArray)
{
	anArray.clear();
	if (!g_bEditScriptValid) return false;

	anArray = g_EditScript;

	return true;
}

TString DiffFunctions(FUNC_COMPARE_METHOD nMethod,
						const CFuncDescFile &file1, std::size_t nFile1FuncNdx,
						const CFuncDescFile &file2, std::size_t nFile2FuncNdx,
						OUTPUT_OPTIONS nOutputOptions,
						double &nMatchPercent,
						CSymbolMap *pSymbolMap)
{
	TString strRetVal;

	CStringArray oes;
	CStringArray Func1Lines;
	CStringArray Func2Lines;

	assert(nFile1FuncNdx < file1.GetFuncCount());
	const CFuncDesc &function1 = file1.GetFunc(nFile1FuncNdx);
	assert(nFile2FuncNdx < file2.GetFuncCount());
	const CFuncDesc &function2 = file2.GetFunc(nFile2FuncNdx);

	nMatchPercent = CompareFunctions(nMethod, file1, nFile1FuncNdx, file2, nFile2FuncNdx, true);
	if (!GetLastEditScript(oes)) return strRetVal;


/*
// The following is for special debugging only:
	for (auto const & oesLine : oes) {
		strRetVal += "    " + oesLine + "\n";
//	return strRetVal;
	strRetVal += "\n";
*/

	std::string::size_type nLeftMax = 0;
	for (auto const &funcObj : function1) {
		TString strTemp = funcObj->CreateOutputLine(nOutputOptions);
		nLeftMax = std::max(nLeftMax, strTemp.size());
		Func1Lines.push_back(strTemp);
	}

	std::string::size_type nRightMax = 0;
	for (auto const &funcObj :function2) {
		TString strTemp = funcObj->CreateOutputLine(nOutputOptions);
		nRightMax = std::max(nRightMax, strTemp.size());
		Func2Lines.push_back(strTemp);
	}


	strRetVal += padString(padString(TString(), function1.GetMainName().size()/2) + function1.GetMainName(), nLeftMax);
	strRetVal += "      ";
	strRetVal += padString(padString(TString(), function2.GetMainName().size()/2) + function2.GetMainName(), nRightMax);
	strRetVal += "\n";

	strRetVal.append(nLeftMax, '-');
	strRetVal += "      ";
	strRetVal.append(nRightMax, '-');
	strRetVal += "\n";

	unsigned int nLeftPos = 0;
	unsigned int nRightPos = 0;
	unsigned int nLeftIndex = 0;
	unsigned int nRightIndex = 0;

	for (auto const & strEntry : oes) {
		TString::size_type pos = strEntry.find_first_of("<->");
		if (pos == strEntry.npos) continue;
		nLeftIndex = strtoul(strEntry.substr(0, pos).c_str(), nullptr, 0);
		nRightIndex = strtoul(strEntry.substr(pos+1).c_str(), nullptr, 0);

		assert(nLeftIndex <= Func1Lines.size());
		assert(nRightIndex <= Func2Lines.size());

		for (; ((nLeftPos < nLeftIndex) && (nRightPos < nRightIndex)); nLeftPos++, nRightPos++) {
			assert(nLeftPos < function1.size());
			assert(nRightPos < function2.size());

			strRetVal += padString(Func1Lines.at(nLeftPos), nLeftMax);
			if (function1.at(nLeftPos)->isExactMatch(*function2.at(nRightPos))) {
				strRetVal += "  ==  ";
			} else {
				strRetVal += "  --  ";
			}
			strRetVal += Func2Lines.at(nRightPos) + "\n";
			if (pSymbolMap) pSymbolMap->AddObjectMapping(*function1.at(nLeftPos), *function2.at(nRightPos));
		}

#if DEBUG_OES_SCRIPT
if ((nLeftPos != nLeftIndex) || (nRightPos != nRightIndex)) {
	std::cerr << "\n";
	std::cerr << "@" << function1.GetMainName() << "(" << (nFile1FuncNdx+1)
			<< ")|" << function2.GetMainName() << "(" << (nFile2FuncNdx+1) << ")\n";
	std::copy(oes.cbegin(), oes.cend(), std::ostream_iterator<TString>(std::cerr, "\n"));
}
#endif

		assert(nLeftPos == nLeftIndex);
		assert(nRightPos == nRightIndex);

		#if 0
		// The following is for special debugging only:
		if (nLeftPos != nLeftIndex) {
			std::ostringstream ssTemp;
			ssTemp << "\n\n*** ERROR: nLeftPos = " << nLeftPos << ",  Expected = " << nLeftIndex << "\n\n";
			strRetVal += ssTemp.str();
			nLeftPos = nLeftIndex;
		}

		if (nRightPos != nRightIndex) {
			std::ostringstream ssTemp;
			ssTemp << "\n\n*** ERROR: nRightPos = " << nRightPos << ",  Expected = " << nRightIndex << "\n\n";
			strRetVal += ssTemp.str();
			nRightPos = nRightIndex;
		}
		#endif

		#if 0
		switch (strEntry.at(pos)) {
			case '<':
				if (nRightPos >= Func2Lines.size()) {
					strRetVal += "\n\n*** ERROR: Right-Side Index Out-Of-Range!\n\n";
					return strRetVal;
				}
				break;
			case '-':
				if (nLeftPos >= Func1Lines.size()) {
					strRetVal += "\n\n*** ERROR: Left-Side Index Out-Of-Range!\n\n";
					if (nRightPos >= Func2Lines.size()) {
						strRetVal += "\n\n*** ERROR: Right-Side Index Out-Of-Range!\n\n";
					}
					return strRetVal;
				}
				if (nRightPos >= Func2Lines.size()) {
					strRetVal += "\n\n*** ERROR: Right-Side Index Out-Of-Range!\n\n";
					return strRetVal;
				}
				break;
			case '>':
				if (nLeftPos >= Func1Lines.size()) {
					strRetVal += "\n\n*** ERROR: Left-Side Index Out-Of-Range!\n\n";
					return strRetVal;
				}
				break;
		}
		#endif

		switch (strEntry.at(pos)) {
			case '<':				// Insert in Left
				strRetVal += padString(TString(), nLeftMax) + "  <<  " + Func2Lines.at(nRightPos) + "\n";
				nRightPos++;
				break;

			case '-':				// Change
				strRetVal += padString(Func1Lines.at(nLeftPos), nLeftMax) + "  ->  " + Func2Lines.at(nRightPos) + "\n";
				if (pSymbolMap) pSymbolMap->AddObjectMapping(*function1.at(nLeftPos), *function2.at(nRightPos));
				nLeftPos++;
				nRightPos++;
				break;

			case '>':				// Insert in Right
				strRetVal += padString(Func1Lines.at(nLeftPos), nLeftMax) + "  >>  \n";
				nLeftPos++;
				break;
		}
	}

	nLeftIndex = Func1Lines.size();
	nRightIndex = Func2Lines.size();
	for (; ((nLeftPos < nLeftIndex) && (nRightPos < nRightIndex)); nLeftPos++, nRightPos++) {
		strRetVal += padString(Func1Lines.at(nLeftPos), nLeftMax);
		if (function1.at(nLeftPos)->isExactMatch(*function2.at(nRightPos))) {
			strRetVal += "  ==  ";
		} else {
			strRetVal += "  --  ";
		}
		strRetVal += Func2Lines.at(nRightPos) + "\n";
		if (pSymbolMap) pSymbolMap->AddObjectMapping(*function1.at(nLeftPos), *function2.at(nRightPos));
	}

	return strRetVal;
}

