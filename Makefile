kasa: kasa.cc
	g++ -Wall -Wextra -g -std=c++17 kasa.cc -o kasa

regex: regex.cc
	g++ -Wall -Wextra -g -std=c++17 regex.cc -o regex

sand: sand.cc
	g++ -Wall -Wextra -g -std=c++17 sand.cc -o sand

all: kasa regex sand

clean:
	rm -rf kasa regex sand