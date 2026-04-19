#pragma once

#include <array>
#include <memory>
#include <optional>

#include <QCheckBox>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QTimer>

#include "DataEngine.h"

class OscilloscopeWidget;
class SmartUSBHub;

class MainWindow : public QMainWindow {
public:
    MainWindow();

private:
    struct ChannelUi {
        QPushButton* powerButton = nullptr;
        QCheckBox* voltageCheck = nullptr;
        QCheckBox* currentCheck = nullptr;
        QLabel* voltageLabel = nullptr;
        QLabel* currentLabel = nullptr;
    };

    static constexpr int kChannelCount = 4;

    void setupUi();
    void setupHub();
    void tryReconnect();
    void pollOneChannel();

    void onPowerToggled(int channel, bool checked);
    void onSeriesVisibilityChanged(int channel);

    void updateConnectionText();
    void updateChannelUiValues(int channel, std::optional<int> power, std::optional<int> voltage, std::optional<int> current);

    DataEngine engine_;
    std::unique_ptr<SmartUSBHub> hub_;

    QWidget* rootWidget_ = nullptr;
    QLabel* connectionLabel_ = nullptr;
    OscilloscopeWidget* oscilloscope_ = nullptr;

    std::array<ChannelUi, kChannelCount> channelUi_{};
    std::array<std::optional<int>, kChannelCount> latestVoltage_{};
    std::array<std::optional<int>, kChannelCount> latestCurrent_{};

    QTimer pollTimer_;
    QTimer reconnectTimer_;
    int pollIndex_ = 0;
};
