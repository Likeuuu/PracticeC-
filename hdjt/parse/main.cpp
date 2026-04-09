#include "parser.h"
#include <iostream>

int main(int argc, char** argv) {
    std::string filepath ;
    if (argc > 1) filepath = argv[1];
    else return 0;

    hdjt::Parser parser;
    hdjt::Circuit_t c = parser.parse(filepath);

    std::cout << "INPUT: ";
    for (auto& i : c.inputs) std::cout << i << " ";
    std::cout << "\n";

    std::cout << "OUTPUT: ";
    for (auto& o : c.outputs) std::cout << o << " ";
    std::cout << "\n";

    for (auto& g : c.gates) {
        std::cout << "GATE: " << g.type << " " << g.name << " (";
        for (size_t i = 0; i < g.inputs.size(); ++i) {
            std::cout << g.inputs[i];
            if (i != g.inputs.size() - 1) std::cout << ", ";
        }
        std::cout << ") -> " << g.output << "\n";
    }

    return 0;
}
