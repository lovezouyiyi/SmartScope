#pragma once

#include <array>

#include <QWidget>

#include "DataEngine.h"

class OscilloscopeWidget : public QWidget {
public:
    explicit OscilloscopeWidget(DataEngine* engine, QWidget* parent = nullptr);

    void setSeriesVisible(int channel, bool voltageVisible, bool currentVisible);
    void setChannelPowered(int channel, bool powered);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    struct VisibilityState {
        bool voltageVisible = true;
        bool currentVisible = true;
    };

    static constexpr int kChannelCount = 4;
    static constexpr int kMaxMilliUnit = 5500;

    int indexFromChannel(int channel) const;
    int yForValue(int value, const QRect& plotRect) const;
    int xForSampleIndex(int sampleIndex, int sampleCount, const QRect& plotRect) const;

    DataEngine* engine_ = nullptr;
    std::array<VisibilityState, kChannelCount> visibility_{};
    std::array<bool, kChannelCount> powered_{{false, false, false, false}};
};
