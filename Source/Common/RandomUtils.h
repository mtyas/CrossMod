#pragma once

#include <cstdint>
#include <cmath>
#include <random>

namespace MidiFlux
{

class FastRandom
{
public:
    FastRandom(uint64_t seed = 0x853c49e6748fea9bULL)
    {
        if (seed == 0)
            seed = 0x853c49e6748fea9bULL;
        state = seed;
    }

    void setSeed(uint64_t seed)
    {
        state = (seed == 0) ? 0x853c49e6748fea9bULL : seed;
    }

    // Returns a 64-bit unsigned integer using SplitMix64
    uint64_t nextU64()
    {
        uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }

    // Uniform float in [0.0, 1.0)
    float nextFloat()
    {
        return static_cast<float>(nextU64() >> 40) * (1.0f / 16777216.0f);
    }

    // Uniform float in [minVal, maxVal]
    float nextFloat(float minVal, float maxVal)
    {
        return minVal + nextFloat() * (maxVal - minVal);
    }

    // Uniform integer in [minVal, maxVal] inclusive
    int nextInt(int minVal, int maxVal)
    {
        if (minVal >= maxVal)
            return minVal;
        uint32_t range = static_cast<uint32_t>(maxVal - minVal + 1);
        return minVal + static_cast<int>(nextU64() % range);
    }

    // Bernoulli trial: returns true with probability p [0.0 .. 1.0]
    bool nextBool(float probability)
    {
        if (probability <= 0.0f) return false;
        if (probability >= 1.0f) return true;
        return nextFloat() < probability;
    }

    // Gaussian / normal distribution using Box-Muller transform
    float nextGaussian(float mean = 0.0f, float stddev = 1.0f)
    {
        float u1 = nextFloat();
        float u2 = nextFloat();
        while (u1 <= 1e-7f) // avoid log(0)
            u1 = nextFloat();

        float z0 = std::sqrt(-2.0f * std::log(u1)) * std::cos(6.283185307179586f * u2);
        return mean + z0 * stddev;
    }

    // Brownian step / random walk with decay towards center
    float nextBrownian(float currentValue, float stepSize, float decay = 0.95f, float center = 0.0f)
    {
        float delta = nextFloat(-stepSize, stepSize);
        return (currentValue - center) * decay + center + delta;
    }

private:
    uint64_t state;
};

} // namespace MidiFlux
