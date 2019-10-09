#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <map>
#include <regex>
#include <stdexcept>
#include <optional>
#include <functional>
#include <limits>
#include <unordered_set>
using namespace std;

using clock_nat_t = uint8_t; 
enum { _hour = 0, _minute = 1 };
using stop_time_t = tuple<clock_nat_t, clock_nat_t>;

stop_time_t stop_time_from_string(const string& str) {    
    static const auto stop_time_re = regex("([0-9]+):([0-9]+)");    
    smatch m; 

    regex_match(str, m, stop_time_re);

    stop_time_t stop_time;
    auto& [hour, minute] = stop_time;

    stringstream ss {};
    ss << m[1]; ss >> hour;
    ss << m[2]; ss >> minute;

    return stop_time;
}

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

bool is_stop_time_valid(const stop_time_t& stop_time) {
    static const auto start = stop_time_t(5, 55), end = stop_time_t(21, 21);
    return start <= stop_time && stop_time <= end;
}

using stop_id_t = string;
using seq_index_t = size_t;
enum { _seq_index = 0, _stop_time = 1 };
using tramline_t = unordered_map<stop_id_t, tuple<seq_index_t, stop_time_t>>;

using tramline_id_t = int;
using tramlines_t = unordered_map<tramline_id_t, tramline_t>;

using ticket_price_t = uint64_t; // natively in 1/100ths to avoid rounding errors
using ticket_id_t = string;
using ticket_dur_t = uint64_t;
enum { _ticket_dur = 0, _ticket_price = 1 };
using price_map_t = map<ticket_price_t, tuple<ticket_dur_t, ticket_id_t>>;
using tickets_t = tuple<unordered_set<ticket_id_t>, price_map_t>;

ticket_dur_t stop_time_diff(const stop_time_t& time1, const stop_time_t& time2) {
    const auto& [hour1, minute1] = time1;
    const auto& [hour2, minute2] = time2;
    
    ticket_dur_t abs1 = 60 * hour1 + minute1, abs2 = 60 * hour2 + minute2; 
    return abs1 > abs2 ? abs1 - abs2 : abs2 - abs1;    
}

using counter_t = uint32_t;
enum { _tramlines = 0, _tickets = 1, _counter = 2 };
using state_t = tuple<tramlines_t, tickets_t, counter_t>;

using line_index_t = size_t;
enum { _line_index = 0, _content = 1 };

using line_t = tuple<line_index_t, string>;
void raise_error(const line_t& line) {
    const auto& [line_index, content] = line;
    cerr << line_index << ' ' << content << '\n';
}

void process_tramline_addition(const line_t& line, tramlines_t& tramlines) {
    const auto& [_1, content] = line;
    stringstream ss {};
    ss << content;

    tramline_id_t id; ss >> id;
    if (tramlines.find(id) != tramlines.end()) {
        raise_error(line);
    }
    else {
        tramline_t tram_line;
        auto prior_time = optional<stop_time_t>();

        for (size_t idx = 0; ss.rdbuf()->in_avail() > 0; ++idx) {
            string time_str; ss >> time_str;
            stop_time_t stop_time = stop_time_from_string(time_str);
            stop_id_t stop_id; ss >> stop_id;

            if (!is_stop_time_valid(stop_time) ||
                (prior_time.has_value() && prior_time.value() <= stop_time) ||
                tram_line.find(stop_id) != tram_line.end()) {
                raise_error(line);
                return;
            }

            prior_time.value() = stop_time;
            tram_line[stop_id] = { idx, stop_time };
        }

        tramlines[id] = tram_line;
    }
}

void process_ticket_addition(const smatch& m, const line_t& line, tickets_t& tickets) {
    ticket_id_t id = m[1].str();

    string price_str = m[2].str();
    price_str.erase(remove(price_str.begin(), price_str.end(), '.'), price_str.end());
    ticket_price_t price;
    stringstream ss {};
    ss << price_str; ss >> price;
    
    ticket_dur_t dur;
    ss = {};
    ss << m[3].str(); ss >> dur;

    auto& [ids, price_map] = tickets;

    if (ids.find(id) != ids.end()) {
        raise_error(line);
    }
    else {
        if (price_map.find(price) != price_map.end()) {
            auto& cur = price_map.at(price);
            auto& [cur_dur, _1] = cur;
            if (cur_dur < dur) cur = { dur, id };
        }
        else {
            price_map[price] = { dur, id };
        }
    }
}

enum { _chosen_prices = 0, _total_price = 1, _total_dur = 2 };
using branch_state_t = tuple<vector<ticket_price_t>, ticket_price_t, ticket_dur_t>;

bool operator<(const branch_state_t& lhs, const branch_state_t& rhs) {
    const auto& [_1, lhs_price, _2] = lhs;
    const auto& [_3, rhs_price, _4] = rhs;
    return lhs_price < rhs_price;
}

void find_ticket_set(const ticket_dur_t& dur, const price_map_t& price_map, counter_t& counter) {
    auto optimal = optional<branch_state_t>();
    
    using branch_off_t = function<void(const branch_state_t&)>;
    branch_off_t branch_off = [&](const branch_state_t& branch) {
        const auto& [chosen_prices, total_price, total_dur] = branch;
        
        if (chosen_prices.size() >= 3 || total_dur >= dur) {
            if (total_dur >= dur) {
                if (optimal.has_value()) optimal.value() = min(optimal.value(), branch);
                else optimal.value() = branch;
            }         
        }        
        else {
            auto cost_bound = !chosen_prices.empty() ? chosen_prices.back() 
                                                     : numeric_limits<ticket_price_t>::max();
            if (optimal.has_value()) {
                const auto& [_1, opt_price, _2] = optimal.value();
                cost_bound = min(cost_bound, opt_price - total_price);
            }

            for (auto it = price_map.begin(); it != price_map.upper_bound(cost_bound); ++it) {
                const auto& price = it->first;
                const auto& [dur, _1] = it->second;

                auto new_chosen_prices = chosen_prices;
                new_chosen_prices.push_back(price);
                branch_off({ new_chosen_prices, total_price + price, total_dur + dur });
            }
        }
    };

    branch_off({ vector<ticket_price_t>(), 0, 0 });

    if (optimal.has_value()) {
        const auto& [opt_prices, _1, _2] = optimal.value();
        cout << '!';
        for (size_t i = 0; i < opt_prices.size(); ++i) {
            cout << opt_prices[i];
            if (i != opt_prices.size()-1) {
                cout << "; ";
            }
        }
        cout << '\n';

        counter += opt_prices.size();
    }
    else {
        cout << ":-|" << '\n';
    }
}

void process_ticket_query(const line_t& line, state_t& state) {
    auto& [tramlines, tickets, counter] = state;
    auto& [_1, price_map] = tickets;
    const auto& [_2, content] = line;

    stringstream ss {};
    ss << content;
    { string _; ss >> _; };

    vector<stop_id_t> stops_seq;
    vector<tramline_id_t> lines_seq;
    
    stop_id_t first_stop; ss >> first_stop;
    stops_seq.push_back(first_stop);

    while (ss.rdbuf()->in_avail() > 0) {
        tramline_id_t tramline_id; ss >> tramline_id;
        lines_seq.push_back(tramline_id);

        stop_id_t stop_id; ss >> stop_id;
        stops_seq.push_back(stop_id);
    }

    auto last_arrival_time = optional<stop_time_t>(),
         first_departure_time = optional<stop_time_t>();
    bool stops_without_the_line = false, 
         arrival_after_departure = true;
    auto arrival_before_departure = optional<stop_id_t>();

    for (size_t i = 0; i < lines_seq.size(); ++i) {
        const auto &start_stop = stops_seq[i], &end_stop = stops_seq[i+1];
        const auto& tramline = tramlines[lines_seq[i]];

        stops_without_the_line |= tramline.find(start_stop) == tramline.end() 
                               || tramline.find(end_stop) == tramline.end();
        const auto& start = tramline.at(start_stop), end = tramline.at(end_stop);
        const auto& [start_seq_idx, start_time] = start;
        const auto& [end_seq_idx, end_time] = end;

        stops_without_the_line |= start_seq_idx + 1 != end_seq_idx;

        if (!first_departure_time.has_value()) {
            first_departure_time.value() = start_time;
        }

        if (last_arrival_time.has_value()) {
            const auto& last_arrival = last_arrival_time.value();
            arrival_after_departure |= last_arrival > start_time;
            if (last_arrival > start_time && !arrival_before_departure.has_value()) {
                arrival_before_departure.value() = start_stop;
            }
        }

        last_arrival_time.value() = end_time;
    }

    if (stops_without_the_line || arrival_after_departure) {
        cout << ":-|" << '\n';
    }
    else if (arrival_before_departure.has_value()) {
        cout << ":-( " << arrival_before_departure.value() << '\n';
    }
    else {
        auto dur = stop_time_diff(first_departure_time.value(), last_arrival_time.value());
        find_ticket_set(dur, price_map, counter);
    }
}

void process_line(const line_t& line, state_t& state) {
    static const auto line_addition_re = 
        regex("[0-9]+(?:\\ +[0-9]+:[0-9]+\\ +[a-zA-Z_\\^]+)");
    static const auto ticket_addition_re =
        regex("([a-zA-Z\\ ]+?)\\ +([0-9]*\\.[0-9][0-9])\\ +([1-9][0-9]*)");
    static const auto ticket_query_re =
        regex("\\?\\ [a-zA-Z_\\^]+(?:\\ +[0-9]+\\ +[a-zA-Z_\\^]+)+");

    const auto& [_1, content] = line;
    auto& [tramlines, tickets, _2] = state;

    smatch m;
    if (regex_match(content, m, line_addition_re)) {
        process_tramline_addition(line, tramlines);
    }
    else if (regex_match(content, m, ticket_addition_re)) {
        process_ticket_addition(m, line, tickets);
    }
    else if (regex_match(content, m, ticket_query_re)) {
        process_ticket_query(line, state);
    }
    else if (!content.empty()) {
        raise_error(line);
    }
}

int main() {
    string line;
    state_t state;
    
    for (line_index_t idx = 1; getline(cin, line); ++idx) {
        process_line({idx, line}, state);
    }

    return EXIT_SUCCESS;
}