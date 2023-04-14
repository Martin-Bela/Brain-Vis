#pragma once

#include <bit>
#include <cstdint>
#include <fstream>
#include <limits>
#include <assert.h>

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

    static NeuronProperties parse(std::ifstream& stream) {
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

    float projection(int i) const {
        switch (i) {
            case 0: return fired;
            case 1: return firedFraction;
            case 2: return electricActivity;
            case 3: return secondaryVariable;
            case 4: return calcium;
            case 5: return targetCalcium;
            case 6: return synapticInput;
            case 7: return backgroundActivity;
            case 8: return grownAxons;
            case 9: return static_cast<float>(connectedAxons);
            case 10: return grownDendrites;
            case 11: return static_cast<float>(connectedDendrites);
        }
        assert(false);
        return 0;
    }
};

static_assert(sizeof(NeuronProperties) == 12 * 4);
static_assert(offsetof(NeuronProperties, firedFraction) == 4);
static_assert(std::endian::native == std::endian::little);
static_assert(std::numeric_limits<float>::is_iec559);