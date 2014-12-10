all: otf_parser

otf_parser: otf_parser.o
	g++ -std=c++11 otf_parser.o -o otf_parser

otf_parser.o: otf_parser.cpp
	g++ -c -std=c++11 otf_parser.cpp -o otf_parser.o
