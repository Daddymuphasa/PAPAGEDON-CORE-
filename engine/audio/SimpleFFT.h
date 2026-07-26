#pragma once

#include <vector>
#include <complex>
#include <cmath>

namespace papagedon::audio {

/// A lightweight, permissive Radix-2 Cooley-Tukey FFT implementation.
/// Designed for real-time audio analysis without external dependencies.
class SimpleFFT final {
public:
    explicit SimpleFFT(size_t size) : size_(size) {
        // Size must be a power of 2
        size_t n = 1;
        while (n < size_) n <<= 1;
        size_ = n;

        twiddles_.resize(size_ / 2);
        const double pi = 3.14159265358979323846;
        for (size_t i = 0; i < size_ / 2; ++i) {
            double angle = -2.0 * pi * i / size_;
            twiddles_[i] = std::complex<float>(static_cast<float>(std::cos(angle)), static_cast<float>(std::sin(angle)));
        }

        bitReverse_.resize(size_);
        size_t log2n = 0;
        while ((1ULL << log2n) < size_) log2n++;
        for (size_t i = 0; i < size_; ++i) {
            size_t rev = 0;
            for (size_t j = 0; j < log2n; ++j) {
                if ((i >> j) & 1) rev |= (1ULL << (log2n - 1 - j));
            }
            bitReverse_[i] = rev;
        }
    }

    void Execute(const std::vector<float>& input, std::vector<std::complex<float>>& output) const {
        output.resize(size_);
        for (size_t i = 0; i < size_; ++i) {
            output[bitReverse_[i]] = (i < input.size()) ? std::complex<float>(input[i], 0.0f) : std::complex<float>(0.0f, 0.0f);
        }

        for (size_t len = 2; len <= size_; len <<= 1) {
            size_t halfLen = len >> 1;
            size_t step = size_ / len;
            for (size_t i = 0; i < size_; i += len) {
                for (size_t j = 0; j < halfLen; ++j) {
                    std::complex<float> u = output[i + j];
                    std::complex<float> v = output[i + j + halfLen] * twiddles_[j * step];
                    output[i + j] = u + v;
                    output[i + j + halfLen] = u - v;
                }
            }
        }
    }

    size_t Size() const { return size_; }

private:
    size_t size_;
    std::vector<std::complex<float>> twiddles_;
    std::vector<size_t> bitReverse_;
};

} // namespace papagedon::audio
