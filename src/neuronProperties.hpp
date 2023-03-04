#include <cstdint>
#include <fstream>

struct NeuronProperties {
    bool fired;
    float firedFraction;
    float electricActivity;
    float secondaryVariable;
    float calcium;
    float targetCalcium;
    float synapticInput;
    float backgroundActivity;
    float grownAxons;
    uint32_t connectedAxons;
    float grownDendrites;
    uint32_t connectedDendrites;

    static NeuronProperties Parse(std::ifstream& stream) {
        NeuronProperties result{};

        constexpr auto all = std::numeric_limits<std::streamsize>::max();

        int64_t timestep;
        stream >> timestep;
        stream.ignore(all, ';');
        stream >> result.fired;
        stream.ignore(all, ';');
        stream >> result.firedFraction;
        stream.ignore(all, ';');
        stream >> result.electricActivity;
        stream.ignore(all, ';');
        stream >> result.secondaryVariable;
        stream.ignore(all, ';');
        stream >> result.calcium;
        stream.ignore(all, ';');
        stream >> result.targetCalcium;
        stream.ignore(all, ';');
        stream >> result.synapticInput;
        stream.ignore(all, ';');
        stream >> result.backgroundActivity;
        stream.ignore(all, ';');
        stream >> result.grownAxons;
        stream.ignore(all, ';');
        stream >> result.connectedAxons;
        stream.ignore(all, ';');
        stream >> result.grownDendrites;
        stream.ignore(all, ';');
        stream >> result.connectedDendrites;
        stream.ignore(all, '\n');
        return result;
    }
};