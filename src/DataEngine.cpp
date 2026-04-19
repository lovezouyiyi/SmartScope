#include "DataEngine.h"

void DataEngine::push(int channel, int voltageMilliV, int currentMilliA) {
    const int index = indexFromChannel(channel);
    if (index < 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto& q = buffers_[index];
    q.push_back(ChannelSample{voltageMilliV, currentMilliA});
    while (static_cast<int>(q.size()) > kMaxSamples) {
        q.pop_front();
    }
}

std::vector<ChannelSample> DataEngine::samples(int channel) const {
    const int index = indexFromChannel(channel);
    if (index < 0) {
        return {};
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const auto& q = buffers_[index];
    return std::vector<ChannelSample>(q.begin(), q.end());
}

std::optional<ChannelSample> DataEngine::latest(int channel) const {
    const int index = indexFromChannel(channel);
    if (index < 0) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const auto& q = buffers_[index];
    if (q.empty()) {
        return std::nullopt;
    }

    return q.back();
}

int DataEngine::indexFromChannel(int channel) const {
    if (channel < 1 || channel > kChannelCount) {
        return -1;
    }
    return channel - 1;
}
