#pragma once

#include <array>
#include <deque>
#include <mutex>
#include <optional>
#include <vector>

struct ChannelSample {
    int voltageMilliV = 0;
    int currentMilliA = 0;
};

class DataEngine {
public:
    static constexpr int kChannelCount = 4;
    static constexpr int kMaxSamples = 120;

    void push(int channel, int voltageMilliV, int currentMilliA);
    std::vector<ChannelSample> samples(int channel) const;
    std::optional<ChannelSample> latest(int channel) const;

private:
    int indexFromChannel(int channel) const;

    mutable std::mutex mutex_;
    std::array<std::deque<ChannelSample>, kChannelCount> buffers_;
};
