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
using namespace std;

using ticket_office_exn = invalid_argument;

using clock_nat_t = uint16_t;
using stop_time_t = tuple<clock_nat_t, clock_nat_t>;

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

stop_time_t stop_time_from_string(const string& str) {    
    static const auto stop_time_re = regex("([0-9]+):([0-9][0-9])");
    smatch m; 

    if (!regex_match(str, m, stop_time_re)) {
        throw ticket_office_exn("provided str not a valid stop time str");
    }

    stop_time_t stop_time;
    auto& [hour, minute] = stop_time;

    stringstream ss {};
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

using stop_id_t = string;
using seq_index_t = size_t;
using tramline_t = unordered_map<stop_id_t, tuple<seq_index_t, stop_time_t>>;

using tramline_id_t = int;
using tramlines_t = unordered_map<tramline_id_t, tramline_t>;

using ticket_price_t = uint64_t;
using ticket_id_t = string;
using ticket_dur_t = uint64_t;
using price_map_t = map<ticket_price_t, tuple<ticket_dur_t, ticket_id_t>>;
using tickets_t = tuple<unordered_set<ticket_id_t>, price_map_t>;

ticket_dur_t stop_time_diff(const stop_time_t& time1, const stop_time_t& time2) {
    const auto& [hour1, minute1] = time1;
    const auto& [hour2, minute2] = time2;
    
    ticket_dur_t abs1 = 60 * hour1 + minute1, abs2 = 60 * hour2 + minute2;
    return abs1 > abs2 ? abs1 - abs2 : abs2 - abs1;
}

using counter_t = uint32_t;
using state_t = tuple<tramlines_t, tickets_t, counter_t>;

void process_tramline_addition(const string& line, tramlines_t& tramlines) {
    stringstream ss {};
    ss << line;

    tramline_id_t id; ss >> id;
    if (tramlines.find(id) != tramlines.end()) {
        throw ticket_office_exn("tramline with specified id already exists");
    }
    else {
        tramline_t tram_line;
        auto prior_time = optional<stop_time_t>();

        for (size_t idx = 0; ss.rdbuf()->in_avail() > 0; ++idx) {
            string time_str; ss >> time_str;
            stop_time_t stop_time = stop_time_from_string(time_str);
            stop_id_t stop_id; ss >> stop_id;

            if (prior_time.has_value() && prior_time.value() >= stop_time) {
                throw ticket_office_exn("stops not in causal order");
            }
            if (tram_line.find(stop_id) != tram_line.end()) {
                throw ticket_office_exn("some stop repeated");
            }

            prior_time = make_optional(stop_time);
            tram_line[stop_id] = {idx, stop_time};
        }

        tramlines[id] = tram_line;
    }
}

void process_ticket_addition(const smatch& m, tickets_t& tickets) {
    ticket_id_t id = m[1].str();

    string price_str = m[2].str();
    price_str.erase(remove(price_str.begin(), price_str.end(), '.'), price_str.end());
    ticket_price_t price;
    stringstream ss {};
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
            auto&[cur_dur, _1] = cur;
            if (cur_dur < dur) cur = {dur, id};
        } else {
            price_map[price] = {dur, id};
        }
    }
}

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
                else optimal = make_optional(branch);
            }         
        }        
        else {
            auto cost_bound = !chosen_prices.empty() ? chosen_prices.back() 
                                                     : numeric_limits<ticket_price_t>::max();
            if (optimal.has_value()) {
                const auto& [_1, opt_total, _2] = optimal.value();
                cost_bound = min(cost_bound, opt_total > total_price ? opt_total - total_price: 0);
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
        cout << "! ";
        for (size_t i = 0; i < opt_prices.size(); ++i) {
            const auto& [_1, id] = price_map.at(opt_prices[i]);
            cout << id;
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

void process_ticket_query(const string& line, state_t& state) {
    auto& [tramlines, tickets, counter] = state;
    auto& [_1, price_map] = tickets;

    stringstream ss {};
    ss << line;
    { string question_mark; ss >> question_mark; }

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
    auto improper_stop_seq = false;
    auto arrival_before_departure = optional<stop_id_t>();

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
            first_departure_time = make_optional(start_time);
        }

        if (last_arrival_time.has_value()) {
            const auto& last_arrival = last_arrival_time.value();
            improper_stop_seq |= last_arrival > start_time;
            if (last_arrival > start_time && !arrival_before_departure.has_value()) {
                arrival_before_departure = make_optional(start_stop);
            }
        }

        last_arrival_time = make_optional(end_time);
    }

    if (improper_stop_seq) {
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

void process_line(const string& line, state_t& state) {
    static const auto line_addition_re = 
        regex(R"([0-9]+(?:\ [0-9]+:[0-9][0-9]\ [a-zA-Z_\^]+)+)");
    static const auto ticket_addition_re =
        regex(R"(([a-zA-Z\ ]+?)\ ([0-9]*\.[0-9][0-9])\ ([1-9][0-9]*))");
    static const auto ticket_query_re =
        regex(R"(\?\ [a-zA-Z_\^]+(?:\ [0-9]+\ [a-zA-Z_\^]+)+)");

    auto& [tramlines, tickets, _1] = state;

    auto m = smatch();
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

int main() {
    string line;
    state_t state;

    auto in_file = fstream("tests/example.in", ios::in);
    cin.rdbuf(in_file.rdbuf());

    auto out_stream = stringstream {};
    auto cout_copy = cout.rdbuf();
    cout.rdbuf(out_stream.rdbuf());

    auto err_stream = stringstream {};
    auto cerr_copy = cout.rdbuf();
    cerr.rdbuf(err_stream.rdbuf());

    for (size_t idx = 1; getline(cin, line); ++idx) {
        try {
            process_line(line, state);
        }
        catch (const ticket_office_exn& _) {
            cerr << "Error in line " << idx << ": " << line << '\n';
        }
    }

    auto& [_1, _2, counter] = state;
    cout << counter << '\n';

    auto out_file = fstream("tests/example.out", ios::in);
    assert(equal(istream_iterator<char>(out_file), istream_iterator<char>(), istream_iterator<char>(out_stream)));
    cout.rdbuf(cout_copy);

    auto err_file = fstream("tests/example.err", ios::in);
    assert(equal(istream_iterator<char>(err_file), istream_iterator<char>(), istream_iterator<char>(err_stream)));
    cerr.rdbuf(cerr_copy);

    return EXIT_SUCCESS;
}