#include <iostream>
#include <vector>
#include "result-exposed.h"

std::string implode(const std::vector<std::string> &list, const std::string &separator = ";") {
    std::string out;
    for (size_t i = 0; i < list.size(); ++i) {
        out += list[i];
        if (i != list.size() - 1) {
            out += separator;
        }
    }

    return out;
}

std::string get_item(int row, const std::vector<double> &v){
    return row < v.size() ? std::to_string(v.at(row)) : "-";
}

std::string exported_result_to_csv(ExposedResult &obj) {
    std::string out = implode({"N", "BERS", "EXPOSED_RATES", "FERS", "FER_STD_DEVS", "ESTIMATED_BERS", "ESTIMATED_BER_STD_DEVS"}) + '\n';
    int table_rows = std::max({obj.result.bers.size(),obj.result.exposed_rates.size(), obj.result.fers.size(), obj.result.fer_std_devs.size()});
    for (int row = 0; row < table_rows; row++) {
        out+=implode({
                             std::to_string(row),
                             get_item(row, obj.result.bers),
                             get_item(row, obj.result.exposed_rates),
                             get_item(row, obj.result.fers),
                             get_item(row, obj.result.fer_std_devs),
                             get_item(row, obj.result.estimated_bers),
                             get_item(row, obj.result.estimated_ber_std_devs),
                     }) + '\n';
    }

    return out;
}

std::ostream &operator<<(std::ostream &stream, ExposedResult &obj) {

    int table_rows = std::max({obj.result.bers.size(),obj.result.exposed_rates.size(), obj.result.fers.size(), obj.result.fer_std_devs.size()});

    auto comp_function = [](double left, double right) {
        return std::to_string(left).size() < std::to_string(right).size();
    };

    // determine max width of each column ("2+" for 1 prefix and 1 suffix whitespaces)
    int num_col_width = 2+std::to_string(table_rows).size();

    int bers_col_width = 2+std::to_string(*std::max_element(obj.result.bers.begin(),
                                                            obj.result.bers.end(), comp_function)).size();

    int exposes_col_width = 2+std::to_string(*std::max_element(obj.result.exposed_rates.begin(),
                                                            obj.result.exposed_rates.end(), comp_function)).size();

    int fers_col_width = 2+std::to_string(*std::max_element(obj.result.fers.begin(),
                                                            obj.result.fers.end(), comp_function)).size();

    int std_dev_col_width = 2+std::to_string(*std::max_element(obj.result.fer_std_devs.begin(),
                                                               obj.result.fer_std_devs.end(), comp_function)).size();

    std::string num_cell_str = "N";
    std::string bers_cell_str = "BERS";
    std::string exposes_cell_str = "EXPOSED_RATES";
    std::string fers_cell_str = "FERS";
    std::string std_dev_cell_str = "FER_STD_DEVS";
    bers_col_width = std::max(2+bers_cell_str.size(), (size_t)bers_col_width);
    exposes_col_width = std::max(2+exposes_cell_str.size(), (size_t)exposes_col_width);
    fers_col_width = std::max(2+fers_cell_str.size()+2, (size_t)fers_col_width);
    std_dev_col_width = std::max(2+std_dev_cell_str.size()+2, (size_t)std_dev_col_width);

    for (int row = -1; row < table_rows; row++) {

        if (row > -1) {
            num_cell_str = std::to_string(row);
            bers_cell_str = row < obj.result.bers.size() ? std::to_string(obj.result.bers.at(row)) : "-";
            exposes_cell_str = row < obj.result.exposed_rates.size() ? std::to_string(obj.result.exposed_rates.at(row)) : "-";
            fers_cell_str = row < obj.result.fers.size() ? std::to_string(obj.result.fers.at(row)) : "-";
            std_dev_cell_str = row < obj.result.fer_std_devs.size() ? std::to_string(obj.result.fer_std_devs.at(row)) : "-";
        }

        std::cout << "|" << std::string(num_col_width-1-num_cell_str.size(), ' ') << num_cell_str << " " <<
                  "|" << std::string(bers_col_width-1-bers_cell_str.size(), ' ') << bers_cell_str << " " <<
                  "|" << std::string(exposes_col_width-1-exposes_cell_str.size(), ' ') << exposes_cell_str << " " <<
                  "|" << std::string(fers_col_width-1-fers_cell_str.size(), ' ') << fers_cell_str << " " <<
                  "|" << std::string(std_dev_col_width-1-std_dev_cell_str.size(), ' ') << std_dev_cell_str << " " <<
                  "|" << std::endl;

    }
    return stream;
}


bool all_under_line(std::vector<double> const& values, double line)
// Проверяет, что все числа вектора находятся под пороговой линией
{
    for (double value : values) {
        if (value > line) {
            return false;
        }
    }

    return true;
}


double sum_distance_from_threshold(std::vector<double> const& values, double line)
// Вычисляет сумму расстояний всех точек вектора от пороговой линии
{
    double distance{0};

    for (double value : values) {
        if (value > line) {
            throw std::runtime_error{"Value is over the threshold"};
        }
        distance += (line - value);
    }

    return distance;
}


double find_intersection_point(std::vector<double> const& x_values, std::vector<double> const& y_values, double line)
{
    for (size_t i{0}; i < y_values.size(); ++i) {
        if (y_values[i] > line) {
            double x1{x_values[i - 1]}, y1{y_values[i - 1]}, x2{x_values[i]}, y2{y_values[i]};
            double x3{x_values[i - 1]}, y3{line}, x4{x_values[i]}, y4{line};
            return ((x1 * y2 - y1 * x2) * (x3 - x4) - (x1 - x2) * (x3 * y4 - y3 * x4)) / ((x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4));
        }
    }
}


bool ExposedResult::operator<(ExposedResult const& right) const
{
    double constexpr line{0.001};

    // Checking if results are compatible (represent same experiment)

    if (result.bers.size() != right.result.bers.size()) {
        throw std::runtime_error{"Incompatible results (bers size)"};
    }
    if (result.exposed_rates.size() != right.result.exposed_rates.size()) {
        throw std::runtime_error{"Incompatible results (exposed_rates size)"};
    }

    for (size_t j{0}; j < result.bers.size(); ++j) {
        if (result.bers[j] != right.result.bers[j]) {
            throw std::runtime_error{"Incompatible BERs in results"};
        }
    }
    for (size_t j{0}; j < result.exposed_rates.size(); ++j) {
        if (result.exposed_rates[j] != right.result.exposed_rates[j]) {
            throw std::runtime_error{"Incompatible exposed_rates in results"};
        }
    }

    bool res1_under_line{all_under_line(this->result.fers, line)};
    bool res2_under_line{all_under_line(right.result.fers, line)};

    if (res1_under_line && res2_under_line) {
        return sum_distance_from_threshold(this->result.fers, line) > sum_distance_from_threshold(right.result.fers, line);
    }

    if (res1_under_line) {
        return true;
    }

    if (res2_under_line) {
        return false;
    }

    return find_intersection_point(this->result.exposed_rates, this->result.fers, line) > find_intersection_point(right.result.exposed_rates, right.result.fers, line);
}


ExposedResult compute_average_result(std::vector<ExposedResult> const& results)
{
    if (results.empty()) {
        throw std::runtime_error{"Empty results vector"};
    }

    // Checking if results are compatible (represent same experiment)

    for (size_t i{0}; i < results.size() - 1; ++i) {
        if (results[i].result.bers.size() != results[i + 1].result.bers.size()) {
            throw std::runtime_error{"Incompatible results (bers.size)"};
        }
        for (size_t j{0}; j < results[i].result.bers.size(); ++j) {
            if (results[i].result.bers[j] != results[i + 1].result.bers[j]) {
                throw std::runtime_error{"Incompatible BERs in results"};
            }
        }

        if (results[i].result.exposed_rates.size() != results[i + 1].result.exposed_rates.size()) {
            throw std::runtime_error{"Incompatible results (exposed_rates size)"};
        }
        for (size_t j{0}; j < results[i].result.exposed_rates.size(); ++j) {
            if (results[i].result.exposed_rates[j] != results[i + 1].result.exposed_rates[j]) {
                throw std::runtime_error{"Incompatible exposed_rates in results"};
            }
        }
    }

    ExposedResult average_res;

    average_res.result.exposed_rates = results[0].result.exposed_rates;

    for (size_t j{0}; j < results[0].result.fers.size(); ++j) {
        std::vector<double> fers_for_one_dot;
        fers_for_one_dot.reserve(results.size());

        for (size_t i{0}; i < results.size() - 1; ++i) {
            fers_for_one_dot.push_back(results[i].result.fers[j]);
        }

        auto [mean_fer, fer_std_dev] = compute_mean_and_std(fers_for_one_dot);
        average_res.result.fers.push_back(mean_fer);
        average_res.result.fer_std_devs.push_back(fer_std_dev);
    }

    return average_res;
}