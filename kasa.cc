#include <iostream>
#include <string>
#include <unordered_map>
#include <map>
#include <regex>
#include <stdexcept>
#include <optional>
#include <functional>
#include <limits>
#include <unordered_set>
#include <fstream>
#include <cassert>

using ticket_office_exn = std::invalid_argument;

using clock_nat_t = uint16_t;
enum { _hour = 0, _minute = 1 };
using stop_time_t = std::tuple<clock_nat_t, clock_nat_t>;

int compare_stop_time(const stop_time_t& lhs, const stop_time_t& rhs) {
    const auto& [lhs_hour, lhs_minute] = lhs;
    const auto& [rhs_hour, rhs_minute] = rhs;

    if (lhs_hour != rhs_hour) {
        return lhs_hour < rhs_hour ? -1: 1;
    }
    else if (lhs_minute != rhs_minute) {
        return lhs_minute < rhs_minute ? -1: 1;
    }
    else {
        return 0;
    }
}
bool operator<=(const stop_time_t& lhs, const stop_time_t& rhs) {
    return compare_stop_time(lhs, rhs) <= 0;
}
bool operator>=(const stop_time_t& lhs, const stop_time_t& rhs) {
    return compare_stop_time(lhs, rhs) >= 0;
}
bool operator<(const stop_time_t& lhs, const stop_time_t& rhs) {
    return compare_stop_time(lhs, rhs) < 0;
}
bool operator>(const stop_time_t& lhs, const stop_time_t& rhs) {
    return compare_stop_time(lhs, rhs) > 0;
}
bool operator==(const stop_time_t& lhs, const stop_time_t& rhs) {
    return compare_stop_time(lhs, rhs) == 0;
}
bool operator!=(const stop_time_t& lhs, const stop_time_t& rhs) {
    return compare_stop_time(lhs, rhs) != 0;
}

stop_time_t stop_time_from_string(const std::string& str) {
    static const auto stop_time_re = std::regex("([0-9]+):([0-9][0-9])");
    std::smatch m;

    if (!regex_match(str, m, stop_time_re)) {
        throw ticket_office_exn("provided str not a valid stop time str");
    }

    stop_time_t stop_time {};
    auto& [hour, minute] = stop_time;

    std::stringstream ss {};
    ss << m[1].str(); ss >> hour;

    ss = {};
    ss << m[2].str(); ss >> minute;

    if (!(hour <= 23 && minute <= 59)) {
        throw ticket_office_exn("number fields in str are invalid");
    }

    static const auto start = stop_time_t(5, 55), end = stop_time_t(21, 21);
    if (!(start <= stop_time && stop_time <= end)) {
        throw ticket_office_exn("provided stop time is outside of operation range");
    }

    return stop_time;
}

using stop_id_t = std::string;
using seq_index_t = size_t;
enum { _seq_index = 0, _stop_time = 1 };
using tramline_t = std::unordered_map<stop_id_t, std::tuple<seq_index_t, stop_time_t>>;

using tramline_id_t = int;
using tramlines_t = std::unordered_map<tramline_id_t, tramline_t>;

using ticket_price_t = uint64_t;
using ticket_id_t = std::string;
using ticket_dur_t = uint64_t;
enum { _ticket_dur = 0, _ticket_id = 1 };
using price_map_t = std::map<ticket_price_t, std::tuple<ticket_dur_t, ticket_id_t>>;
enum { _ticket_ids = 0, _price_map = 1 };
using tickets_t = std::tuple<std::unordered_set<ticket_id_t>, price_map_t>;

ticket_dur_t stop_time_diff(const stop_time_t& time1, const stop_time_t& time2) {
    const auto& [hour1, minute1] = time1;
    const auto& [hour2, minute2] = time2;
    
    ticket_dur_t abs1 = 60 * hour1 + minute1, abs2 = 60 * hour2 + minute2;
    return abs1 > abs2 ? abs1 - abs2 : abs2 - abs1;
}

using counter_t = uint32_t;
enum { _tramlines = 0, _tickets = 1, _counter = 2 };
using state_t = std::tuple<tramlines_t, tickets_t, counter_t>;

void process_tramline_addition(const std::string& line, tramlines_t& tramlines) {
    std::stringstream ss {};
    ss << line;

    tramline_id_t id; ss >> id;
    if (tramlines.find(id) != tramlines.end()) {
        throw ticket_office_exn("tramline with specified id already exists");
    }
    else {
        tramline_t tram_line = {};
        auto prior_time = std::optional<stop_time_t>();

        for (size_t idx = 0; ss.rdbuf()->in_avail() > 0; ++idx) {
            std::string time_str; ss >> time_str;
            stop_time_t stop_time = stop_time_from_string(time_str);
            stop_id_t stop_id; ss >> stop_id;

            if (prior_time.has_value() && prior_time.value() >= stop_time) {
                throw ticket_office_exn("stops not in causal order");
            }
            if (tram_line.find(stop_id) != tram_line.end()) {
                throw ticket_office_exn("some stop repeated");
            }

            prior_time = std::make_optional(stop_time);
            tram_line[stop_id] = {idx, stop_time};
        }

        tramlines[id] = tram_line;
    }
}

void process_ticket_addition(const std::smatch& m, tickets_t& tickets) {
    ticket_id_t id = m[1].str();

    std::string price_str = m[2].str();
    // following line removes the dot from price, i.e. we store prices in an integral form of 1/100th per unit
    price_str.erase(std::remove(price_str.begin(), price_str.end(), '.'), price_str.end());
    ticket_price_t price;
    std::stringstream ss {};
    ss << price_str; ss >> price;

    ticket_dur_t dur;
    ss = {};
    ss << m[3].str(); ss >> dur;

    auto&[ids, price_map] = tickets;

    if (ids.find(id) != ids.end()) {
        throw ticket_office_exn("ticket with specified id already exists");
    } else {
        if (price_map.find(price) != price_map.end()) {
            auto &cur = price_map.at(price);
            auto& cur_dur = std::get<_ticket_dur>(cur);
            if (cur_dur < dur) cur = {dur, id};
        } else {
            price_map[price] = {dur, id};
        }
    }
}

enum { _prices_vector = 0, _total_price = 1, _total_dur = 2 };
using branch_state_t = std::tuple<std::vector<ticket_price_t>, ticket_price_t, ticket_dur_t>;

bool operator<(const branch_state_t& lhs, const branch_state_t& rhs) {
    return std::get<_total_price>(lhs) < std::get<_total_price>(rhs);
}

void find_ticket_set(const ticket_dur_t& dur, const price_map_t& price_map, counter_t& counter) {
    auto optimal = std::optional<branch_state_t>();
    
    using branch_off_t = std::function<void(const branch_state_t&)>;
    branch_off_t branch_off = [&](const branch_state_t& branch) {
        const auto& [chosen_prices, total_price, total_dur] = branch;
        
        if (chosen_prices.size() >= 3 || total_dur >= dur) {
            if (total_dur >= dur) {
                if (optimal.has_value()) optimal.value() = min(optimal.value(), branch);
                else optimal = std::make_optional(branch);
            }         
        }        
        else {
            auto cost_bound = !chosen_prices.empty() ? chosen_prices.back() 
                                                     : std::numeric_limits<ticket_price_t>::max();
            if (optimal.has_value()) {
                const auto& opt_total = std::get<_total_price>(optimal.value());
                cost_bound = std::min(cost_bound, opt_total > total_price ? opt_total - total_price: 0);
            }

            for (auto it = price_map.begin(); it != price_map.upper_bound(cost_bound); ++it) {
                const auto& ext_price = it->first;
                const auto& ext_dur = std::get<_ticket_dur>(it->second);

                auto new_chosen_prices = chosen_prices;
                new_chosen_prices.push_back(ext_price);
                branch_off({ new_chosen_prices, total_price + ext_price, total_dur + ext_dur });
            }
        }
    };

    branch_off({ std::vector<ticket_price_t>(), 0, 0 });

    if (optimal.has_value()) {
        const auto& opt_prices = std::get<_prices_vector>(optimal.value());
        std::cout << "! ";
        for (size_t i = 0; i < opt_prices.size(); ++i) {
            const auto& id = std::get<_ticket_id>(price_map.at(opt_prices[i]));
            std::cout << id;
            if (i != opt_prices.size()-1) {
                std::cout << "; ";
            }
        }
        std::cout << '\n';

        counter += opt_prices.size();
    }
    else {
        std::cout << ":-|" << '\n';
    }
}

void process_ticket_query(const std::string& line, state_t& state) {
    auto& [tramlines, tickets, counter] = state;
    auto& price_map = std::get<_price_map>(tickets);

    std::stringstream ss {};
    ss << line;
    { std::string question_mark; ss >> question_mark; }

    std::vector<stop_id_t> stops_seq;
    std::vector<tramline_id_t> lines_seq;
    
    stop_id_t first_stop; ss >> first_stop;
    stops_seq.push_back(first_stop);

    while (ss.rdbuf()->in_avail() > 0) {
        tramline_id_t tramline_id; ss >> tramline_id;
        lines_seq.push_back(tramline_id);

        stop_id_t stop_id; ss >> stop_id;
        stops_seq.push_back(stop_id);
    }

    auto last_arrival_time = std::optional<stop_time_t>(),
         first_departure_time = std::optional<stop_time_t>();
    auto improper_stop_seq = false;
    auto arrival_before_departure = std::optional<stop_id_t>();

    for (size_t i = 0; i < lines_seq.size(); ++i) {
        const auto &start_stop = stops_seq[i], &end_stop = stops_seq[i+1];
        const auto& tramline = tramlines[lines_seq[i]];

        improper_stop_seq |= tramline.find(start_stop) == tramline.end()
                             || tramline.find(end_stop) == tramline.end();
        const auto& start = tramline.at(start_stop), end = tramline.at(end_stop);
        const auto& [start_seq_idx, start_time] = start;
        const auto& [end_seq_idx, end_time] = end;

        improper_stop_seq |= start_seq_idx >= end_seq_idx;

        if (!first_departure_time.has_value()) {
            first_departure_time = std::make_optional(start_time);
        }

        if (last_arrival_time.has_value()) {
            const auto& last_arrival = last_arrival_time.value();
            improper_stop_seq |= last_arrival > start_time;
            if (last_arrival > start_time && !arrival_before_departure.has_value()) {
                arrival_before_departure = std::make_optional(start_stop);
            }
        }

        last_arrival_time = std::make_optional(end_time);
    }

    if (improper_stop_seq) {
        std::cout << ":-|" << '\n';
    }
    else if (arrival_before_departure.has_value()) {
        std::cout << ":-( " << arrival_before_departure.value() << '\n';
    }
    else {
        auto dur = stop_time_diff(first_departure_time.value(), last_arrival_time.value());
        find_ticket_set(dur, price_map, counter);
    }
}

void process_line(const std::string& line, state_t& state) {
    static const auto line_addition_re = 
        std::regex(R"([0-9]+(?:\ [0-9]+:[0-9][0-9]\ [a-zA-Z_\^]+)+)");
    static const auto ticket_addition_re =
        std::regex(R"(([a-zA-Z\ ]+?)\ ([0-9]*\.[0-9][0-9])\ ([1-9][0-9]*))");
    static const auto ticket_query_re =
        std::regex(R"(\?\ [a-zA-Z_\^]+(?:\ [0-9]+\ [a-zA-Z_\^]+)+)");

    auto& [tramlines, tickets, counter] = state;

    auto m = std::smatch();
    if (regex_match(line, m, line_addition_re)) {
        process_tramline_addition(line, tramlines);
    }
    else if (regex_match(line, m, ticket_addition_re)) {
        process_ticket_addition(m, tickets);
    }
    else if (regex_match(line, m, ticket_query_re)) {
        process_ticket_query(line, state);
    }
    else if (!line.empty()) {
        throw ticket_office_exn("request given in unknown format");
    }
}

std::string from_file(std::ifstream& file) {
    std::string contents;

    file.seekg(0, std::ios::end);
    contents.resize(file.tellg());

    file.seekg(0, std::ios::beg);
    file.read(contents.data(), contents.size());

    return contents;
}

int main() {
    std::string line;
    state_t state;

    auto in_file = std::fstream("tests/example.in", std::ios::in);
    std::cin.rdbuf(in_file.rdbuf());

    auto out_stream = std::stringstream {};
    auto cout_copy = std::cout.rdbuf();
    std::cout.rdbuf(out_stream.rdbuf());

    auto err_stream = std::stringstream {};
    auto cerr_copy = std::cout.rdbuf();
    std::cerr.rdbuf(err_stream.rdbuf());

    for (size_t idx = 1; std::getline(std::cin, line); ++idx) {
        try {
            process_line(line, state);
        }
        catch (const ticket_office_exn& _) {
            std::cerr << "Error in line " << idx << ": " << line << '\n';
        }
    }

    auto& counter = std::get<_counter>(state);
    std::cout << counter << '\n';

    auto correct_out_file = std::ifstream("tests/example.out");
    auto correct_out_str = from_file(correct_out_file);
    auto true_out_str = out_stream.str();

    std::cout.rdbuf(cout_copy);
    std::cout << std::boolalpha << "out correct -> " << (correct_out_str == true_out_str) << '\n';

    auto correct_err_file = std::ifstream("tests/example.err");
    auto correct_err_str = from_file(correct_err_file);
    auto true_err_str = err_stream.str();

    std::cerr.rdbuf(cerr_copy);
    std::cout << std::boolalpha << "err correct -> " << (correct_err_str == true_err_str) << '\n';

    return EXIT_SUCCESS;
}