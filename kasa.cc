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
#include <variant>

// An alias for all subsequent exceptions related to the operation of ticket office.
using ticket_office_exn = std::invalid_argument;

// A representation of clock time: hour/minute numbers.
using clock_nat_t = uint16_t;
enum { _hour = 0, _minute = 1 };
using stop_time_t = std::tuple<clock_nat_t, clock_nat_t>;

// C-style comparison operator for clock time
int compare_stop_time(const stop_time_t& lhs, const stop_time_t& rhs) noexcept {
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

// An usual sequence of operator overloads for comparison

bool operator<=(const stop_time_t& lhs, const stop_time_t& rhs) noexcept {
    return compare_stop_time(lhs, rhs) <= 0;
}

bool operator>=(const stop_time_t& lhs, const stop_time_t& rhs) noexcept {
    return compare_stop_time(lhs, rhs) >= 0;
}

bool operator<(const stop_time_t& lhs, const stop_time_t& rhs) noexcept {
    return compare_stop_time(lhs, rhs) < 0;
}

bool operator>(const stop_time_t& lhs, const stop_time_t& rhs) noexcept {
    return compare_stop_time(lhs, rhs) > 0;
}

bool operator==(const stop_time_t& lhs, const stop_time_t& rhs) noexcept {
    return compare_stop_time(lhs, rhs) == 0;
}

bool operator!=(const stop_time_t& lhs, const stop_time_t& rhs) noexcept {
    return compare_stop_time(lhs, rhs) != 0;
}

// Convert string to the clock time, provided the string is in correct format (otherwise raise an exception)
stop_time_t stop_time_from_string(const std::string& str) {
    // Prepare re for stop time parsing (notice two capture groups).
    static const auto stop_time_re = std::regex("([0-9]+):([0-9][0-9])");
    std::smatch m;

    if (!regex_match(str, m, stop_time_re)) {
        throw ticket_office_exn("provided str not a valid stop time str");
    }

    // Prepare final structure.
    stop_time_t stop_time {};
    auto& [hour, minute] = stop_time;

    // Retrieve appropriate captured strings and parse into hour/minute numbers using stringstream.
    std::stringstream ss {};
    ss << m[1].str();
    ss >> hour;
    ss = {};
    ss << m[2].str();
    ss >> minute;

    // Verify whether the numbers are in proper range.
    if (!(hour <= 23 && minute <= 59)) {
        throw ticket_office_exn("number fields in str are invalid");
    }

    // Check whether stop time is correct (i.e. between 5:55 and 21:21)
    static const auto start = stop_time_t(5, 55), end = stop_time_t(21, 21);
    if (!(start <= stop_time && stop_time <= end)) {
        throw ticket_office_exn("provided stop time is outside of operation range");
    }

    return stop_time;
}

// A representation of tramline: map from stop ids to <index, stop time> tuples
using stop_id_t = std::string;
using seq_index_t = size_t;
enum { _seq_index = 0, _stop_time = 1 };
using tramline_t = std::unordered_map<stop_id_t, std::tuple<seq_index_t, stop_time_t>>;

// A representation of tramlines container: map from tramline ids to tramline objects proper
using tramline_id_t = uint64_t;
using tramlines_t = std::unordered_map<tramline_id_t, tramline_t>;

// A representation of tickets container: set of id's (for uniqueness tracking) and map from prices to <dur, id> tuples.
using ticket_price_t = uint64_t;
using ticket_id_t = std::string;
using ticket_dur_t = uint64_t;
enum { _ticket_dur = 0, _ticket_id = 1 };
using price_map_t = std::map<ticket_price_t, std::tuple<ticket_dur_t, ticket_id_t>>;
enum { _ticket_ids = 0, _price_map = 1 };
using tickets_t = std::tuple<std::unordered_set<ticket_id_t>, price_map_t>;

// Time difference between two stop times.
ticket_dur_t stop_time_diff(const stop_time_t& time1, const stop_time_t& time2) noexcept {
    const auto& [hour1, minute1] = time1;
    const auto& [hour2, minute2] = time2;
    
    ticket_dur_t abs1 = 60 * hour1 + minute1, abs2 = 60 * hour2 + minute2;
    return abs1 > abs2 ? abs1 - abs2 : abs2 - abs1;
}

using counter_t = uint32_t;
enum { _tramlines = 0, _tickets = 1, _counter = 2 };
using state_t = std::tuple<tramlines_t, tickets_t, counter_t>;

// Add tramline as encoded in line to the tramlines container.
void process_tramline_addition(const std::string& line, tramlines_t& tramlines) {
    // We shall use stringstream for token parsing.
    std::stringstream ss {};
    ss << line;

    // Retrieve id and verify if it's unique.
    tramline_id_t id;
    ss >> id;
    if (tramlines.find(id) != tramlines.end()) {
        throw ticket_office_exn("tramline with specified id already exists");
    }

    // Retrieve subsequent line stops.
    tramline_t tram_line = {};
    auto prior_time = std::optional<stop_time_t>();

    for (size_t idx = 0; ss.rdbuf()->in_avail() > 0; ++idx) {
        // Retrieve stop time and stop id.
        std::string time_str;
        ss >> time_str;
        stop_time_t stop_time = stop_time_from_string(time_str);
        stop_id_t stop_id;
        ss >> stop_id;

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

// Add a new ticket to tickets container, given the appropriate match.
void process_ticket_addition(const std::smatch& m, tickets_t& tickets) {
    // Retrieve id (no further processing required).
    ticket_id_t id = m[1].str();

    // Retrieve price, using stringstream.
    std::string price_str = m[2].str();
    // Following line removes the dot from price as to allow us to store it in an integral form.
    price_str.erase(std::remove(price_str.begin(), price_str.end(), '.'), price_str.end());
    ticket_price_t price;
    std::stringstream ss {};
    ss << price_str;
    ss >> price;

    // Retrieve ticket duration.
    ticket_dur_t dur;
    ss = {};
    ss << m[3].str();
    ss >> dur;

    auto& [ids, price_map] = tickets;

    if (ids.find(id) != ids.end()) {
        throw ticket_office_exn("ticket with specified id already exists");
    } else {
        // Since the structure of our container has prices as keys, we must distinguish (non-)presence.
        if (price_map.find(price) != price_map.end()) {
            auto &cur = price_map.at(price);
            auto& cur_dur = std::get<_ticket_dur>(cur);
            // If ticket's duration is greater, replace the prior one.
            if (cur_dur < dur) cur = {dur, id};
        } else {
            price_map[price] = {dur, id};
        }
    }
}

// A container for storing current branch in the following branch-and-bound algorithm; a sequence of prices, total price
// and total duration.
enum { _prices_vector = 0, _total_price = 1, _total_dur = 2 };
using branch_state_t = std::tuple<std::vector<ticket_price_t>, ticket_price_t, ticket_dur_t>;

// We shall consider an order over branches, with the minimal ones being the most optimal (in sense of minimal price).
bool operator<(const branch_state_t& lhs, const branch_state_t& rhs) {
    return std::get<_total_price>(lhs) < std::get<_total_price>(rhs);
}

// A branch-and-bound algorithm for finding <=3 tickets with sufficient total duration and an optimal price.
std::optional<branch_state_t> find_ticket_set(const ticket_dur_t& dur, const price_map_t& price_map) {
    // A (yet uninitialized) optimal ticket set/sequence.
    auto optimal = std::optional<branch_state_t>();

    // A subroutine for branching off the specific branch.
    using branch_off_t = std::function<void(const branch_state_t&)>;
    // The choice of lambda is merely to avoid passing superfluous arguments everywhere.
    branch_off_t branch_off = [&](const branch_state_t& branch) {
        const auto& [chosen_prices, total_price, total_dur] = branch;
        
        if (chosen_prices.size() >= 3 || total_dur >= dur) {
            // In this case we may as well merely verify if the branch is optimal and not branch off further, as no
            // further improvement is possible, since prices are non-negative and necessary conditions are satisfied.
            if (total_dur >= dur) {
                if (optimal.has_value()) optimal.value() = min(optimal.value(), branch);
                else optimal = std::make_optional(branch);
            }
        }        
        else {
            // In this case we shall continue branching off; we shall start with the cost bound for new addition.
            // First of all, as to avoid the repetition, we shall order ticket sequence in order >= of prices.
            auto cost_bound = !chosen_prices.empty() ? chosen_prices.back() 
                                                     : std::numeric_limits<ticket_price_t>::max();
            if (optimal.has_value()) {
                // If there is any current optimal ticket set, we can also bound an addition by it.
                const auto& opt_total = std::get<_total_price>(optimal.value());
                cost_bound = std::min(cost_bound, opt_total > total_price ? opt_total - total_price: 0);
            }

            // In a standard way, we shall enumerate appropriate tickets. Notice that we use upper_bound: indeed, that
            // is the reason why the tickets container was sorted map with prices as keys.
            for (auto it = price_map.begin(); it != price_map.upper_bound(cost_bound); ++it) {
                const auto& ext_price = it->first;
                const auto& ext_dur = std::get<_ticket_dur>(it->second);

                auto new_chosen_prices = chosen_prices;
                new_chosen_prices.push_back(ext_price);
                branch_off({ new_chosen_prices, total_price + ext_price, total_dur + ext_dur });
            }
        }
    };

    // Call the subroutine on the initial object.
    branch_off({ std::vector<ticket_price_t>(), 0, 0 });
    return optimal;
}

std::tuple<std::vector<stop_id_t>, std::vector<tramline_id_t>> extract_query_data(const std::string& line) {
    // We shall use stringstream.
    std::stringstream ss {};
    ss << line;

    { // Here, we wish to remove the initial question mark, but without the pollution of the local scope.
        std::string question_mark;
        ss >> question_mark;
    }

    std::vector<stop_id_t> stops_seq;
    std::vector<tramline_id_t> lines_seq;

    // Extract the initial stop, and then others in a loop, since the tramline ids are interleaved with stop ids.
    stop_id_t first_stop;
    ss >> first_stop;
    stops_seq.push_back(first_stop);

    while (ss.rdbuf()->in_avail() > 0) {
        tramline_id_t tramline_id;
        ss >> tramline_id;
        lines_seq.push_back(tramline_id);

        stop_id_t stop_id;
        ss >> stop_id;
        stops_seq.push_back(stop_id);
    }

    return std::make_tuple(stops_seq, lines_seq);
}

bool verify_if_proper_route(const std::vector<stop_id_t>& stops_seq, const std::vector<tramline_id_t>& lines_seq,
        const tramlines_t& tramlines) {
    auto last_arrival_time = std::optional<stop_time_t>();

    for (size_t i = 0; i < lines_seq.size(); ++i) {
        const auto &start_stop = stops_seq.at(i), &end_stop = stops_seq.at(i+1);
        if (tramlines.find(lines_seq[i]) == tramlines.end()) {
            return false;
        }
        const auto& tramline = tramlines.at(lines_seq[i]);

        // If line does not even contain the specified stops, the seq is improper.
        if (tramline.find(start_stop) == tramline.end() || tramline.find(end_stop) == tramline.end()) {
            return false;
        }

        // Due to previous break, start and end exist (i.e. at will not throw std::out_of_range)
        const auto& start = tramline.at(start_stop), end = tramline.at(end_stop);
        const auto& [start_seq_idx, start_time] = start;
        const auto& [end_seq_idx, end_time] = end;

        // If stops are not in order, the seq is improper.
        if (start_seq_idx >= end_seq_idx) {
            return false;
        }

        if (last_arrival_time.has_value()) {
            const auto& last_arrival = last_arrival_time.value();
            // In this case we arrive too late, so sequence is incorrect.
            if (last_arrival > start_time) {
                return false;
            }
        }
        last_arrival_time = std::make_optional(end_time);
    }

    // After all that, the seq must be proper.
    return true;
}

std::optional<stop_id_t> verify_if_has_to_wait(const std::vector<stop_id_t>& stops_seq,
        const std::vector<tramline_id_t>& lines_seq, const tramlines_t& tramlines) {
    auto last_arrival_time = std::optional<stop_time_t>(std::nullopt);

    for (size_t i = 0; i < lines_seq.size(); ++i) {
        const auto &start_stop = stops_seq[i], &end_stop = stops_seq[i+1];
        const auto& tramline = tramlines.at(lines_seq[i]);

        // We need not worry about std::out_of_range, since the seq is proper
        const auto& start = tramline.at(start_stop), end = tramline.at(end_stop);
        const auto& [start_seq_idx, start_time] = start;
        const auto& [end_seq_idx, end_time] = end;

        if (last_arrival_time.has_value()) {
            // If last arrival time was recorded (i.e. if this is not first iteration of loop), we must check if we
            // would not perhaps have to wait for the next one.
            const auto& last_arrival = last_arrival_time.value();
            if (last_arrival > start_time) {
                // Here, if so befalls, we would have to wait, and thus return the appropriate stop id.
                return std::make_optional<stop_id_t>(start_stop);
            }
        }
        last_arrival_time = std::make_optional(end_time);
    }

    // After all that, the arrivals/departures are sync'd, and so we may return nullopt.
    return std::nullopt;
}

void print_ticket_set(const std::vector<ticket_price_t>& prices_vec, const price_map_t& price_map) {
    std::cout << "! ";
    for (size_t i = 0; i < prices_vec.size(); ++i) {
        const auto& id = std::get<_ticket_id>(price_map.at(prices_vec[i]));
        std::cout << id;
        if (i != prices_vec.size()-1) {
            std::cout << "; ";
        }
    }
    std::cout << '\n';
}

// Process route query.
void process_route_query(const std::string& line, state_t& state) {
    auto& [tramlines, tickets, counter] = state;
    auto& price_map = std::get<_price_map>(tickets);

    auto [stops_seq, lines_seq] = extract_query_data(line);

    // After retrieving the tokens, we must verify the correctness.
    if (!verify_if_proper_route(stops_seq, lines_seq, tramlines)) {
        std::cout << ":-|" << '\n';
        return;
    }

    auto wait_stop_id_opt = verify_if_has_to_wait(stops_seq, lines_seq, tramlines);
    if (wait_stop_id_opt.has_value()) {
        auto& wait_stop_id = wait_stop_id_opt.value();
        std::cout << ":-( " << wait_stop_id << '\n';
    }
    else {
        // At this point the sequence itself is fully correct, and we may try to find appropriate ticket set.
        // Firstly, retrieve the interval of the route, and duration thereof.
        auto first_departure_time = std::get<_stop_time>(tramlines.at(lines_seq.front()).at(stops_seq.front())),
             last_arrival_time = std::get<_stop_time>(tramlines.at(lines_seq.back()).at(stops_seq.back()));
        auto dur = stop_time_diff(first_departure_time, last_arrival_time);

        // Then, find the appropriate ticket set.
        // The +1 is due to recent spec change to closed-closed time intervals, as opposed to closed-open.
        auto ticket_set_opt = find_ticket_set(dur + 1, price_map);
        if (ticket_set_opt.has_value()) {
            auto& prices_vec = std::get<_prices_vector>(ticket_set_opt.value());
            print_ticket_set(prices_vec, price_map);

            // Also, in accordance with spec, we must record total # of sold tickets.
            counter += prices_vec.size();
        }
        else {
            // In such a case, of course, no appropriate ticket set has been found.
            std::cout << ":-|" << '\n';
        }
    }
}

void process_line(const std::string& line, state_t& state) {
    static const auto line_addition_re = 
        std::regex(R"([0-9]+(?:\ [1-9][0-9]*:[0-9][0-9]\ [a-zA-Z_\^]+)+)");
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
        process_route_query(line, state);
    }
    else if (!line.empty()) {
        throw ticket_office_exn("request given in unknown format");
    }
}

int main() {
    std::string line;
    state_t state;

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

    return EXIT_SUCCESS;
}