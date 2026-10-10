#pragma once
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace nes::runtime {
// Streaming mono16-bitPCM export, independent of the SDL output device.
class WavWriter {
public:
    explicit WavWriter(const std::string& path, uint32_t sample_rate);
    ~WavWriter();
    WavWriter(const WavWriter&) = delete;
    WavWriter& operator=(const WavWriter&) = delete;
    void append(const std::vector<float>& samples);
    void finish();
    uint64_t samples_written() const noexcept { return sample_count_; }
private:
    std::ofstream file_;
    uint32_t rate_;
    uint64_t sample_count_{};
    bool finished_{};
    void header(uint32_t data_size);
};
}
