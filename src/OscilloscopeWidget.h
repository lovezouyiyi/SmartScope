#pragma once

#include <QWidget>
#include <QVector>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QTimer>

// Forward declarations for QtCharts classes
class QChart;
class QChartView;
class QLineSeries;
class QValueAxis;

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

class OscilloscopeWidget : public QWidget {
    Q_OBJECT

public:
    explicit OscilloscopeWidget(QWidget* parent = nullptr);
    ~OscilloscopeWidget();

    void setChannelCount(int count);
    void setChannelEnabled(int channel, bool enabled);
    void setSeriesVisible(int channel, bool voltageVisible, bool currentVisible);

    void updateVoltage(int channel, int milliVolt);
    void updateCurrent(int channel, int milliAmp);

    void setChannelPowered(int channel, bool powered);

signals:
    void channelPowerToggled(int channel, bool checked);

private slots:
    void toggleChannel(int channelIdx);
    void toggleCurve(int channelIdx, const QString& dataType);

private:
    struct ChannelWidgets {
        QPushButton* button = nullptr;
        QCheckBox* voltageCheck = nullptr;
        QCheckBox* currentCheck = nullptr;
        QChartView* chartView = nullptr;
        QLineSeries* voltageSeries = nullptr;
        QLineSeries* currentSeries = nullptr;
        QValueAxis* axisX = nullptr;
        QValueAxis* axisY = nullptr;
    };

    void setupChannelUI(int channelIdx);
    void updateChartData(int channelIdx);

    static constexpr int kMaxChannels = 4;
    static constexpr int kMaxSamples = 100;

    QVector<ChannelWidgets> channelWidgets_;
    QVector<QVector<double>> voltageData_;
    QVector<QVector<double>> currentData_;
    QVector<bool> channelEnabled_;
    int channelCount_ = 4;
};
