#pragma once

#include <QWidget>
#include <QVector>

#ifdef USE_QT_CHARTS
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>

QT_BEGIN_NAMESPACE
class QChart;
class QChartView;
class QLineSeries;
class QValueAxis;
QT_END_NAMESPACE
#else
#include <QPaintEvent>
#endif

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

#ifdef USE_QT_CHARTS
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
        QLabel* voltageLabel = nullptr;
        QLabel* currentLabel = nullptr;
    };

    void setupChannelUI(int channelIdx);
    void updateChartData(int channelIdx);

    QVector<ChannelWidgets> channelWidgets_;
#else
protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void drawChannel(QPainter& p, int channel, const QRect& rect);
#endif

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
    QVector<bool> channelEnabled_;
    int channelCount_ = 4;
};
