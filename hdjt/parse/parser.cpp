#include "parser.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace hdjt
{
Circuit_t  Parser::parse(const std::string& filepath)
{
    Circuit_t  circuit;

    std::ifstream file(filepath);
    std::string line;

    while (std::getline(file, line)) 
    {
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "INPUT")
        {
            while (iss >> token)
            {
                circuit.inputs.push_back(token);
            }
        }
        else if (token == "OUTPUT")
        {
            while (iss >> token)
            {
                circuit.outputs.push_back(token);
            }
        }
        else
        {
            Gate_t gate;
            gate.type = token;
            iss >>  gate.name;
            while (iss >> token)
            {
                gate.inputs.push_back(token);
            }

            if (!gate.inputs.empty())
            {
                gate.output = gate.inputs.back();
                gate.inputs.pop_back();
            }

            circuit.gates.push_back(gate);
        }
    }

    return circuit;
}
} // namespace hdjt
