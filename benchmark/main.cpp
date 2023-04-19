#include <iostream>

#include "instrumentor.hpp"

int main()
{
	Timer timer("test timer");
	std::cout << "testing timer\n";
}