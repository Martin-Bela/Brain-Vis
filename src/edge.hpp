#pragma once
#include <cstdint>
#include <cstddef>
#include <bit>
#include <fstream>

struct Edge {
	uint16_t from;
	uint16_t to;
	uint16_t weight;
};


static_assert(sizeof(Edge) == 3 * sizeof(uint16_t));
static_assert(std::endian::native == std::endian::little);
