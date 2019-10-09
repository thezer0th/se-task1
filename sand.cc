#include <iostream>
#include <regex>
using namespace std;

int main() {
    auto str = string("123 8:43 A 9:11 B 10:24 C 19:59 D");
    auto re = regex(R"([0-9]+(?:\ +[0-9]+:[0-9]+\ +[a-zA-Z_\^]+)+)");

    // auto str = string("123 8:43 A");
    // auto re = regex(R"([0-9]+(?:\ +)+)");

    smatch m;
    cout << regex_match(str, m, re) << '\n';

    return EXIT_SUCCESS;
}