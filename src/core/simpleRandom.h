/**
 * @file simpleRandom.h
 * @brief Tiny deterministic pseudo-random generator (xorshift32).
 *
 * The scene uses randomness for stars, the skyline and the crowd. A fixed
 * seed means every run produces the *same* image, which matters for
 * checkpoint screenshots and, later, for animation frames that must not
 * flicker from one frame to the next.
 */
#pragma once

#include <cstdint>

class SimpleRandom {
public:
    /** @param seed Any non-zero value; zero is replaced by 1 (xorshift needs a non-zero state). */
    explicit SimpleRandom(std::uint32_t seed) : state(seed != 0u ? seed : 1u) {}

    /** @brief Next raw 32-bit value using Marsaglia's xorshift (shift-xor three times). */
    std::uint32_t nextUInt() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    /** @brief Uniform double in [0, 1). */
    double nextDouble() { return nextUInt() / 4294967296.0; }

    /** @brief Uniform double in [minValue, maxValue). */
    double nextRange(double minValue, double maxValue) {
        return minValue + (maxValue - minValue) * nextDouble();
    }

    /** @brief Uniform integer in [minValue, maxValue] (inclusive). */
    int nextInt(int minValue, int maxValue) {
        const auto span = static_cast<std::uint32_t>(maxValue - minValue + 1);
        return minValue + static_cast<int>(nextUInt() % span);
    }

private:
    std::uint32_t state;
};
