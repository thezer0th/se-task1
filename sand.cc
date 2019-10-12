#include <iostream>
#include <sstream>
using namespace std;

int main() {
    stringstream ss {};
    ss << "In nova fert animus, mutatas dicere formas corpora.";
    while (ss.rdbuf()->in_avail() > 0) {
        string word; ss >> word;
        cout << "> " << word << '\n';
    }
    return EXIT_SUCCESS;
}