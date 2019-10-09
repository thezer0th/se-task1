#include <iostream>
#include <regex>
#include <string>
using namespace std;

int main() {
    auto re = regex("([a-zA-Z]+)(\\ [a-zA-Z]+)*");
    string str; getline(cin, str);

    smatch m;
    if (regex_match(str, m, re)) {
        for (size_t i = 0; i < m.size(); ++i) {
            cout << '"' << m[i].str() << '"' << '\n';
        }
    }
    return 0;
}