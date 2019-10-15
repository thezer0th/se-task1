#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <tuple>
#include <algorithm>
#include <set>
#include <fstream>
#include <cstdlib>

using hour = std::tuple<uint16_t, uint16_t >;
using drive = std::tuple<uint16_t, std::vector<std::tuple<hour, std::string>>>;
using price = std::tuple<int, int, int>;
using ticket = std::tuple<std::string, price, int>;

/* 0 = random input
 * 1 = 50% validation input
 * 2 = 100% validation input
 */
const static int TEST_FILES = 10;
const static int TEST_SIZE = 100;
const static int CORRECT_FORMAT = 1;
const static std::string VALID_STOP_NAME =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "^_";

const static std::string VALID_TICKET_NAME =
        " ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

std::string randomString(std::string charset, const int len) {
	std::string s;
	s.reserve(len);
	for (int i = 0; i < len && i < charset.length(); ++i) {
		s.append(1, charset.at(rand() % charset.length()));
	}

	return s;
}

hour getHour(bool correct_format) {
    hour h;
    if(correct_format) {
        int hour = rand() % 25;
        int minute = rand() % 61;
        h = std::make_tuple(hour, minute);
    } else {
        h = std::make_tuple(rand() % 30, rand() % 30);
    }
    return h;
}

price getPrice(int correct_format) {
    if(correct_format > 0) {
        return std::make_tuple(rand() % 100, rand() % 10, rand() % 10);
    } else {
        return std::make_tuple(rand() % 100000, rand() % 1000, rand() % 1000);
    }
}

std::vector<drive> generateRandomDrives(uint16_t quantity, int max_drive_len, int max_stop_name_len) {
	std::vector<drive> vec;
	vec.reserve(quantity);

	for(int i = 0; i < quantity; i++) {
		uint16_t number = rand() % UINT_FAST8_MAX;
		std::vector<std::tuple<hour, std::string>> content;
		int drive_len = (rand() % (max_drive_len - 1)) + 1;

		for(int j = 0; j < drive_len; j++) {
			hour h = getHour(CORRECT_FORMAT);
			int stop_name_len = rand() % max_stop_name_len;
			content.push_back(std::make_tuple(h, randomString(VALID_STOP_NAME, stop_name_len)));
		}
		vec.push_back(std::make_tuple(number, content));
		content.clear();
	}
	return vec;
}

std::vector<ticket> generateRandomTickets(int quantity, int max_ticket_name_len) {
    std::vector<ticket> vec;
    vec.reserve(quantity);

    for(int i = 0; i < quantity; i++) {
        std::string name = randomString(VALID_TICKET_NAME, (rand() % max_ticket_name_len) + 1);
        price p = getPrice(CORRECT_FORMAT);
        int duration = rand() % 120;
        vec.push_back(std::make_tuple(name, p, duration));
    }
    return vec;
}

std::string hourToString(hour h) {
	std::string s;
	s.append(std::to_string(std::get<0>(h)));
	s.append(":");
	s.append(std::to_string(std::get<1>(h)));
	return s;
}

std::string priceToString(price p) {
    std::string s;
    s.append(std::to_string(std::get<0>(p)));
    s.append(".");
    s.append(std::to_string(std::get<1>(p)));
    s.append(std::to_string(std::get<2>(p)));
    return s;
}

std::string driveToAddDriveString(drive d) {
	std::string s;
	int drive_numb = std::get<0>(d);
	std::vector<std::tuple<hour, std::string>> content = std::get<1>(d);

	s.append(std::to_string(drive_numb));
	for(auto elem : content) {
		s.append(" ");
		s.append(hourToString(std::get<0>(elem)));
		s.append(" ");
		s.append(std::get<1>(elem));
	}
	return s;
}

std::string ticketToNewTicketString(ticket t) {
    std::string s;
    s.append(std::get<0>(t));
    s.append(" ");
    s.append(priceToString(std::get<1>(t)));
    s.append(" ");
    s.append(std::to_string(std::get<2>(t)));
    return s;
}

std::string generateTicketRequest(std::vector<std::string> stop_names, std::vector<uint16_t> drive_numb, size_t max_len_request) {
    std::vector<std::string> stop_names_vec;
    int max_len = std::min(stop_names.size(), std::min(drive_numb.size(), max_len_request));
    std::string s;
    s.append("? ");
    int len = (rand() % max_len - 1) + 1;
    for(int i = 0; i < len; i++) {
        int rand1 = (rand() % max_len - 1) + 1;
        int rand2 = (rand() % max_len - 1) + 1;
        s.append(stop_names.at(rand1));
        s.append(" ");
        s.append(std::to_string(drive_numb.at(rand2)));
        s.append(" ");
    }
    s.append(stop_names.at((rand() % max_len - 1) + 1));
    return s;
}

std::tuple<std::vector<uint16_t >, std::vector<std::string>> extractStopNamesAndDriveLines(std::vector<drive> drives) {
    std::set<std::string> stop_names;
    std::set<uint16_t> drive_lines;
    for(auto drive : drives) {
        drive_lines.insert(std::get<0>(drive));
        for(auto v : std::get<1>(drive)) {
            stop_names.insert(std::get<1>(v));
        }
    }
    std::vector<std::string> stop_names_vec(stop_names.begin(), stop_names.end());
    std::vector<uint16_t> drive_lines_vec(drive_lines.begin(), drive_lines.end());
    return std::make_tuple(drive_lines_vec, stop_names_vec);
}

std::vector<std::string> generateSingleTestLines(int test_size) {
    std::vector<drive> drives;
    std::vector<ticket> tickets;
    std::vector<uint16_t> drive_lines;
    std::vector<std::string> stop_names;

    int quantities[3];
    quantities[0] = (rand() % test_size/5) + test_size/5;
    quantities[1] = (rand() % test_size/5) + test_size/5;
    quantities[2] = test_size - quantities[0] - quantities[1];

    drives = generateRandomDrives(quantities[0], 10, 10);
    tickets = generateRandomTickets(quantities[1], 10);
    std::tuple<std::vector<uint16_t>, std::vector<std::string>> drive_extraction = extractStopNamesAndDriveLines(drives);
    drive_lines = std::get<0>(drive_extraction);
    stop_names = std::get<1>(drive_extraction);

    std::vector<std::string> outputs;
    outputs.reserve(test_size);
    for(int i = 0; i < quantities[0]; ++i) {
        outputs.emplace_back(driveToAddDriveString(drives.at(i)));
    }
    for(int i = 0; i < quantities[1]; ++i) {
        outputs.emplace_back(ticketToNewTicketString(tickets.at(i)));
    }
    for(int i = 0; i < quantities[2]; ++i) {
        outputs.emplace_back(generateTicketRequest(stop_names, drive_lines, 10));
    }
    std::random_shuffle(outputs.begin(), outputs.end());
    return outputs;
}

void writeLinesIntoFile(std::string file_name, std::vector<std::string> lines) {
    std::ofstream file;
    file.open(file_name);
    for(std::string line : lines) {
        file << line << std::endl;
    }
    std::cout << file_name << std::endl;
    file.close();
}

int main() {
    std::srand(std::time(nullptr));
    std::vector<std::string> lines;
    std::string filename;
    std::string shell_command;
    for(int i = 0; i < TEST_FILES; i++) {
        lines = generateSingleTestLines(TEST_SIZE);
        filename = "tests/in"+std::to_string(i);
        writeLinesIntoFile(filename, lines);
        //shell_command = "./kasa < "+filename+" >> tests/out"+std::to_string(i);
        //shell_command = "./kasa < "+filename+" &>> tests/err"+std::to_string(i);
        std::system(shell_command.c_str());
    }

	return 0;
}