#include <iostream>

#include "simple_txt_notation.h"

int main(int argc, char** argv)
{
	if (argc != 2) {
		std::cout << "USAGE: " << argv[0] << " [filename]\n";
		std::cout << "The attributes and their values are printed as follows:\n";
		std::cout << "attr: [attribute], val: [value]";
		return 0;
	}

	auto vals = stn::parse(argv[1]);
	for (const auto& val : vals) {
		std::cout << "attr: " << val.first << ", val: " << val.second << '\n';
	}
}