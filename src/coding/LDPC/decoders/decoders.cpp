#include "decoders.h"
#include <tuple>
#include <iostream>
#include <iomanip>


std::vector<GF2> hard_decision(std::vector<double> const& llrs)
{
	std::vector<GF2> res(llrs.size());
	for (size_t i{0}; i < llrs.size(); ++i) {
		res[i] = llrs[i] < 0 ? 1 : 0;
	}
	
	return res;
}


double phi(double x)
{
	return -log(tanh(x / 2.0));
}


auto make_ab(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& H) -> std::tuple<std::vector<std::vector<size_t>>, std::vector<std::vector<size_t>>>
{
	std::vector<std::vector<size_t>> a(H.cols()), b(H.rows());
	for (size_t i{1}; i < H.outerSize() + 1; ++i) {
		size_t inner_len{i == H.outerSize() ? H.nonZeros() : H.outerIndexPtr()[i]};
		for (size_t j{H.outerIndexPtr()[i - 1]}; j < inner_len; ++j) {
			b[i - 1].push_back(H.innerIndexPtr()[j]);
			a[H.innerIndexPtr()[j]].push_back(i - 1);
		}
	}

	return {a, b};
}


std::tuple<double, double, size_t> find_2_mins(std::vector<std::vector<size_t>> const& B, std::vector<std::vector<LLR>> const& L, size_t string_number)
{
	std::function<double&(double &, double &)> max =
	[](double & a, double & b) -> double&
	{
		return (b > a) ? b : a;
	};

	double min_1{L[string_number][B[string_number][0]].beta()}, min_2{L[string_number][B[string_number][0]].beta()};
	for (size_t index : B[string_number]) {
		if (L[string_number][index].beta() < std::max(min_1, min_2)) {
			max(min_1, min_2) = L[string_number][index].beta();
		}
	}
	std::tie(min_1, min_2) = std::minmax(min_1, min_2);

	// Finding position of min_1 with precision E
	double E{1e-6};
	size_t min_1_pos = -1;
	for (size_t index : B[string_number]) {
		if (abs(L[string_number][index].beta() - min_1) < E) {
			min_1_pos = index;
		}
	}

	if (min_1_pos == -1) {
		throw std::runtime_error{"Min 1 position not found"};
	}

	return {min_1, min_2, min_1_pos};
}


GF2 compute_overall_sign(std::vector<std::vector<size_t>> const& B, std::vector<std::vector<LLR>> const& L, size_t string_number)
{
	GF2 overall_sign{0};
	for (size_t index : B[string_number]) {
		overall_sign += L[string_number][index].alpha();
	}
	return overall_sign;
}


std::tuple<double, double, size_t> find_2_mins(Eigen::SparseMatrix<LLR, Eigen::RowMajor> const& L, size_t string_number)
{
	// std::vector<double> v;

	// for (Eigen::SparseMatrix<LLR, Eigen::RowMajor>::InnerIterator it{L, string_number}; it; ++it) {
	// 	v.push_back(it.value().beta());
	// }
	// std::vector<double>::iterator iter = std::find(v.begin(), v.end(), *std::min_element(v.begin(), v.end()));
	// size_t min_1_pos_nnz = std::distance(v.begin(), iter);
	// size_t min_1_pos = L.innerIndexPtr()[L.outerIndexPtr()[string_number] + min_1_pos_nnz];
	// std::sort(v.begin(), v.end());

	// return {v[0], v[1], min_1_pos};


	Eigen::SparseMatrix<LLR, Eigen::RowMajor>::InnerIterator first{L, string_number};
	double min_1{std::numeric_limits<double>::max()};
	double min_2{std::numeric_limits<double>::max()};
	size_t min_1_pos{first.col()};

	// Ищем первый минимум
	for (Eigen::SparseMatrix<LLR, Eigen::RowMajor>::InnerIterator it{L, string_number}; it; ++it) {
		if (it.value().beta() < min_1) {
			min_1 = it.value().beta();
			min_1_pos = it.col();
		}
	}

	// Ищем второй минимум
	for (Eigen::SparseMatrix<LLR, Eigen::RowMajor>::InnerIterator it{L, string_number}; it; ++it) {
		if (it.value().beta()  < min_2 && it.col() != min_1_pos) {
			min_2 = it.value().beta();
		}
	}

	return {min_1, min_2, min_1_pos};
}


GF2 compute_overall_sign(Eigen::SparseMatrix<LLR, Eigen::RowMajor> const& L, size_t string_number)
{
	GF2 overall_sign{0};

	for (Eigen::SparseMatrix<LLR, Eigen::RowMajor>::InnerIterator it{L, string_number}; it; ++it) {
		overall_sign += it.value().alpha();
	}

	return overall_sign;
}


std::tuple<double, double, size_t> find_2_mins(Eigen::Map<Eigen::SparseMatrix<LLR, Eigen::RowMajor>> const& L, size_t string_number)
{
	// std::vector<double> v;

	// for (Eigen::SparseMatrix<LLR, Eigen::RowMajor>::InnerIterator it{L, string_number}; it; ++it) {
	// 	v.push_back(it.value().beta());
	// }
	// std::vector<double>::iterator iter = std::find(v.begin(), v.end(), *std::min_element(v.begin(), v.end()));
	// size_t min_1_pos_nnz = std::distance(v.begin(), iter);
	// size_t min_1_pos = L.innerIndexPtr()[L.outerIndexPtr()[string_number] + min_1_pos_nnz];
	// std::sort(v.begin(), v.end());

	// return {v[0], v[1], min_1_pos};


	Eigen::Map<Eigen::SparseMatrix<LLR, Eigen::RowMajor>>::InnerIterator first{L, string_number};
	double min_1{std::numeric_limits<double>::max()};
	double min_2{std::numeric_limits<double>::max()};
	size_t min_1_pos{first.col()};

	// Ищем первый минимум
	for (Eigen::Map<Eigen::SparseMatrix<LLR, Eigen::RowMajor>>::InnerIterator it{L, string_number}; it; ++it) {
		if (it.value().beta() < min_1) {
			min_1 = it.value().beta();
			min_1_pos = it.col();
		}
	}

	// Ищем второй минимум
	for (Eigen::Map<Eigen::SparseMatrix<LLR, Eigen::RowMajor>>::InnerIterator it{L, string_number}; it; ++it) {
		if (it.value().beta()  < min_2 && it.col() != min_1_pos) {
			min_2 = it.value().beta();
		}
	}

	return {min_1, min_2, min_1_pos};
}


GF2 compute_overall_sign(Eigen::Map<Eigen::SparseMatrix<LLR, Eigen::RowMajor>> const& L, size_t string_number)
{
	GF2 overall_sign{0};

	for (Eigen::Map<Eigen::SparseMatrix<LLR, Eigen::RowMajor>>::InnerIterator it{L, string_number}; it; ++it) {
		overall_sign += it.value().alpha();
	}

	return overall_sign;
}


struct size_t_hash
{
	std::size_t operator()(const std::pair<size_t, size_t>& k) const
	{
		return std::hash<size_t>()(k.first) ^
			(std::hash<size_t>()(k.second) << 1);
	}
};
typedef std::unordered_map<std::pair<size_t, size_t>, LLR, size_t_hash> lights_sparse_m_t;


std::tuple<double, double, size_t> find_2_mins(std::vector<std::vector<size_t>> const& B, lights_sparse_m_t const& L, size_t string_number)
{
	std::function<double&(double &, double &)> max =
	[](double & a, double & b) -> double&
	{
		return (b > a) ? b : a;
	};

	double min_1{L.at({string_number, B[string_number][0]}).beta()}, min_2{L.at({string_number, B[string_number][0]}).beta()};
	for (size_t index : B[string_number]) {
		if (L.at({string_number, index}).beta() < std::max(min_1, min_2)) {
			max(min_1, min_2) = L.at({string_number, index}).beta();
		}
	}
	std::tie(min_1, min_2) = std::minmax(min_1, min_2);

	// Finding position of min_1 with precision E
	double E{1e-10};
	size_t min_1_pos = -1;
	for (size_t index : B[string_number]) {
		if (abs(L.at({string_number, index}).beta() - min_1) < E) {
			min_1_pos = index;
		}
	}

	if (min_1_pos == -1) {
		throw std::runtime_error{"Min 1 position not found"};
	}

	return {min_1, min_2, min_1_pos};
}


GF2 compute_overall_sign(std::vector<std::vector<size_t>> const& B, lights_sparse_m_t const& L, size_t string_number)
{
	GF2 overall_sign{0};
	for (size_t index : B[string_number]) {
		overall_sign += L.at({string_number, index}).alpha();
	}
	return overall_sign;
}


Eigen::VectorX<GF2> decode_sum_product(Eigen::SparseMatrix<GF2, Eigen::RowMajor> H, std::vector<double> const& R, size_t max_iters, bool verbose)
{
	size_t m{H.rows()};
	size_t n{H.cols()};

	auto [A, B] = make_ab(H);

	std::vector<std::vector<double>> M(m, std::vector<double>(n));
	for (size_t i{0}; i < n; ++i) {
		for (size_t j{0}; j < m; ++j) {
			M[j][i] = R[i];
		}
	}

	size_t I{0};
	std::vector<std::vector<double>> E(m, std::vector<double>(n));
	while(true) {

		if (verbose) { 
			std::cout << "Iteration " << I << ":\n";
		}

		for (size_t j{0}; j < m; ++j) {
			for (size_t i : B[j]) {
				double product{1.0};
				for (size_t i_ : B[j]) {
					if (i_ != i) {
						product *= tanh(M[j][i_] / 2.0);
					}
				}
				E[j][i] = log((1 + product)/(1 - product));

				if (verbose) {
					std::cout << "E[" << j << "][" << i << "] = " << E[j][i] << std::endl;
				}
			}
		}

		std::vector<double> L(n);
		Eigen::VectorX<GF2> c(n);
		for (size_t i{0}; i < n; ++i) {
			double E_sum{0};
			for (size_t j : A[i]) {
				E_sum += E[j][i];
			}
			L[i] = E_sum + R[i];
			c[i] = L[i] <= 0.0 ? 1 : 0; // Hard decision
		}

		if (verbose) {
			std::cout << "Hard decision: ";
			for (GF2 bit : c) {
				std::cout << bit << " ";
			}
			std::cout << std::endl;
		}

		if ((I == max_iters) || (H * c).isZero()) {
			return c;
		}

		for (size_t i{0}; i < n; ++i) {
			for (size_t j : A[i]) {
				double E_sum{0};
				for (size_t j_ : A[i]) {
					if (j_ != j) {
						E_sum += E[j_][i];
					}
				}
				M[j][i] = E_sum + R[i];

				if (verbose) {
					std::cout << "M[" << j << "][" << i << "] = " << M[j][i] << std::endl;
				}
			}
		}
		++I;
	}
}


Eigen::VectorX<GF2> decode_sum_product_opt(Eigen::SparseMatrix<GF2, Eigen::RowMajor> H, std::vector<LLR> const& R, size_t max_iters, bool verbose)
{
	size_t m{H.rows()};
	size_t n{H.cols()};

	auto [A, B] = make_ab(H);

	std::vector<std::vector<LLR>> M(m, std::vector<LLR>(n));
	for (size_t i{0}; i < n; ++i) {
		for (size_t j{0}; j < m; ++j) {
			M[j][i] = R[i];
		}
	}

	if (verbose) {		
		for (size_t j{0}; j < m; ++j) {
			for (size_t i{0}; i < n; ++i) {
				if (std::find(A[i].begin(), A[i].end(), j) == A[i].end()) {
					std::cout << "------- ";
				}
				else {
					std::cout << std::setw(7) << std::setprecision(5) << M[j][i] << " ";
				}
			}
			std::cout << std::endl;
		}
	}

	size_t I{0};
	std::vector<std::vector<LLR>> E(m, std::vector<LLR>(n));
	while(true) {

		if (verbose) {
			std::cout << "Iteration " << I << ":\n";
		}

		for (size_t j{0}; j < m; ++j) {
			for (size_t i : B[j]) {
				GF2 sign_product{0};
				double val_sum{0.0};
				for (size_t i_ : B[j]) {
					if (i_ != i) {
						val_sum += phi(M[j][i_].beta());
						sign_product += M[j][i_].alpha();
					}
				}
				E[j][i] = {sign_product, phi(val_sum)};

				// if (verbose) {
				// 	std::cout << "E[" << j << "][" << i << "] = " << E[j][i] << std::endl;
				// }
			}
		}

		if (verbose) {
			for (size_t j{0}; j < m; ++j) {
				for (size_t i{0}; i < n; ++i) {
					if (std::find(B[j].begin(), B[j].end(), i) == B[j].end()) {
						std::cout << "------- ";
					}
					else {
						std::cout << std::setw(7) << std::setprecision(5) << E[j][i] << " ";
					}
				}
				std::cout << std::endl;
			}
		}

		std::vector<LLR> L(n);
		Eigen::VectorX<GF2> c(n);
		for (size_t i{0}; i < n; ++i) {
			LLR E_sum{0.0};
			for (size_t j : A[i]) {
				E_sum += E[j][i];
			}
			L[i] = E_sum + R[i];
			c[i] = L[i].alpha(); // Hard decision
		}

		if (verbose) {
			std::cout << "Hard decision: ";
			for (GF2 bit : c) {
				std::cout << bit << " ";
			}
			std::cout << std::endl;
		}

		if ((I == max_iters) || (H * c).isZero()) {
			return c;
		}

		for (size_t i{0}; i < n; ++i) {
			for (size_t j : A[i]) {
				LLR E_sum{0.0};
				for (size_t j_ : A[i]) {
					if (j_ != j) {
						E_sum += E[j_][i];
					}
				}
				M[j][i] = E_sum + R[i];

				// if (verbose) {
				// 	std::cout << "M[" << j << "][" << i << "] = " << M[j][i] << std::endl;
				// }
			}
		}

		if (verbose) {
			
			for (size_t j{0}; j < m; ++j) {
				for (size_t i{0}; i < n; ++i) {
					if (std::find(A[i].begin(), A[i].end(), j) == A[i].end()) {
						std::cout << "------- ";
					}
					else {
						std::cout << std::setw(7) << std::setprecision(5) << M[j][i] << " ";
					}
				}
				std::cout << std::endl;
			}
		}

		++I;
	}
}


Eigen::VectorX<GF2> decode_normalized_min_sum(Eigen::SparseMatrix<GF2, Eigen::RowMajor> H, std::vector<LLR> const& R, double scale, size_t max_iters, bool verbose)
{
	size_t m{H.rows()};
	size_t n{H.cols()};

	auto [A, B] = make_ab(H);

	Eigen::SparseMatrix<LLR> M{m, n};
	for (size_t i{0}; i < n; ++n) {
		for (size_t j : A[i]) {
			M.insert(j, i) = R[i];
		}
	}

	if (verbose) {		
		for (size_t j{0}; j < m; ++j) {
			for (size_t i{0}; i < n; ++i) {
				if (std::find(A[i].begin(), A[i].end(), j) == A[i].end()) {
					std::cout << "------- ";
				}
				else {
					std::cout << std::setw(7) << std::setprecision(5) << M.coeff(j, i) << " ";
				}
			}
			std::cout << std::endl;
		}
	}

	size_t I{0};
	Eigen::SparseMatrix<LLR> E{m, n};
	while(true) {

		if (verbose) {
			std::cout << "Iteration " << I << ":\n";
		}

		for (size_t j{0}; j < m; ++j) {
			for (size_t i : B[j]) {
				GF2 sign_product{0};
				double min_val{M.coeff(j, B[j][0]).beta()};
				for (size_t i_ : B[j]) {
					if (i_ != i) {
						if (M.coeff(j, i_).beta() < min_val) {
							min_val = M.coeff(j, i_).beta();
						}
						sign_product += M.coeff(j, i_).alpha();
					}
				}
				E.insert(j, i) = {sign_product, min_val * scale};

				// if (verbose) {
				// 	std::cout << "E[" << j << "][" << i << "] = " << E[j][i] << std::endl;
				// }
			}
		}

		if (verbose) {
			for (size_t j{0}; j < m; ++j) {
				for (size_t i{0}; i < n; ++i) {
					if (std::find(B[j].begin(), B[j].end(), i) == B[j].end()) {
						std::cout << "------- ";
					}
					else {
						std::cout << std::setw(7) << std::setprecision(5) << E.coeff(j, i) << " ";
					}
				}
				std::cout << std::endl;
			}
		}

		std::vector<LLR> L(n);
		Eigen::VectorX<GF2> c(n);
		for (size_t i{0}; i < n; ++i) {
			LLR E_sum{0.0};
			for (size_t j : A[i]) {
				E_sum += E.coeff(j, i);
			}
			L[i] = E_sum + R[i];
			c[i] = L[i].alpha(); // Hard decision
		}

		if (verbose) {
			std::cout << "Hard decision: ";
			for (GF2 bit : c) {
				std::cout << bit << " ";
			}
			std::cout << std::endl;
		}

		if ((I == max_iters) || (H * c).isZero()) {
			return c;
		}

		for (size_t i{0}; i < n; ++i) {
			for (size_t j : A[i]) {
				LLR E_sum{0.0};
				for (size_t j_ : A[i]) {
					if (j_ != j) {
						E_sum += E.coeff(j_, i);
					}
				}
				M.insert(j, i) = E_sum + R[i];

				// if (verbose) {
				// 	std::cout << "M[" << j << "][" << i << "] = " << M[j][i] << std::endl;
				// }
			}
		}

		if (verbose) {
			
			for (size_t j{0}; j < m; ++j) {
				for (size_t i{0}; i < n; ++i) {
					if (std::find(A[i].begin(), A[i].end(), j) == A[i].end()) {
						std::cout << "------- ";
					}
					else {
						std::cout << std::setw(7) << std::setprecision(5) << M.coeff(j, i) << " ";
					}
				}
				std::cout << std::endl;
			}
		}

		++I;
	}
}


Eigen::VectorX<GF2> decode_normalized_min_sum_opt(Eigen::SparseMatrix<GF2, Eigen::RowMajor> H, std::vector<LLR> const& R, double scale, size_t max_iters, bool verbose)
{
	size_t m{H.rows()};
	size_t n{H.cols()};

	auto [A, B] = make_ab(H);

	Eigen::SparseMatrix<LLR> M{m, n};
	for (size_t i{0}; i < n; ++i) {
		for (size_t j : A[i]) {
			M.insert(j, i) = R[i];
		}
	}

	if (verbose) {		
		for (size_t j{0}; j < m; ++j) {
			for (size_t i{0}; i < n; ++i) {
				if (std::find(A[i].begin(), A[i].end(), j) == A[i].end()) {
					std::cout << "------- ";
				}
				else {
					std::cout << std::setw(7) << std::setprecision(5) << M.coeff(j, i) << " ";
				}
			}
			std::cout << std::endl;
		}
	}

	size_t I{0};
	Eigen::SparseMatrix<LLR> E{m, n};
	while(true) {

		if (verbose) {
			std::cout << "Iteration " << I << ":\n";
		}

		for (size_t j{0}; j < m; ++j) {

			auto [min_1, min_2, min_1_pos] = find_2_mins(M, j);
			GF2 overall_sign{compute_overall_sign(M, j)};

			for (size_t i : B[j]) {

				E.insert(j, i) = (i == min_1_pos) ? LLR{M.coeff(j, i).alpha() + overall_sign, min_2 * scale} : LLR{M.coeff(j, i).alpha() + overall_sign, min_1 * scale};

				// if (verbose) {
				// 	std::cout << "E[" << j << "][" << i << "] = " << E[j][i] << std::endl;
				// }
			}
		}

		if (verbose) {
			for (size_t j{0}; j < m; ++j) {
				for (size_t i{0}; i < n; ++i) {
					if (std::find(B[j].begin(), B[j].end(), i) == B[j].end()) {
						std::cout << "------- ";
					}
					else {
						std::cout << std::setw(7) << std::setprecision(5) << E.coeff(j, i) << " ";
					}
				}
				std::cout << std::endl;
			}
		}

		std::vector<LLR> L(n);
		Eigen::VectorX<GF2> c(n);
		for (size_t i{0}; i < n; ++i) {
			LLR E_sum{0.0};
			for (size_t j : A[i]) {
				E_sum += E.coeff(j, i);
			}
			L[i] = E_sum + R[i];
			c[i] = L[i].alpha(); // Hard decision
		}

		if (verbose) {
			std::cout << "Hard decision: ";
			for (GF2 bit : c) {
				std::cout << bit << " ";
			}
			std::cout << std::endl;
		}

		if ((I == max_iters) || (H * c).isZero()) {
			return c;
		}

		for (size_t i{0}; i < n; ++i) {
			for (size_t j : A[i]) {
				LLR E_sum{0.0};
				for (size_t j_ : A[i]) {
					if (j_ != j) {
						E_sum += E.coeff(j_, i);
					}
				}
				M.insert(j, i) = E_sum + R[i];

				// if (verbose) {
				// 	std::cout << "M[" << j << "][" << i << "] = " << M[j][i] << std::endl;
				// }
			}
		}

		if (verbose) {
			
			for (size_t j{0}; j < m; ++j) {
				for (size_t i{0}; i < n; ++i) {
					if (std::find(A[i].begin(), A[i].end(), j) == A[i].end()) {
						std::cout << "------- ";
					}
					else {
						std::cout << std::setw(7) << std::setprecision(5) << M.coeff(j, i) << " ";
					}
				}
				std::cout << std::endl;
			}
		}

		++I;
	}
}


Eigen::VectorX<GF2> decode_layered_normalized_min_sum(Eigen::SparseMatrix<GF2, Eigen::RowMajor> H, std::vector<LLR> const& R, size_t layer_size, double scale, size_t max_iters, bool verbose)
{
	size_t m{H.rows()};
	size_t n{H.cols()};

	if (m % layer_size) {
		throw std::runtime_error{"Layer size incompatible"};
	}
	size_t layers_number{m / layer_size};

	auto [A, B] = make_ab(H);

	std::vector<std::vector<LLR>> L(m, std::vector<LLR>(n)); // Naming according to Anrew Thangaraj lecture (https://youtu.be/CNjdkQOAqhE?list=PLyqSpQzTE6M81HJ26ZaNv0V3ROBrcv-Kc)
	for (size_t i{0}; i < n; ++i) {
		for (size_t j{0}; j < m; ++j) {
			L[j][i] = 0;
		}
	}

	std::vector<LLR> r{R}; // Belief. To be changed in every layer.

	if (verbose) {
		std::cout << "Init r values:\n";
		for (LLR val : r) {
			std::cout << val << " ";
		}
		std::cout << std::endl;
	}

	size_t I{0};
	std::vector<std::vector<LLR>> E(layers_number, std::vector<LLR>{});
	while(true) {

		if (verbose) {
			std::cout << "Iteration " << I << ":\n";
		}

		for (size_t layer_counter{0}; layer_counter < layers_number; ++layer_counter) {
			if (verbose) {
				std::cout << "Layer " << layer_counter << ", strings from " << layer_counter * layer_size << " to " << (layer_counter + 1) * layer_size - 1 << ":\n";
			}

			// Subtraction
			for (size_t j{layer_counter * layer_size}; j < ((layer_counter + 1) * layer_size); ++j) {
				for (size_t index : B[j]) {
					r[index] -= L[j][index];
				}
			}
			if (verbose) {
				std::cout << "r values after subtraction:\n";
				for (LLR val : r) {
					std::cout << val << " ";
				}
				std::cout << std::endl;
			}

			// Initialization
			for (size_t j{layer_counter * layer_size}; j < ((layer_counter + 1) * layer_size); ++j) {
				for (size_t index : B[j]) {
					L[j][index] = r[index];
				}
			}
			if (verbose) {
				std::cout << "L values after initialization:\n";
				for (size_t j{layer_counter * layer_size}; j < ((layer_counter + 1) * layer_size); ++j) {
					for (size_t index{0}; index < n; ++index) {
						if (std::find(B[j].begin(), B[j].end(), index) != B[j].end()) {
							std::cout << L[j][index] << "\t";
						}
						else {
							std::cout << "-----\t";
						}
					}
					std::cout << std::endl;
				}
			}
			
			// Min
			for (size_t j{layer_counter * layer_size}; j < ((layer_counter + 1) * layer_size); ++j) {
				auto [min_1, min_2, min_1_pos] = find_2_mins(B, L, j);
				GF2 overall_sign{compute_overall_sign(B, L, j)};

				for (size_t index : B[j]) {
					L[j][index] = (index == min_1_pos) ? LLR{L[j][index].alpha(), min_2} : LLR{L[j][index].alpha(), min_1};
					L[j][index] = {L[j][index].alpha() + overall_sign, L[j][index].beta() * scale};
				}
			}
			if (verbose) {
				std::cout << "L values after min:\n";
				for (size_t j{layer_counter * layer_size}; j < ((layer_counter + 1) * layer_size); ++j) {
					for (size_t index{0}; index < n; ++index) {
						if (std::find(B[j].begin(), B[j].end(), index) != B[j].end()) {
							std::cout << L[j][index] << "\t";
						}
						else {
							std::cout << "-----\t";
						}
					}
					std::cout << std::endl;
				}
			}

			// Sum
			for (size_t j{layer_counter * layer_size}; j < ((layer_counter + 1) * layer_size); ++j) {
				for (size_t index : B[j]) {
					r[index] += L[j][index];
				}
			}
			if (verbose) {
				std::cout << "r values after sum:\n";
				for (LLR val : r) {
					std::cout << val << " ";
				}
				std::cout << std::endl;
			}

			// Check
			Eigen::VectorX<GF2> c(n);
			for (size_t i{0}; i < n; ++i) {
				c[i] = r[i].alpha(); // Hard decision
			}

			if (verbose) {
				std::cout << "Hard decision: ";
				for (GF2 bit : c) {
					std::cout << bit << " ";
				}
				std::cout << std::endl;
			}

			if ((I == max_iters) || (H * c).isZero()) {
				return c;
			}
		}
		++I;
	}
}


Eigen::VectorX<GF2> decode_sp_to_syndrome(Eigen::SparseMatrix<GF2, Eigen::RowMajor> H, std::vector<LLR> const& R, Eigen::VectorX<GF2> const& s, size_t max_iters, bool verbose)
{
	size_t m{H.rows()};
	size_t n{H.cols()};

	auto [A, B] = make_ab(H);

	std::vector<std::vector<LLR>> M(m, std::vector<LLR>(n));
	for (size_t i{0}; i < n; ++i) {
		for (size_t j{0}; j < m; ++j) {
			M[j][i] = R[i];
		}
	}

	size_t I{0};
	std::vector<std::vector<LLR>> E(m, std::vector<LLR>(n));
	while(true) {

		if (verbose) {
			std::cout << "Iteration " << I << ":\n";
		}

		for (size_t j{0}; j < m; ++j) {
			for (size_t i : B[j]) {
				GF2 sign_product{0};
				double val_sum{0.0};
				for (size_t i_ : B[j]) {
					if (i_ != i) {
						val_sum += phi(M[j][i_].beta());
						sign_product += M[j][i_].alpha();
					}
				}
				sign_product += s[j];
				E[j][i] = {sign_product, phi(val_sum)};

				if (verbose) {
					std::cout << "E[" << j << "][" << i << "] = " << E[j][i] << std::endl;
				}
			}
		}

		std::vector<LLR> L(n);
		Eigen::VectorX<GF2> c(n);
		for (size_t i{0}; i < n; ++i) {
			LLR E_sum{0.0};
			for (size_t j : A[i]) {
				E_sum += E[j][i];
			}
			L[i] = E_sum + R[i];
			c[i] = L[i].alpha(); // Hard decision
		}

		if (verbose) {
			std::cout << "Hard decision: ";
			for (GF2 bit : c) {
				std::cout << bit << " ";
			}
			std::cout << std::endl;
		}

		if ((I == max_iters) || (H * c) == s) {
			return c;
		}

		for (size_t i{0}; i < n; ++i) {
			for (size_t j : A[i]) {
				LLR E_sum{0.0};
				for (size_t j_ : A[i]) {
					if (j_ != j) {
						E_sum += E[j_][i];
					}
				}
				M[j][i] = E_sum + R[i];

				if (verbose) {
					std::cout << "M[" << j << "][" << i << "] = " << M[j][i] << std::endl;
				}
			}
		}
		++I;
	}
}


Eigen::VectorX<GF2> decode_nms_to_syndrome(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& H, std::vector<LLR> const& R, Eigen::VectorX<GF2> const& s, double scale, size_t max_iters, bool verbose)
{
	size_t m{H.rows()};
	size_t n{H.cols()};

	auto [A, B] = make_ab(H);

	lights_sparse_m_t M{m};
	for (size_t i{0}; i < n; ++i) {
		for (size_t j : A[i]) {
			M[{j, i}] = R[i];
		}
	}

	if (verbose) {	
		std::cout << "Init M values:\n";	
		for (size_t j{0}; j < m; ++j) {
			for (size_t i{0}; i < n; ++i) {
				if (std::find(A[i].begin(), A[i].end(), j) == A[i].end()) {
					std::cout << "------- ";
				}
				else {
					std::cout << std::setw(7) << std::setprecision(5) << M[{j, i}] << " ";
				}
			}
			std::cout << std::endl;
		}
	}

	size_t I{0};
	lights_sparse_m_t E{m};
	while(true) {

		if (verbose) {
			std::cout << "Iteration " << I << ":\n";
		}

		for (size_t j{0}; j < m; ++j) {
			for (size_t i : B[j]) {
				GF2 sign_product{0};
				double min_val{M[{j, B[j][0]}].beta()};
				for (size_t i_ : B[j]) {
					if (i_ != i) {
						if (M[{j, i_}].beta() < min_val) {
							min_val = M[{j, i_}].beta();
						}
						sign_product += M[{j, i_}].alpha();
					}
				}
				sign_product += s[j];
				E[{j, i}] = {sign_product, min_val * scale};
			}
		}

		if (verbose) {
			std::cout << "E values:\n";
			for (size_t j{0}; j < m; ++j) {
				for (size_t i{0}; i < n; ++i) {
					if (std::find(B[j].begin(), B[j].end(), i) == B[j].end()) {
						std::cout << "------- ";
					}
					else {
						std::cout << std::setw(7) << std::setprecision(5) << E[{j, i}] << " ";
					}
				}
				std::cout << std::endl;
			}
		}

		std::vector<LLR> L(n);
		Eigen::VectorX<GF2> c(n);
		for (size_t i{0}; i < n; ++i) {
			LLR E_sum{0.0};
			for (size_t j : A[i]) {
				E_sum += E[{j, i}];
			}
			L[i] = E_sum + R[i];
			c[i] = L[i].alpha(); // Hard decision
		}

		if (verbose) {
			std::cout << "Hard decision: ";
			for (GF2 bit : c) {
				std::cout << bit << " ";
			}
			std::cout << std::endl;
		}

		if ((I == max_iters) || (H * c) == s) {
			return c;
		}

		for (size_t i{0}; i < n; ++i) {
			for (size_t j : A[i]) {
				LLR E_sum{0.0};
				for (size_t j_ : A[i]) {
					if (j_ != j) {
						E_sum += E[{j_, i}];
					}
				}
				M[{j, i}] = E_sum + R[i];
			}
		}

		if (verbose) {
			std::cout << "M values:\n";
			for (size_t j{0}; j < m; ++j) {
				for (size_t i{0}; i < n; ++i) {
					if (std::find(A[i].begin(), A[i].end(), j) == A[i].end()) {
						std::cout << "------- ";
					}
					else {
						std::cout << std::setw(7) << std::setprecision(5) << M[{j, i}] << " ";
					}
				}
				std::cout << std::endl;
			}
		}

		++I;
	}
}


Eigen::VectorX<GF2> decode_nms_to_syndrome_r(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& H, std::vector<LLR> const& R, Eigen::VectorX<GF2> const& s, MemoryManager const& mm, size_t thread_index, double scale, size_t max_iters, bool verbose)
{
	size_t m{H.rows()};
	size_t n{H.cols()};

	std::vector<std::vector<size_t>> A, B;

	if (verbose) { // A and B are needed for debugging only
		std::tie(A, B) = make_ab(H);
	}

	LLR * M_data = mm.get_Ms()[thread_index];
	Eigen::Map<Eigen::SparseMatrix<LLR, Eigen::RowMajor>> M{m, n, mm.get_non_zeros(), mm.get_outer_index_ptr(), mm.get_inner_index_ptr(), M_data};
	for (size_t j{0}; j < m; ++j) {
		for (llr_spmmap_in_it M_iter{M, j}; M_iter; ++M_iter) {
			M_iter.valueRef() = R[M_iter.col()];
		}
	}

	if (verbose) {	
		std::cout << "Init M values:\n";	
		for (size_t j{0}; j < m; ++j) {
			for (size_t i{0}; i < n; ++i) {
				if (std::find(A[i].begin(), A[i].end(), j) == A[i].end()) {
					std::cout << "------- ";
				}
				else {
					std::cout << std::setw(7) << std::setprecision(5) << M.coeff(j, i) << " ";
				}
			}
			std::cout << std::endl;
		}
	}

	size_t I{0};

	LLR * E_data = mm.get_Es()[thread_index];
	Eigen::Map<Eigen::SparseMatrix<LLR, Eigen::RowMajor>> E{m, n, mm.get_non_zeros(), mm.get_outer_index_ptr(), mm.get_inner_index_ptr(), E_data};

	while(true) {

		if (verbose) {
			std::cout << "Iteration " << I << ":\n";
		}

		for (size_t j{0}; j < m; ++j) {

			auto [min_1, min_2, min_1_pos] = find_2_mins(M, j);
			GF2 overall_sign{compute_overall_sign(M, j)};

			for (llr_spmmap_in_it M_iter{M, j}, E_iter{E, j}; M_iter && E_iter; ++M_iter, ++E_iter) {
				LLR val = (M_iter.col() == min_1_pos) ? LLR{M_iter.value().alpha() + overall_sign + s[j], min_2 * scale} : LLR{M_iter.value().alpha() + overall_sign + s[j], min_1 * scale};
				E_iter.valueRef() = val;
			}
		}

		if (verbose) {
			std::cout << "E values:\n";
			for (size_t j{0}; j < m; ++j) {
				for (size_t i{0}; i < n; ++i) {
					if (std::find(B[j].begin(), B[j].end(), i) == B[j].end()) {
						std::cout << "------- ";
					}
					else {
						std::cout << std::setw(7) << std::setprecision(5) << E.coeff(j, i) << " ";
					}
				}
				std::cout << std::endl;
			}
		}

		std::vector<LLR> L(n, 0.0);
		Eigen::VectorX<GF2> c(n);

		for (size_t j{0}; j < m; ++j) {
			for (llr_spmmap_in_it E_iter{E, j}; E_iter; ++E_iter) {
				L[E_iter.col()] += E_iter.value();
			}
		}
		std::transform(L.begin(), L.end(), R.begin(), L.begin(), std::plus<LLR>()); // Element-wise addition R to L
		std::transform(L.begin(), L.end(), c.begin(), [](LLR const& llr) { return llr.alpha(); }); // Copy signs of L to c

		if (verbose) {
			std::cout << "Hard decision: ";
			for (GF2 bit : c) {
				std::cout << bit << " ";
			}
			std::cout << std::endl;
		}

		if ((I == max_iters) || (H * c) == s) {
			return c;
		}

		for (size_t j{0}; j < m; ++j) {
			for (llr_spmmap_in_it M_iter{M, j}, E_iter{E, j}; M_iter && E_iter; ++M_iter, ++E_iter) {
				M_iter.valueRef() = L[M_iter.col()] - E_iter.value(); // Sum of M_iter.col()-th column without value on j-th row
			}
		}

		if (verbose) {
			std::cout << "M values:\n";
			for (size_t j{0}; j < m; ++j) {
				for (size_t i{0}; i < n; ++i) {
					if (std::find(A[i].begin(), A[i].end(), j) == A[i].end()) {
						std::cout << "------- ";
					}
					else {
						std::cout << std::setw(7) << std::setprecision(5) << M.coeff(j, i) << " ";
					}
				}
				std::cout << std::endl;
			}
		}

		++I;
	}
}


Eigen::VectorX<GF2> decode_nms_to_syndrome_opt(Eigen::SparseMatrix<GF2, Eigen::RowMajor> H, std::vector<LLR> const& R, Eigen::VectorX<GF2> const& s, double scale, size_t max_iters, bool verbose)
{
	size_t m{H.rows()};
	size_t n{H.cols()};

	auto [A, B] = make_ab(H);

	Eigen::SparseMatrix<LLR, Eigen::RowMajor> M{m, n};
	for (size_t i{0}; i < n; ++i) {
		for (size_t j : A[i]) {
			M.coeffRef(j, i) = R[i];
		}
	}
	M.makeCompressed();

	if (verbose) {	
		std::cout << "Init M values:\n";	
		for (size_t j{0}; j < m; ++j) {
			for (size_t i{0}; i < n; ++i) {
				if (std::find(A[i].begin(), A[i].end(), j) == A[i].end()) {
					std::cout << "------- ";
				}
				else {
					std::cout << std::setw(7) << std::setprecision(5) << M.coeff(j, i) << " ";
				}
			}
			std::cout << std::endl;
		}
	}

	size_t I{0};
	Eigen::SparseMatrix<LLR, Eigen::RowMajor> E{m, n};
	while(true) {

		if (verbose) {
			std::cout << "Iteration " << I << ":\n";
		}

		for (size_t j{0}; j < m; ++j) {

			auto [min_1, min_2, min_1_pos] = find_2_mins(M, j);
			GF2 overall_sign{compute_overall_sign(M, j)};

			for (size_t i : B[j]) {
				LLR val = (i == min_1_pos) ? LLR{M.coeff(j, i).alpha() + overall_sign + s[j], min_2 * scale} : LLR{M.coeff(j, i).alpha() + overall_sign + s[j], min_1 * scale};
				E.coeffRef(j, i) = val;
			}
		}

		if (verbose) {
			std::cout << "E values:\n";
			for (size_t j{0}; j < m; ++j) {
				for (size_t i{0}; i < n; ++i) {
					if (std::find(B[j].begin(), B[j].end(), i) == B[j].end()) {
						std::cout << "------- ";
					}
					else {
						std::cout << std::setw(7) << std::setprecision(5) << E.coeff(j, i) << " ";
					}
				}
				std::cout << std::endl;
			}
		}

		std::vector<LLR> L(n);
		Eigen::VectorX<GF2> c(n);
		for (size_t i{0}; i < n; ++i) {
			LLR E_sum{0.0};
			for (size_t j : A[i]) {
				E_sum += E.coeff(j, i);
			}
			L[i] = E_sum + R[i];
			c[i] = L[i].alpha(); // Hard decision
		}

		if (verbose) {
			std::cout << "Hard decision: ";
			for (GF2 bit : c) {
				std::cout << bit << " ";
			}
			std::cout << std::endl;
		}

		if ((I == max_iters) || (H * c) == s) {
			return c;
		}

		for (size_t i{0}; i < n; ++i) {
			for (size_t j : A[i]) {
				LLR E_sum{0.0};
				for (size_t j_ : A[i]) {
					if (j_ != j) {
						E_sum += E.coeff(j_, i);
					}
				}
				M.coeffRef(j, i) = E_sum + R[i];
			}
		}

		if (verbose) {
			std::cout << "M values:\n";
			for (size_t j{0}; j < m; ++j) {
				for (size_t i{0}; i < n; ++i) {
					if (std::find(A[i].begin(), A[i].end(), j) == A[i].end()) {
						std::cout << "------- ";
					}
					else {
						std::cout << std::setw(7) << std::setprecision(5) << M.coeff(j, i) << " ";
					}
				}
				std::cout << std::endl;
			}
		}

		++I;
	}
}


Eigen::VectorX<GF2> decode_lnms_to_syndrome(Eigen::SparseMatrix<GF2, Eigen::RowMajor> H, std::vector<LLR> const& R, Eigen::VectorX<GF2> const& s, size_t layer_size, double scale, size_t max_iters, bool verbose)
{
	size_t m{H.rows()};
	size_t n{H.cols()};

	if (m % layer_size) {
		throw std::runtime_error{"Layer size incompatible"};
	}
	size_t layers_number{m / layer_size};

	auto [A, B] = make_ab(H);

	std::vector<std::vector<LLR>> L(m, std::vector<LLR>(n)); // Naming according to Anrew Thangaraj lecture (https://youtu.be/CNjdkQOAqhE?list=PLyqSpQzTE6M81HJ26ZaNv0V3ROBrcv-Kc)
	for (size_t i{0}; i < n; ++i) {
		for (size_t j{0}; j < m; ++j) {
			L[j][i] = 0;
		}
	}

	std::vector<LLR> r{R}; // Belief. To be changed in every layer.

	if (verbose) {
		std::cout << "Init r values:\n";
		for (LLR val : r) {
			std::cout << val << " ";
		}
		std::cout << std::endl;
	}

	size_t I{0};
	std::vector<std::vector<LLR>> E(layers_number, std::vector<LLR>{});
	while(true) {

		if (verbose) {
			std::cout << "Iteration " << I << ":\n";
		}

		for (size_t layer_counter{0}; layer_counter < layers_number; ++layer_counter) {
			if (verbose) {
				std::cout << "Layer " << layer_counter << ", strings from " << layer_counter * layer_size << " to " << (layer_counter + 1) * layer_size - 1 << ":\n";
			}

			// Subtraction
			for (size_t j{layer_counter * layer_size}; j < ((layer_counter + 1) * layer_size); ++j) {
				for (size_t index : B[j]) {
					r[index] -= L[j][index];
				}
			}
			if (verbose) {
				std::cout << "r values after subtraction:\n";
				for (LLR val : r) {
					std::cout << val << " ";
				}
				std::cout << std::endl;
			}

			// Initialization
			for (size_t j{layer_counter * layer_size}; j < ((layer_counter + 1) * layer_size); ++j) {
				for (size_t index : B[j]) {
					L[j][index] = r[index];
				}
			}
			if (verbose) {
				std::cout << "L values after initialization:\n";
				for (size_t j{layer_counter * layer_size}; j < ((layer_counter + 1) * layer_size); ++j) {
					for (size_t index{0}; index < n; ++index) {
						if (std::find(B[j].begin(), B[j].end(), index) != B[j].end()) {
							std::cout << L[j][index] << "\t";
						}
						else {
							std::cout << "-----\t";
						}
					}
					std::cout << std::endl;
				}
			}
			
			// Min
			for (size_t j{layer_counter * layer_size}; j < ((layer_counter + 1) * layer_size); ++j) {
				auto [min_1, min_2, min_1_pos] = find_2_mins(B, L, j);
				GF2 overall_sign{compute_overall_sign(B, L, j)};

				for (size_t index : B[j]) {
					L[j][index] = (index == min_1_pos) ? LLR{L[j][index].alpha(), min_2} : LLR{L[j][index].alpha(), min_1};
					L[j][index] = {L[j][index].alpha() + overall_sign + s[j], L[j][index].beta() * scale};
				}
			}
			if (verbose) {
				std::cout << "L values after min:\n";
				for (size_t j{layer_counter * layer_size}; j < ((layer_counter + 1) * layer_size); ++j) {
					for (size_t index{0}; index < n; ++index) {
						if (std::find(B[j].begin(), B[j].end(), index) != B[j].end()) {
							std::cout << L[j][index] << "\t";
						}
						else {
							std::cout << "-----\t";
						}
					}
					std::cout << std::endl;
				}
			}

			// Sum
			for (size_t j{layer_counter * layer_size}; j < ((layer_counter + 1) * layer_size); ++j) {
				for (size_t index : B[j]) {
					r[index] += L[j][index];
				}
			}
			if (verbose) {
				std::cout << "r values after sum:\n";
				for (LLR val : r) {
					std::cout << val << " ";
				}
				std::cout << std::endl;
			}

			// Check
			Eigen::VectorX<GF2> c(n);
			for (size_t i{0}; i < n; ++i) {
				c[i] = r[i].alpha(); // Hard decision
			}

			if (verbose) {
				std::cout << "Hard decision: ";
				for (GF2 bit : c) {
					std::cout << bit << " ";
				}
				std::cout << std::endl;
			}

			if ((I == max_iters) || (H * c) == s) {
				return c;
			}
		}

		++I;
	}
}