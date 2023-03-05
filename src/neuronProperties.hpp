#include <bit>
#include <cstdint>
#include <fstream>
#include <limits>

struct NeuronProperties {
    uint8_t fired; // 1 or 0
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

constexpr int x = sizeof(NeuronProperties);

static_assert(sizeof(NeuronProperties) == 12 * 4);
static_assert(offsetof(NeuronProperties, firedFraction) == 4);
static_assert(std::endian::native == std::endian::little);
static_assert(std::numeric_limits<float>::is_iec559);