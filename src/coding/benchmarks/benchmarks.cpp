#include "benchmarks.h"
#include "file-processor.h"
#include "efrinv.hpp"

#include <random>
#include <chrono>
#include <thread>
#include <mutex>
#include <iostream>
#include <iomanip>

#include <taskflow/taskflow.hpp>


std::pair<double, double> compute_mean_and_std(std::vector<double> const& values)
{
	double mean{std::accumulate(values.begin(), values.end(), 0.0) / values.size()};
	double sq_sum{0.};
	for (double val : values) {
		sq_sum += pow(mean - val, 2);
	}
	double std_dev{sqrt(sq_sum / values.size())};

	return {mean, std_dev};
}


template <typename seq_t>
void print_seq(seq_t const& seq){
	for (auto const& elem : seq) {
		std::cout << elem << "\t";
	}
	std::cout << std::endl;
}


std::pair<double, double> compute_bernoulli_p_confidence_interval(const std::vector<double> &values) {
    auto n = double(values.size());
    double w{std::accumulate(values.begin(), values.end(), 0.0) / n};
    double shift = sqrt(w * (1 - w) / n) * benchmarks::laplace_z_value_confidence90;
    return {w - shift, w + shift};
}

double estimate_ber_by_exposed(Eigen::Vector<GF2, Eigen::Dynamic> const &codeword, Eigen::Vector<double, Eigen::Dynamic> const &received_data, double exposed_bits_rate) {
    size_t seed{std::chrono::steady_clock::now().time_since_epoch().count()};
    std::mt19937 random_engine;
    random_engine.seed(seed);

    std::stringstream debug_msg;
    debug_msg<<std::endl<<std::endl;

    size_t word_size = codeword.size();
    int exposed_count = word_size * exposed_bits_rate;
    std::vector<size_t> exposed_indexes;
    exposed_indexes.resize(word_size);
    for (size_t i = 0; i < word_size; ++i) {
        exposed_indexes[i] = i;
    }

    debug_msg << "exposed_indexes (before): ";
    for (int i=0;i<exposed_indexes.size();++i){
        debug_msg<<std::to_string(exposed_indexes[i])<<" ";
    }
    debug_msg << std::endl;

    debug_msg << "word_size: "<< std::to_string(word_size) << std::endl;
    debug_msg << "exposed_count: "<< std::to_string(exposed_count) << std::endl;

    for (size_t i = 0; i < exposed_count; ++i) {
        std::uniform_int_distribution<> index_distribution{static_cast<int>(i + 1),
                                                           static_cast<int>(word_size - 1)};
        std::swap(exposed_indexes[i], exposed_indexes[index_distribution(random_engine)]);
    }

    debug_msg << "codeword:      ";
    for (int i=0;i<codeword.size();++i){
        debug_msg<<std::to_string(int (codeword[i]))<<" ";
    }
    debug_msg << std::endl;

    debug_msg << "received_data: ";
    for (int i=0;i<received_data.size();++i){
        debug_msg<<std::to_string(int (received_data[i]))<<" ";
    }
    debug_msg << std::endl;

    exposed_indexes.resize(exposed_count);

    debug_msg << "exposed_indexes: ";
    for (int i=0;i<exposed_indexes.size();++i){
        debug_msg<<std::to_string(exposed_indexes[i])<<" ";
    }
    debug_msg << std::endl;

    std::vector<double> error_sample;
    error_sample.resize(exposed_count);
    for (size_t i = 0; i < exposed_count; ++i) {
        size_t target_index = exposed_indexes[i];
        if (double(codeword[target_index]) != received_data[target_index]) {
            error_sample[i] = 1.0;
        } else {
            error_sample[i] = 0.0;
        }
    }

    debug_msg << "error_sample: ";
    for (int i=0;i<error_sample.size();++i){
        debug_msg<<std::to_string(error_sample[i])<<" ";
    }
    debug_msg << std::endl;

    auto [min_p, max_p] = compute_bernoulli_p_confidence_interval(error_sample);
    debug_msg << "p [min_p, max_p]: " <<std::to_string((min_p + max_p) / 2)<<" ["<<std::to_string(min_p)<<", "<<std::to_string(max_p)<<"]"<< std::endl;
//    std::cout<<debug_msg.str();
    return (min_p + max_p) / 2;
}


namespace benchmarks
{

BaseBenchmark::BaseBenchmark(std::string const& H_name, BG_type bg_type, size_t bg_rows, size_t bg_cols, size_t Z) : m_Z{Z}
{
	m_H = load_matrix_from_alist(H_name);
	size_t m = m_H.rows();
	size_t n = m_H.cols();

	if (bg_type != BG_type::NOT_5G) {
		Eigen::SparseMatrix<GF2, Eigen::RowMajor> new_H{m_H.block(0, 0, bg_rows, bg_cols)};
		new_H = enhance_from_base(new_H, Z);
		new_H = shift_eyes(new_H, Z, bg_type);
		m_H = new_H;
	}
}


BaseBenchmark::BaseBenchmark(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& H) : m_H{H} {}


auto BaseBenchmark::gen_rand_bit_seq(size_t len) -> Eigen::Vector<GF2, Eigen::Dynamic> const
{
	Eigen::Vector<GF2, Eigen::Dynamic> result(len);
	std::uniform_int_distribution<> bitDistribution{0, 1};

	#ifdef DEBUG
	size_t seed{std::chrono::steady_clock::now().time_since_epoch().count()};
	#else
	size_t seed{std::chrono::steady_clock::now().time_since_epoch().count()};
	// size_t seed{0};
	#endif

	std::mt19937 random_engine;
	random_engine.seed(seed);

	for (GF2& bit : result) {
		bit = bitDistribution(random_engine);
	}

	return result;
}


// auto BaseBenchmark::run(double ber_start, double ber_stop, double ber_step, LDPC_algo alg_type, bool verbose) -> RunningResult const
// {
// 	size_t constexpr STAT_ITERATIONS{30};
// 	size_t constexpr MAX_FAILURES{50};
// 	size_t constexpr MAX_DECODINGS{1000};
// 	size_t constexpr SUB_THREADS_NUMBER{3};

// 	std::vector<double> fers;
// 	std::vector<double> bers;

// 	size_t interval_number{0};

// 	for (double current_ber{ber_start}; current_ber < ber_stop; current_ber += ber_step) {
// 		double fer_sum_for_ber{0.0};

// 		// #ifdef NDEBUG
// 		std::mutex fer_sum_sync;
// 		std::vector<std::thread> threads;
// 		// #endif
// 		for (size_t stat_iter{0}; stat_iter < STAT_ITERATIONS; ++stat_iter) {
// 			// #ifdef NDEBUG
// 			std::thread t([=, &fer_sum_for_ber, &fer_sum_sync]() {
// 			// #endif
// 				std::atomic_size_t failures{0}; // No need to synchronize
// 				std::atomic_size_t total_iters{0}; // No need to synchronize
// 				while (failures < MAX_FAILURES && total_iters < MAX_DECODINGS) {
// 					// #ifdef NDEBUG
// 					std::vector<std::thread> sub_threads;
// 					for (size_t i{0}; i < SUB_THREADS_NUMBER; ++i) {
// 						sub_threads.emplace_back([&]() {
// 					// #endif
// 							if (!perform_error_correction(current_ber, alg_type)) {
// 								++failures;
// 							}
// 							++total_iters;
// 					// #ifdef NDEBUG
// 						});
// 					}
// 					for (std::thread& sub_th : sub_threads) {
// 						sub_th.join();
// 					}
// 					// #endif
// 				}
// 				// #ifdef NDEBUG
// 				fer_sum_sync.lock();
// 				// #endif
// 				fer_sum_for_ber += static_cast<double>(failures) / static_cast<double>(total_iters);
// 			// #ifdef NDEBUG
// 				fer_sum_sync.unlock();
// 			});
// 			threads.push_back(std::move(t));
// 			// #endif
// 		}
// 		// #ifdef NDEBUG
// 		for (std::thread& t : threads) {
// 			t.join();
// 		}
// 		// #endif

// 		double fer_av_for_epsilon{fer_sum_for_ber / static_cast<double>(STAT_ITERATIONS)};

// 		bers.push_back(current_ber);
// 		fers.push_back(fer_av_for_epsilon);

// 		if (verbose) {
// 			std::cout << "Interval " << interval_number + 1 << "/" << round((ber_stop - ber_start) / ber_step) + 1;
// 			std::cout << ": ber = " << std::setw(4) << current_ber << ",\tfer = " << fer_av_for_epsilon << std::endl;
// 		}

// 		++interval_number;
// 	}

// 	return {bers, fers};
// }


auto BaseBenchmark::compute_one_point(double ber, LDPC_algo alg_type, MemoryManager const& mm, size_t const STAT_ITERATIONS, bool verbose) -> std::pair<double, double>
{
	size_t constexpr MAX_FAILURES{1000}; // 100
	size_t constexpr MAX_DECODINGS{100000}; // 10000

	std::vector<double> fers_for_ber;
	fers_for_ber.reserve(STAT_ITERATIONS);

	#ifdef NDEBUG
	tf::Executor executor;
	tf::Taskflow taskflow;
	std::mutex fer_sum_sync;
	#endif
	for (size_t stat_iter{0}; stat_iter < STAT_ITERATIONS; ++stat_iter) {
		#ifdef NDEBUG
		taskflow.emplace([=, &mm, &fers_for_ber, &fer_sum_sync]() {
		#endif
			std::atomic_size_t failures{0}; // No need to synchronize
			std::atomic_size_t total_iters{0}; // No need to synchronize
			while (failures < MAX_FAILURES && total_iters < MAX_DECODINGS) {
				if (!perform_error_correction(ber, alg_type, mm, stat_iter)) {
					++failures;
				}
				++total_iters;
			}
			#ifdef NDEBUG
			fer_sum_sync.lock();
			#endif
			fers_for_ber.push_back(static_cast<double>(failures) / static_cast<double>(total_iters));
		#ifdef NDEBUG
			fer_sum_sync.unlock();
		});
		#endif

		// std::cout << stat_iter << std::endl;
	}
	#ifdef NDEBUG
	executor.run(taskflow).wait(); 
	#endif

	return compute_mean_and_std(fers_for_ber);
}


auto BaseBenchmark::run(double ber_start, double ber_stop, double ber_step, LDPC_algo alg_type, bool verbose) -> RunningResult const
{
	size_t constexpr STAT_ITERATIONS{30}; // 100

	std::vector<double> fers;
	std::vector<double> fer_std_devs;
	std::vector<double> bers;

	size_t interval_number{0};

	MemoryManager mm{m_H, STAT_ITERATIONS};

	for (double current_ber{ber_start}; current_ber < ber_stop; current_ber += ber_step) {

		auto [fer_av_for_epsilon, fer_std_dev_for_epsilon] = compute_one_point(current_ber, alg_type, mm, STAT_ITERATIONS, verbose);

		bers.push_back(current_ber);
		fers.push_back(fer_av_for_epsilon);
		fer_std_devs.push_back(fer_std_dev_for_epsilon);

		if (verbose) {
			std::cout << "Interval " << interval_number + 1 << "/" << round((ber_stop - ber_start) / ber_step) + 1;
			std::cout << ": ber = " << std::setw(4) << current_ber << ",\tfer = " << fer_av_for_epsilon << ",\t\tfer std dev = " << fer_std_dev_for_epsilon << std::endl;
		}

		++interval_number;
	}

	return {bers, fers, fer_std_devs};
}


auto BaseBenchmark::find_intersection(double ber_start, double ber_stop, double ber_prec, double threshold, LDPC_algo alg_type, bool verbose) -> double const
{
    if (ber_start >= ber_stop || ber_prec <= 0) {
        throw std::runtime_error{"Invalid BER parameters"};
    }

	size_t constexpr STAT_ITERATIONS{30}; // 100
    double left{ber_start};
    double right{ber_stop};
	MemoryManager mm{m_H, STAT_ITERATIONS};

    while (right - left > ber_prec) {
        double mid{(left + right) / 2.0};

		auto [fer_av_for_epsilon, fer_std_dev_for_epsilon] = compute_one_point(mid, alg_type, mm, STAT_ITERATIONS, verbose);

		double fer{fer_av_for_epsilon};

        if (verbose) {
            std::cout << "Testing BER-" << mid << ", FER-" << fer << std::endl;
        }

        if (fer < threshold) {
            left = mid;
        } else {
            right = mid;
        }
    }
    return (left + right) / 2.0;
}


void BaseBenchmark::change_m_H(std::vector<std::pair<int, int>> changes) {
	for (auto [row, col] : changes) {
		this->m_H.coeff(row, col) = this->m_H.coeff(row, col) + GF2(1);
	}
}

auto ClassicEC::perform_error_correction(double ber, LDPC_algo alg_type, MemoryManager const& mm, size_t thread_index) -> bool const
{
	// Only zero-message support for now
	// TODO: add support for arbitrary messages
	Eigen::Vector<GF2, Eigen::Dynamic> message(Eigen::Vector<GF2, Eigen::Dynamic>::Zero(m_H.cols() - m_H.rows()));

	Eigen::Vector<GF2, Eigen::Dynamic> codeword{encode(message)};

	Eigen::Vector<double, Eigen::Dynamic> received_data{add_errors(codeword, ber)};

	std::vector<LLR> llrs{compute_llrs(received_data, ber)};

	size_t constexpr DECODING_ITERS_NUMBER{30}; // 50
	
	switch (alg_type) {
		case LDPC_algo::SP:
			if (decode_sum_product_opt(m_H, llrs, DECODING_ITERS_NUMBER) == message) {
				return true;
			}
			break;
		case LDPC_algo::MS:
			if (decode_normalized_min_sum_opt(m_H, llrs, 1.0, DECODING_ITERS_NUMBER) == message) {
				return true;
			}
			break;
		case LDPC_algo::NMS:
			if (decode_normalized_min_sum_opt(m_H, llrs, 0.75, DECODING_ITERS_NUMBER) == message) {
				return true;
			}
			break;
		case LDPC_algo::LMS:
			if (decode_layered_normalized_min_sum(m_H, llrs, m_Z, 1.0, DECODING_ITERS_NUMBER) == message) {
				return true;
			}
			break;
		case LDPC_algo::LNMS:
			if (decode_layered_normalized_min_sum(m_H, llrs, m_Z, 0.75, DECODING_ITERS_NUMBER) == message) {
				return true;
			}
			break;
		default:
			throw std::runtime_error{"Invalid LDPC algorithm for ClassicEC benchmark"};
	}
	return false;
}


auto ClassicEC::encode(Eigen::Vector<GF2, Eigen::Dynamic> const& message) -> Eigen::Vector<GF2, Eigen::Dynamic> const
{
	if (message.size() != (m_H.cols() - m_H.rows())) {
		throw std::runtime_error{"Encoder: Unable to encode: message size incompatible"};
	}

	// TODO: add support for arbitrary messages
	if (!message.isZero()) {
		throw std::runtime_error{"Encoder: Non-zero messages are not supported for now"};
	}

	return Eigen::Vector<GF2, Eigen::Dynamic>::Zero(m_H.cols());
}


auto BSChannellEC::compute_llrs(Eigen::Vector<double, Eigen::Dynamic> const& received_data, double ber) -> std::vector<LLR> const
{
	std::vector<LLR> llrs;

	double llr;
	bool condition;
	double const llr_base_val{std::min(log((1.0 - ber) / ber), 1000.)}; // min is used for cases with extremely small ber

	for (size_t i{0}; i < received_data.size(); ++i) {
		condition = received_data[i] == 0.;
		if (condition) { llr = llr_base_val; }  // P(0)
		else { llr = -llr_base_val; }  // = log(eps/(1-eps), P(1)
		llrs.push_back(llr);
	}

	return llrs;
}


auto BSChannellEC::add_errors(Eigen::Vector<GF2, Eigen::Dynamic> const& codeword, double ber) -> Eigen::Vector<double, Eigen::Dynamic> const
{
	std::mt19937 random_engine{std::chrono::steady_clock::now().time_since_epoch().count()};
	std::uniform_real_distribution<> error_distribution{0.0, 1.0};

	Eigen::Vector<double, Eigen::Dynamic> received_data(codeword.size());
	std::copy(codeword.begin(), codeword.end(), received_data.begin());
	for (double &bit: received_data) {
		if (error_distribution(random_engine) < ber)
			bit = !bit;
	}

	return received_data;
}


auto BIAWGNChannellEC::compute_llrs(Eigen::Vector<double, Eigen::Dynamic> const& received_data, double ber) -> std::vector<LLR> const
{
	std::vector<LLR> llrs;
	long double const sigma {ber_to_sigma(ber)};
	long double const multiplier {2. / (sigma * sigma)};

	for (double symbol : received_data) {
		llrs.push_back(multiplier * symbol);
	}

	return llrs;
}


auto BIAWGNChannellEC::add_errors(Eigen::Vector<GF2, Eigen::Dynamic> const& codeword, double ber) -> Eigen::Vector<double, Eigen::Dynamic> const
{
	std::mt19937 random_engine{std::chrono::steady_clock::now().time_since_epoch().count()};
	std::normal_distribution<> error_distribution{0.0, 1.0};

	long double const sigma {ber_to_sigma(ber)};

	Eigen::Vector<double, Eigen::Dynamic> received_data(codeword.size());
	std::transform(codeword.begin(), codeword.end(), received_data.begin(), [](GF2 bit){ return 1. - 2 * static_cast<double>(bit); });
	for (double &bit : received_data) {
		bit = bit + sigma * error_distribution(random_engine);
	}

	return received_data;
}


auto BIAWGNChannellEC::ber_to_sigma(double ber) -> double const
{
	return 1. / (sqrtl(2.) * erfcinv(2 * ber));
}


auto WynersEC::perform_error_correction(double ber, LDPC_algo alg_type, MemoryManager const& mm, size_t thread_index) -> bool const
{
	Eigen::Vector<GF2, Eigen::Dynamic> message{gen_rand_bit_seq(m_H.cols())};

	Eigen::Vector<GF2, Eigen::Dynamic> syndrome{m_H * message};

	Eigen::Vector<double, Eigen::Dynamic> received_data{add_errors(message, ber)};

	std::vector<LLR> llrs{compute_llrs(received_data, ber)};

	size_t constexpr DECODING_ITERS_NUMBER{30}; // 50
	
	switch (alg_type) {
		case LDPC_algo::SP:
			if (decode_sp_to_syndrome(m_H, llrs, syndrome, DECODING_ITERS_NUMBER) == message) {
				return true;
			}
			break;
		case LDPC_algo::MS:
			if (decode_nms_to_syndrome_r(m_H, llrs, syndrome, mm, thread_index, 1.0, DECODING_ITERS_NUMBER) == message) {
				return true;
			}
			break;
		case LDPC_algo::NMS:
			if (decode_nms_to_syndrome_r(m_H, llrs, syndrome, mm, thread_index, 0.75, DECODING_ITERS_NUMBER) == message) {
				return true;
			}
			break;
		case LDPC_algo::LMS:
			if (decode_lnms_to_syndrome(m_H, llrs, syndrome, m_Z, 1.0, DECODING_ITERS_NUMBER) == message) {
				return true;
			}
			break;
		case LDPC_algo::LNMS:
			if (decode_lnms_to_syndrome(m_H, llrs, syndrome, m_Z, 0.75, DECODING_ITERS_NUMBER) == message) {
				return true;
			}
			break;
		default:
			throw std::runtime_error{"Invalid LDPC algorithm for WynersEC benchmark"};
	}
	return false;
}


auto BSChannellWynersEC::add_errors(Eigen::Vector<GF2, Eigen::Dynamic> const& codeword, double ber) -> Eigen::Vector<double, Eigen::Dynamic> const
{
	std::mt19937 random_engine{std::chrono::steady_clock::now().time_since_epoch().count()};
	std::uniform_real_distribution<> error_distribution{0.0, 1.0};

	Eigen::Vector<double, Eigen::Dynamic> received_data(codeword.size());
	std::copy(codeword.begin(), codeword.end(), received_data.begin());
	for (double &bit: received_data) {
		if (error_distribution(random_engine) < ber)
			bit = !bit;
	}

	return received_data;
}


auto BSChannellWynersEC::compute_llrs(Eigen::Vector<double, Eigen::Dynamic> const& received_data, double ber) -> std::vector<LLR> const
{
	std::vector<LLR> llrs;

	double llr;
	bool condition;
	double const llr_base_val{std::min(log((1.0 - ber) / ber), 1000.)}; // min is used for cases with extremely small ber

	for (size_t i{0}; i < received_data.size(); ++i) {
		condition = received_data[i] == 0.;
		if (condition) { llr = llr_base_val; }  // P(0)
		else { llr = -llr_base_val; }  // = log(eps/(1-eps), P(1)
		llrs.push_back(llr);
	}

	return llrs;
}


auto BUSChannellWynersEC::add_errors(Eigen::Vector<GF2, Eigen::Dynamic> const& codeword, double ber) -> Eigen::Vector<double, Eigen::Dynamic> const
{
	#ifdef DEBUG
	size_t seed{std::chrono::steady_clock::now().time_since_epoch().count()};
	#else
	size_t seed{std::chrono::steady_clock::now().time_since_epoch().count()};
	// size_t seed{0};
	#endif


	if (m_changing_err_index.first == -1 && m_changing_err_index.second == -1) { // Want to increase all error levels to BER instead of setting one
		for (uint8_t bit_index{0}; bit_index < 2; ++bit_index) {
			for (uint8_t detector_index{0}; detector_index < 2; ++detector_index) {
				m_error_distribution[bit_index][detector_index] = m_initial_error_distribution[bit_index][detector_index] + ber;
			}
		}
	}
	else { // Set only one error level
		m_error_distribution[m_changing_err_index.first][m_changing_err_index.second] = ber;
	}

	std::mt19937 random_engine;
	random_engine.seed(seed);
	std::uniform_real_distribution<> error_distribution{0.0, 1.0};
	std::uniform_int_distribution<> detector_distribution{0, 1};

	Eigen::Vector<double, Eigen::Dynamic> received_data(codeword.size());
	std::copy(codeword.begin(), codeword.end(), received_data.begin());

	m_bit_group_distribution_mutex.lock();
	if (m_bit_group_distribution[std::this_thread::get_id()].empty()) {
		m_bit_group_distribution[std::this_thread::get_id()] = std::vector<GF2>(codeword.size());
	}
	m_bit_group_distribution_mutex.unlock();

	for (size_t i{0}; i < received_data.size(); ++i) {
		size_t detector{detector_distribution(random_engine)};

		m_bit_group_distribution_mutex.lock();
		m_bit_group_distribution[std::this_thread::get_id()][i] = detector;
		m_bit_group_distribution_mutex.unlock();

		if (error_distribution(random_engine) < m_error_distribution[received_data[i]][detector])
			received_data[i] = !received_data[i];
	}

	// print_seq(codeword);
	// print_seq(m_bit_group_distribution[std::this_thread::get_id()]);
	// print_seq(received_data);

	return received_data;
}


auto BUSChannellWynersEC::compute_llrs(Eigen::Vector<double, Eigen::Dynamic> const& received_data, double ber) -> std::vector<LLR> const
{
	m_bit_group_distribution_mutex.lock();
	if (m_bit_group_distribution[std::this_thread::get_id()].empty()) {
		throw std::runtime_error{"BUSChannellWynersEC::compute_llrs: add errors first!"};
	}
	m_bit_group_distribution_mutex.unlock();

	if (m_changing_err_index.first == -1 && m_changing_err_index.second == -1) { // Want to increase all error levels to BER instead of setting one
		for (uint8_t bit_index{0}; bit_index < 2; ++bit_index) {
			for (uint8_t detector_index{0}; detector_index < 2; ++detector_index) {
				m_error_distribution[bit_index][detector_index] = m_initial_error_distribution[bit_index][detector_index] + ber;
			}
		}
	}
	else { // Set only one error level
		m_error_distribution[m_changing_err_index.first][m_changing_err_index.second] = ber;
	}

	std::vector<LLR> llrs;

	double llr;
	bool condition;
	
	if (!m_modify_llrs) {

		double ber_sum{0.0};
		for (auto const& l1 : m_error_distribution) {
			for (auto const& l2 : l1) {
				ber_sum += l2;
			}
		}
		double av_ber = ber_sum / 4; // Average ber in distribution

		double const llr_base_val{std::min(log((1.0 - av_ber) / av_ber), 1000.)}; // min is used for cases with extremely small ber

		for (size_t i{0}; i < received_data.size(); ++i) {
			condition = received_data[i] == 0.;
			if (condition) { llr = llr_base_val; }  // P(0)
			else { llr = -llr_base_val; }  // = log(eps/(1-eps), P(1)
			llrs.push_back(llr);
		}
	}
	else {
		error_distribution_t llr_base_vals;

		llr_base_vals[0][0] = std::min(log((1.0 - (m_error_distribution[0][0])) / (m_error_distribution[1][0])), 1000.);
		llr_base_vals[0][1] = std::min(log((1.0 - (m_error_distribution[0][1])) / (m_error_distribution[1][1])), 1000.);
		llr_base_vals[1][0] = std::max(log((m_error_distribution[0][0]) / (1.0 - (m_error_distribution[1][0]))), -1000.);
		llr_base_vals[1][1] = std::max(log((m_error_distribution[0][1]) / (1.0 - (m_error_distribution[1][1]))), -1000.);

		for (size_t i{0}; i < received_data.size(); ++i) {

			m_bit_group_distribution_mutex.lock();
			size_t detector{m_bit_group_distribution[std::this_thread::get_id()][i]};
			m_bit_group_distribution_mutex.unlock();

			llr = llr_base_vals[int(received_data[i])][detector];
			llrs.push_back(llr);
		}
	}

	// print_seq(received_data);
	// print_seq(m_bit_group_distribution[std::this_thread::get_id()]);
	// print_seq(llrs);

	return llrs;
}


benchmarks::BenchmarkRange::BenchmarkRange(double _start, double _stop, double _step) {
    start = _start;
    stop = _stop;
    step = _step;
    length = int((stop - start) / step) + 1;
}


std::pair<double, double> compute_mean_and_std(std::vector<double> const& values)
{
	double mean{std::accumulate(values.begin(), values.end(), 0.0) / values.size()};
	double sq_sum{0.};
	for (double val : values) {
		sq_sum += pow(mean - val, 2);
	}
	double std_dev{sqrt(sq_sum / values.size())};

	return {mean, std_dev};
}


auto ExposedBUSChannellWynersEC::run(BenchmarkRange ber_range, BenchmarkRange exposed_rate_range, LDPC_algo alg_type, bool verbose) -> ExposedRunningResult const
{
    size_t constexpr STAT_ITERATIONS{30}; // 100
    size_t constexpr MAX_FAILURES{50}; // 100
    size_t constexpr MAX_DECODINGS{2000}; // 10000
    size_t constexpr SUB_THREADS_NUMBER{3}; // 3

    std::vector<double> fers;
    std::vector<double> fer_std_devs;
    std::vector<double> estimated_bers;
    std::vector<double> estimated_ber_std_devs;
    std::vector<double> bers;
    std::vector<double> exposed_rates;

    size_t interval_number{0};

	MemoryManager mm{m_H, STAT_ITERATIONS};

    for (int i = 0; i < ber_range.length; ++i) {
        for (int j = 0; j < exposed_rate_range.length; ++j) {
            std::vector<double> fers_for_ber;
            std::vector<double> estimated_bers_list;
            fers_for_ber.reserve(ber_range.length);
            estimated_bers_list.reserve(ber_range.length);

            double current_ber = ber_range.start + i * ber_range.step;
            double current_exposed = exposed_rate_range.start + j * exposed_rate_range.step;

            #ifdef NDEBUG
            tf::Executor executor;
            tf::Taskflow taskflow;
            std::mutex fer_sum_sync;
            #endif
            for (size_t stat_iter{0}; stat_iter < STAT_ITERATIONS; ++stat_iter) {
                #ifdef NDEBUG
                taskflow.emplace([=, &fers_for_ber, &estimated_bers_list, &fer_sum_sync]() {
                #endif
                std::atomic_size_t failures{0}; // No need to synchronize
                std::atomic_size_t total_iters{0}; // No need to synchronize
                double estimated_ber_sum{0};
                size_t iter_count = 0;
                while (failures < MAX_FAILURES && total_iters < MAX_DECODINGS) {
                    ++iter_count;
                    double cur_estimated_ber{0};
                    bool is_fail = !perform_error_correction(current_ber, current_exposed, alg_type, cur_estimated_ber, mm, stat_iter);
                    estimated_ber_sum += cur_estimated_ber;
                    if (is_fail) {
                        ++failures;
                    }
                    ++total_iters;
                }
                #ifdef NDEBUG
                fer_sum_sync.lock();
                #endif
                fers_for_ber.push_back(static_cast<double>(failures) / static_cast<double>(total_iters));
                estimated_bers_list.push_back(estimated_ber_sum / static_cast<double>(iter_count));
                #ifdef NDEBUG
                fer_sum_sync.unlock();
            });
                #endif

                // std::cout << stat_iter << std::endl;
            }
            #ifdef NDEBUG
            executor.run(taskflow).wait();
            #endif

            auto [fer_av_for_exposed, fer_std_dev_for_exposed] = compute_mean_and_std(fers_for_ber);
            auto [av_estimated_ber, std_dev_estimated_ber] = compute_mean_and_std(estimated_bers_list);

            exposed_rates.push_back(current_exposed);
            bers.push_back(current_ber);
            fers.push_back(fer_av_for_exposed);
            fer_std_devs.push_back(fer_std_dev_for_exposed);
            estimated_bers.push_back(av_estimated_ber);
            estimated_ber_std_devs.push_back(std_dev_estimated_ber);


            if (verbose) {
                ++interval_number;
                std::cout << "Interval " << interval_number << "/" << std::to_string(ber_range.length * exposed_rate_range.length);
                std::cout << ": exposed_rate = " << current_exposed
                << ",\tber = " << current_ber
				<< std::setprecision(6)
                << ",\tfer = " << fer_av_for_exposed
                << ",\testimated_ber = " << av_estimated_ber
                << ",\tfer std dev = " << fer_std_dev_for_exposed
//                << ",\testimated_bers std dev = " << float_to_string(std_dev_estimated_ber, 6)
                << std::endl;
            }


        }
    }

    return {bers, exposed_rates, fers, fer_std_devs, estimated_bers, estimated_ber_std_devs};
}


auto ExposedBUSChannellWynersEC::perform_error_correction(double ber, double exposed_bits_rate, LDPC_algo alg_type, double &estimated_ber, MemoryManager const& mm, size_t thread_index) -> bool const
{
        Eigen::Vector<GF2, Eigen::Dynamic> message{gen_rand_bit_seq(m_H.cols())};

        Eigen::Vector<GF2, Eigen::Dynamic> syndrome{m_H * message};

        Eigen::Vector<double, Eigen::Dynamic> received_data{add_errors(message, ber)};

        estimated_ber = estimate_ber_by_exposed(message, received_data, exposed_bits_rate);
        std::vector<LLR> llrs{compute_llrs(received_data, estimated_ber)};

        size_t constexpr DECODING_ITERS_NUMBER{30}; // 50

        switch (alg_type) {
			case LDPC_algo::SP:
				if (decode_sp_to_syndrome(m_H, llrs, syndrome, DECODING_ITERS_NUMBER) == message) {
					return true;
				}
				break;
			case LDPC_algo::MS:
				if (decode_nms_to_syndrome_r(m_H, llrs, syndrome, mm, thread_index, 1.0, DECODING_ITERS_NUMBER) == message) {
					return true;
				}
				break;
			case LDPC_algo::NMS:
				if (decode_nms_to_syndrome_r(m_H, llrs, syndrome, mm, thread_index, 0.75, DECODING_ITERS_NUMBER) == message) {
					return true;
				}
				break;
			case LDPC_algo::LMS:
				if (decode_lnms_to_syndrome(m_H, llrs, syndrome, m_Z, 1.0, DECODING_ITERS_NUMBER) == message) {
					return true;
				}
				break;
			case LDPC_algo::LNMS:
				if (decode_lnms_to_syndrome(m_H, llrs, syndrome, m_Z, 0.75, DECODING_ITERS_NUMBER) == message) {
					return true;
				}
				break;
            default:
                throw std::runtime_error{"Invalid LDPC algorithm for WynersEC benchmark"};
        }
        return false;
}


} // namespace benchmarks