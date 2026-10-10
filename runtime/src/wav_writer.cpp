#include "runtime/wav_writer.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace nes::runtime {
namespace {
void le16(std::ofstream& file, uint16_t value) {
    const char bytes[] = {static_cast<char>(value), static_cast<char>(value >> 8)};
    file.write(bytes, 2);
}
void le32(std::ofstream& file, uint32_t value) {
    const char bytes[] = {static_cast<char>(value), static_cast<char>(value >> 8),
        static_cast<char>(value >> 16), static_cast<char>(value >> 24)};
    file.write(bytes, 4);
}
}
WavWriter::WavWriter(const std::string& path, uint32_t sample_rate) : file_(path, std::ios::binary), rate_(sample_rate) {
    if (!file_ || !rate_ || rate_ > UINT32_MAX / 2u) throw std::runtime_error("Cannot create WAV output at the requested sample rate");
    header(0);
}
WavWriter::~WavWriter() { try { finish(); } catch (...) {} }
void WavWriter::header(uint32_t size) {
    file_.write("RIFF", 4); le32(file_, size + 36u); file_.write("WAVEfmt ", 8);
    le32(file_, 16); le16(file_, 1); le16(file_, 1); le32(file_, rate_);
    le32(file_, rate_ * 2u); le16(file_, 2); le16(file_, 16);
    file_.write("data", 4); le32(file_, size);
}
void WavWriter::append(const std::vector<float>& samples) {
    if (finished_) throw std::logic_error("WAV already finalized");
    if (samples.size() > (UINT32_MAX - 36u) / 2u - sample_count_) throw std::length_error("WAV exceeds the RIFF size limit");
    for (float sample : samples) {
        if (!std::isfinite(sample)) throw std::invalid_argument("Non-finite audio sample");
        const auto pcm = static_cast<int16_t>(std::lround(std::clamp(static_cast<double>(sample), -1.0, 1.0) * 32767.0));
        le16(file_, static_cast<uint16_t>(pcm));
    }
    sample_count_ += samples.size();
    if (!file_) throw std::runtime_error("Cannot write WAV samples");
}
void WavWriter::finish() {
    if (finished_) return;
    file_.seekp(0); header(static_cast<uint32_t>(sample_count_ * 2u)); file_.flush();
    if (!file_) throw std::runtime_error("Cannot finalize WAV output");
    finished_ = true;
}
}
