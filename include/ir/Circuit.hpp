#ifndef CIRCUIT_HPP
#define GATE_HPP

#include <vector>
#include "Gate.hpp"

class Circuit {
    public:
    std::vector<GateType> gates;
    std::vector<std::vector<int>> qubits;
};
