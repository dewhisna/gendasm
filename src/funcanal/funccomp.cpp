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
#include <sstream>

#include <assert.h>

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
			//				if S(i, j) < (T - X) then S(i, j) <- -oo
			//			}
			//			L <- min{i : S(i, k-i) > -oo }
			//			U <- max{i : S(i, k-i) > -oo }
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
			//	In the following, we will define oo as DBL_MAX and -oo as -DBL_MAX.
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
			if (compareNoCase(file1.GetPrimaryLabel(function1.GetMainAddress()),
								file2.GetPrimaryLabel(function2.GetMainAddress())) != 0) Tp = std::max(0.0, Tp - mat);

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
			//				if i > -oo and S'(i+j, d) >= T[d'] - X then
			//					while i<M, j<N, and a(i+1) = b(j+1) do
			//						i <- i + 1;  j <- j + 1
			//					R(d, k) <- i
			//					T' <- max{T', S'(i+j, d)}
			//				else R(d, k) <- -oo
			//			T[d] <- T'
			//			L <- min{k : R(d, k) > -oo}
			//			U <- max{k : R(d, k) > -oo}
			//			L <- max{L, max{k : R(d, k) = N + k} + 2}	<<< Should be: L <- max{L, max{k : R(d, k) = N + k} + 1}
			//			U <- min{U, min{k : R(d, k) = M } - 2}		<<< Should be: U <- min{U, min{k : R(d, k) = M } - 1}
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
			//	We are also going to define -oo as being -2 since no index can be
			//	lower than 0.  The reason for the -2 instead of -1 is to allow
			//	for the i=R(d,k)+1 calculations to still be below 0.  That is
			//	to say so that -oo + 1 = -oo
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
			int M = a.size();
			int N = b.size();
			const int dmax = ((M+N)*2)+1;
			const int kmax = (M+N+1);
			const int rksize = (kmax*2)+1;
			const double mat = 2;
			const double mis = -2;
			const double X = -1;
			const int floored_d_offset = (int)((X+(mat/2))/(mat-mis));
			#define Sp(x, y) ((double)((x)*(mat/2) - ((y)*(mat-mis))))
			#define R(x, y) (Rm[(x)][(y)+kmax])

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

/*
printf("\n");
*/

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
/*
printf("d=%2ld : k=%2ld, i=%2ld, j=%2ld, M=%2ld, N=%2ld, T=%2ld, Tp=%2ld, Tpp=%2ld", d, k, i, j, M, N, (int)nTemp, (int)Tp, (int)Tpp);
*/
							if (nTemp > Tpp) {
								Tpp = nTemp;
/*
printf(" * Best (%2ld)", (int)Tpp);
*/
									dbest = d;
									kbest = k;


									// Account for hitting the max M or N boundaries:
									if ((i != M) || (j != N)) {
										if (j > N) {
											kbest++;
/*
printf(" >>>>>>  k++ j shift");
*/
										} else {
											if (i > M) {
												kbest--;
/*
printf(" <<<<<<  k-- i shift");
*/
											}
										}
									}

							}
/*
printf("\n");
*/

						} else {
							R(d, k) = -2;
							if (Rvisitmin[d] == k) Rvisitmin[d]++;
							if (Rvisitmax[d] >= k) Rvisitmax[d] = k-1;
						}
					}
					T[d] = Tp;

					L = Rvisitmin[d];
					U = Rvisitmax[d];
					for (k=Rvisitmax[d]; k>=Rvisitmin[d]; k--) if (R(d, k) == (N+k)) break;
					if (k<Rvisitmin[d]) k = INT_MIN;
					L = std::max(L, k+1);
					for (k=Rvisitmin[d]; k<=Rvisitmax[d]; k++) if (R(d, k) == M) break;
					if (k>Rvisitmax[d]) k = INT_MAX;
					U = std::min(U, k-1);
				} while (L <= U+2);
			}

			// If the two PrimaryLabels at the function's address don't match, decrement match by 1*mat.
			//		This helps if there are two are more functions that the user has labeled that
			//		are identical except for the labels:
			if (compareNoCase(file1.GetPrimaryLabel(function1.GetMainAddress()),
								file2.GetPrimaryLabel(function2.GetMainAddress())) != 0) Tp = std::max(0.0, Tp - mat);

			// Normalize it:
			nRetVal = Tp/(std::max(M,N)*mat);

			// Build Edit Script:
			if (bBuildEditScript) {
				int last_i, last_j;
				int cur_i, cur_j;
				int k_rest;

				if (dbest > 0) {
					g_EditScript.resize(dbest);
					k = kbest;
					last_i = M+1;
					last_j = N+1;

/*
printf("\n%s with %s:\n", LPCTSTR(pFunction1->GetMainName()), LPCTSTR(pFunction2->GetMainName()));
*/

					for (d=dbest-1; d>=0; d--) {
						i = std::max((R(d, k-1) + 1), std::max((R(d, k) + 1), (R(d, k+1))));

/*
printf("(%3ld, %3ld) : %3ld(%5ld), %3ld(%5ld), %3ld(%5ld) :", d, k,
						(R(d, k-1) + 1), (int)Sp((R(d, k-1))*2-k+1, d),
						(R(d, k) + 1), (int)Sp((R(d, k))*2-k, d),
						(R(d, k+1)), (int)Sp((R(d, k+1))*2-k-1, d));

for (j=Rvisitmin[dbest-1]; j<=Rvisitmax[dbest-1]; j++) {
	if (j == k-1) printf("("); else printf(" ");
	if (R(d,j)<0) printf("   "); else printf("%3ld", R(d, j));
	if (j == k+1) printf(")"); else printf(" ");
}
printf("\n");
*/

						j = i-k;

						if (i == (R(d, k-1) + 1)) {
							std::sprintf(arrTemp, "%d>%d", i-1, j);
							cur_i = i-1;
							cur_j = j;
							k--;
							k_rest = 1;
						} else {
							if (i == (R(d, k+1))) {
								std::sprintf(arrTemp, "%d<%d", i, j-1);
								cur_i = i;
								cur_j = j-1;
								k++;
								k_rest = -1;
							} else {
								// if (i == (R(d, k) + 1))
								std::sprintf(arrTemp, "%d-%d", i-1, j-1);
								cur_i = i-1;
								cur_j = j-1;
								// k=k;
								k_rest = 0;
							}
						}
						g_EditScript[d] = arrTemp;
						// The following test is needed since our insertion/deletion indexes are
						//		one greater than the stored i and/or j values from the R matrix.
						//		It is possible that the previous comparison added some extra
						//		entries to the R matrix than what was really needed.  This will
						//		cause extra erroneous entries to appear in the edit script.
						//		However, since the indexes should be always increasing, we simply
						//		filter out extra entries added to the end that don't satisfy
						//		this condition:
						if ((k_rest == 0) &&
							((cur_i == last_i) && (cur_j == last_j))) {
							g_EditScript.erase(g_EditScript.begin() + d);
						}

						last_i = cur_i;
						last_j = cur_j;
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
		assert(nLeftPos == nLeftIndex);
		assert(nRightPos == nRightIndex);

		#ifdef _DEBUG
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

		#ifdef _DEBUG
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

