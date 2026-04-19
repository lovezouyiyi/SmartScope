#pragma once

#include <QWidget>
#include <QVector>
#include <QPaintEvent>

class OscilloscopeWidget : public QWidget {
    Q_OBJECT

public:
    explicit OscilloscopeWidget(QWidget* parent = nullptr);

    void setChannelCount(int count);
    void setChannelEnabled(int channel, bool enabled);
    void setSeriesVisible(int channel, bool voltageVisible, bool currentVisible);

    void updateVoltage(int channel, int milliVolt);
    void updateCurrent(int channel, int milliAmp);

    void setChannelPowered(int channel, bool powered);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void drawChannel(QPainter& p, int channel, const QRect& rect);

    static constexpr int kMaxChannels = 4;
    static constexpr int kMaxSamples = 100;
    static constexpr int kMaxMilliUnit = 5500;

    struct ChannelData {
        QVector<int> voltageHistory;
        QVector<int> currentHistory;
        bool voltageVisible = true;
        bool currentVisible = true;
        bool powered = false;
        bool enabled = true;
    };

    QVector<ChannelData> channels_;
    int channelCount_ = 4;
};
