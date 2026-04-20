#include "OscilloscopeWidget.h"

#ifdef USE_QT_CHARTS
// Qt Charts 实现
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QPen>
#include <QDebug>
//sa'sasasa


QT_CHARTS_USE_NAMESPACE

OscilloscopeWidget::OscilloscopeWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumSize(900, 600);
    channelCount_ = kMaxChannels;
    
    // Initialize data arrays
    channels_.resize(kMaxChannels);
    channelEnabled_.resize(kMaxChannels, true);
    channelWidgets_.resize(kMaxChannels);
    
    for (int i = 0; i < kMaxChannels; ++i) {
        channels_[i].voltageHistory.resize(kMaxSamples, 0);
        channels_[i].currentHistory.resize(kMaxSamples, 0);
    }
    
    // Main layout - vertical layout, each channel is one row (matching Python version exactly)
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    
    // Create UI for each channel - each channel is a horizontal row
    for (int ch = 0; ch < kMaxChannels; ++ch) {
        setupChannelUI(ch);
        
        // Each channel gets a horizontal row layout
        auto* rowLayout = new QHBoxLayout();
        rowLayout->setSpacing(12);
        
        // Left side: vertical layout for controls (button + checkboxes)
        auto* controlLayout = new QVBoxLayout();
        controlLayout->setSpacing(6);
        controlLayout->setContentsMargins(0, 0, 0, 0);
        controlLayout->addStretch();  // Top spacing
        
        // Add button
        controlLayout->addWidget(channelWidgets_[ch].button);
        controlLayout->addSpacing(4);
        
        // Add checkboxes
        controlLayout->addWidget(channelWidgets_[ch].voltageCheck);
        controlLayout->addWidget(channelWidgets_[ch].currentCheck);
        
        rowLayout->addLayout(controlLayout);
        
        // Right side: chart view (takes most space)
        rowLayout->addWidget(channelWidgets_[ch].chartView, 1);
        
        mainLayout->addLayout(rowLayout);
    }
}

OscilloscopeWidget::~OscilloscopeWidget() {
}

void OscilloscopeWidget::setupChannelUI(int channelIdx) {
    auto& widgets = channelWidgets_[channelIdx];
    
    // Create button for channel toggle - matching Python version style
    widgets.button = new QPushButton(QString("Channel %1").arg(channelIdx + 1), this);
    widgets.button->setCheckable(true);
    widgets.button->setChecked(true);
    widgets.button->setFixedWidth(110);
    widgets.button->setStyleSheet(
        "QPushButton { "
        "  background-color: #6f7278; "
        "  color: #f1f1f1; "
        "  border: none; "
        "  border-radius: 6px; "
        "  padding: 6px 12px; "
        "  font-weight: bold;"
        "}"
        "QPushButton:checked { "
        "  background-color: #e9b507; "
        "  color: #1a1a1a; "
        "}"
        "QPushButton:hover { "
        "  background-color: #7d8086; "
        "}"
    );
    connect(widgets.button, &QPushButton::clicked, this, [this, channelIdx]() {
        toggleChannel(channelIdx);
    });
    
    // Create checkboxes for voltage/current visibility - matching Python version style
    widgets.voltageCheck = new QCheckBox(" Voltage", this);
    widgets.voltageCheck->setChecked(true);
    widgets.voltageCheck->setStyleSheet(
        "QCheckBox { color: #d8d8d8; spacing: 5px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; border-radius: 3px; }"
        "QCheckBox::indicator:unchecked { background: #2a2d33; border: 1px solid #80848a; }"
        "QCheckBox::indicator:checked { background: #e9b507; border: 1px solid #f0c63c; }"
    );
    connect(widgets.voltageCheck, &QCheckBox::stateChanged, this, [this, channelIdx](int state) {
        toggleCurve(channelIdx, "voltage");
    });
    
    widgets.currentCheck = new QCheckBox(" Current", this);
    widgets.currentCheck->setChecked(true);
    widgets.currentCheck->setStyleSheet(
        "QCheckBox { color: #d8d8d8; spacing: 5px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; border-radius: 3px; }"
        "QCheckBox::indicator:unchecked { background: #2a2d33; border: 1px solid #80848a; }"
        "QCheckBox::indicator:checked { background: #e9b507; border: 1px solid #f0c63c; }"
    );
    connect(widgets.currentCheck, &QCheckBox::stateChanged, this, [this, channelIdx](int state) {
        toggleCurve(channelIdx, "current");
    });
    
    // Create labels for displaying values - matching Python version
    widgets.voltageLabel = new QLabel("-- V", this);
    widgets.voltageLabel->setStyleSheet("color: rgb(255, 255, 0); font-weight: bold;");
    widgets.voltageLabel->setMinimumWidth(80);
    
    widgets.currentLabel = new QLabel("-- A", this);
    widgets.currentLabel->setStyleSheet("color: rgb(218, 0, 102); font-weight: bold;");
    widgets.currentLabel->setMinimumWidth(80);
    
    // Create chart and series - matching Python PlotWidget exactly
    QChart* chart = new QChart();
    chart->setBackgroundBrush(QBrush(QColor(20, 22, 26)));  // Black background
    chart->setPlotAreaBackgroundBrush(QBrush(QColor(20, 22, 26)));
    chart->setPlotAreaBackgroundVisible(true);
    chart->legend()->setVisible(false);
    chart->setMargins(QMargins(0, 0, 0, 0));
    
    // Remove chart title and borders
    chart->setTitle("");
    
    // Voltage series - yellow (matching Python color: 255, 255, 0)
    widgets.voltageSeries = new QLineSeries();
    widgets.voltageSeries->setName("Voltage");
    QPen voltagePen(QColor(255, 255, 0));
    voltagePen.setWidth(2);
    widgets.voltageSeries->setPen(voltagePen);
    chart->addSeries(widgets.voltageSeries);
    
    // Current series - pink/magenta (matching Python color: 218, 0, 102)
    widgets.currentSeries = new QLineSeries();
    widgets.currentSeries->setName("Current");
    QPen currentPen(QColor(218, 0, 102));
    currentPen.setWidth(1.5);
    widgets.currentSeries->setPen(currentPen);
    chart->addSeries(widgets.currentSeries);
    
    // X axis - sample index (0-100) - matching Python version
    widgets.axisX = new QValueAxis();
    widgets.axisX->setRange(0, 100);
    widgets.axisX->setLabelFormat("%d");
    widgets.axisX->setTickCount(6);  // 0, 20, 40, 60, 80, 100
    widgets.axisX->setTitleText("");  // No title
    widgets.axisX->setGridLineVisible(false);  // No grid lines like Python
    widgets.axisX->setLineVisible(true);
    widgets.axisX->setLinePen(QPen(QColor(160, 165, 175), 1));
    widgets.axisX->setLabelsColor(QColor(160, 165, 175));
    widgets.axisX->setLabelsFont(QFont("Arial", 9));
    chart->addAxis(widgets.axisX, Qt::AlignBottom);
    widgets.voltageSeries->attachAxis(widgets.axisX);
    widgets.currentSeries->attachAxis(widgets.axisX);
    
    // Y axis - voltage/current range (0-5000 mV) - matching Python version
    widgets.axisY = new QValueAxis();
    widgets.axisY->setRange(0, 5000);
    widgets.axisY->setLabelFormat("%d");
    widgets.axisY->setTickCount(6);  // 0, 1000, 2000, 3000, 4000, 5000
    widgets.axisY->setTitleText("");  // No title
    widgets.axisY->setGridLineVisible(false);  // No grid lines like Python
    widgets.axisY->setLineVisible(true);
    widgets.axisY->setLinePen(QPen(QColor(160, 165, 175), 1));
    widgets.axisY->setLabelsColor(QColor(160, 165, 175));
    widgets.axisY->setLabelsFont(QFont("Arial", 9));
    chart->addAxis(widgets.axisY, Qt::AlignLeft);
    widgets.voltageSeries->attachAxis(widgets.axisY);
    widgets.currentSeries->attachAxis(widgets.axisY);
    
    // Create chart view - matching Python PlotWidget
    widgets.chartView = new QChartView(chart, this);
    widgets.chartView->setRenderHint(QPainter::Antialiasing);
    widgets.chartView->setBackgroundBrush(QBrush(QColor(20, 22, 26)));  // Black background
    widgets.chartView->setMinimumHeight(120);
    widgets.chartView->setStyleSheet("border: none;");
}

void OscilloscopeWidget::setChannelCount(int count) {
    channelCount_ = qBound(1, count, kMaxChannels);
    update();
}

void OscilloscopeWidget::setChannelEnabled(int channel, bool enabled) {
    if (channel >= 0 && channel < kMaxChannels) {
        channelEnabled_[channel] = enabled;
        if (channelWidgets_[channel].button) {
            channelWidgets_[channel].button->setEnabled(enabled);
        }
    }
}

void OscilloscopeWidget::setSeriesVisible(int channel, bool voltageVisible, bool currentVisible) {
    if (channel >= 0 && channel < kMaxChannels) {
        auto& widgets = channelWidgets_[channel];
        if (widgets.voltageCheck) {
            widgets.voltageCheck->setChecked(voltageVisible);
        }
        if (widgets.currentCheck) {
            widgets.currentCheck->setChecked(currentVisible);
        }
        if (widgets.voltageSeries) {
            widgets.voltageSeries->setVisible(voltageVisible);
        }
        if (widgets.currentSeries) {
            widgets.currentSeries->setVisible(currentVisible);
        }
    }
}

void OscilloscopeWidget::updateVoltage(int channel, int milliVolt) {
    if (channel >= 0 && channel < kMaxChannels) {
        // Roll data left and add new value at end - matching Python np.roll
        auto& data = channels_[channel].voltageHistory;
        for (int i = 0; i < kMaxSamples - 1; ++i) {
            data[i] = data[i + 1];
        }
        data[kMaxSamples - 1] = milliVolt;
        
        // Update label - matching Python format
        auto& widgets = channelWidgets_[channel];
        if (widgets.voltageLabel) {
            double volts = milliVolt / 1000.0;
            widgets.voltageLabel->setText(QString::asprintf("%.3f V", volts));
        }
        
        // Update chart series
        updateChartData(channel);
    }
}

void OscilloscopeWidget::updateCurrent(int channel, int milliAmp) {
    if (channel >= 0 && channel < kMaxChannels) {
        // Roll data left and add new value at end - matching Python np.roll
        auto& data = channels_[channel].currentHistory;
        for (int i = 0; i < kMaxSamples - 1; ++i) {
            data[i] = data[i + 1];
        }
        data[kMaxSamples - 1] = milliAmp;
        
        // Update label - matching Python format
        auto& widgets = channelWidgets_[channel];
        if (widgets.currentLabel) {
            double amps = milliAmp / 1000.0;
            widgets.currentLabel->setText(QString::asprintf("%.3f A", amps));
        }
        
        // Update chart series
        updateChartData(channel);
    }
}

void OscilloscopeWidget::setChannelPowered(int channel, bool powered) {
    if (channel >= 0 && channel < kMaxChannels) {
        channels_[channel].powered = powered;
        auto& widgets = channelWidgets_[channel];
        if (widgets.button) {
            // Change button appearance based on power state - matching Python
            if (powered) {
                widgets.button->setStyleSheet(
                    "QPushButton { background-color: #e9b507; color: #1a1a1a; font-weight: bold; border: 1px solid #f0c63c; border-radius: 6px; padding: 6px 12px; }"
                    "QPushButton:hover { background-color: #f0c63c; }"
                );
            } else {
                widgets.button->setStyleSheet(
                    "QPushButton { background-color: #6f7278; color: #f1f1f1; border: 1px solid #80848a; border-radius: 6px; padding: 6px 12px; }"
                    "QPushButton:hover { background-color: #7d8086; }"
                );
            }
        }
    }
}

void OscilloscopeWidget::toggleChannel(int channelIdx) {
    // Toggle channel power state - this signal should be handled by MainWindow
    emit channelPowerToggled(channelIdx + 1, channelWidgets_[channelIdx].button->isChecked());
}

void OscilloscopeWidget::toggleCurve(int channelIdx, const QString& dataType) {
    auto& widgets = channelWidgets_[channelIdx];
    
    if (dataType == "voltage") {
        bool visible = widgets.voltageCheck->isChecked();
        if (widgets.voltageSeries) {
            widgets.voltageSeries->setVisible(visible);
        }
    } else if (dataType == "current") {
        bool visible = widgets.currentCheck->isChecked();
        if (widgets.currentSeries) {
            widgets.currentSeries->setVisible(visible);
        }
    }
}

void OscilloscopeWidget::updateChartData(int channelIdx) {
    auto& widgets = channelWidgets_[channelIdx];
    
    // Update voltage series - matching Python setData
    if (widgets.voltageSeries) {
        widgets.voltageSeries->clear();
        for (int i = 0; i < kMaxSamples; ++i) {
            widgets.voltageSeries->append(i, channels_[channelIdx].voltageHistory[i]);
        }
    }
    
    // Update current series - matching Python setData
    if (widgets.currentSeries) {
        widgets.currentSeries->clear();
        for (int i = 0; i < kMaxSamples; ++i) {
            widgets.currentSeries->append(i, channels_[channelIdx].currentHistory[i]);
        }
    }
}

#else
// QPainter 实现（当 Qt Charts 不可用时）
#include <QPainter>
#include <QPainterPath>
#include <QDebug>

OscilloscopeWidget::OscilloscopeWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumSize(900, 400);
    channelCount_ = kMaxChannels;
    channels_.resize(kMaxChannels);
    channelEnabled_.resize(kMaxChannels, true);
    
    for (int i = 0; i < kMaxChannels; ++i) {
        channels_[i].voltageHistory.resize(kMaxSamples, 0);
        channels_[i].currentHistory.resize(kMaxSamples, 0);
    }
}

OscilloscopeWidget::~OscilloscopeWidget() {
}

void OscilloscopeWidget::setChannelCount(int count) {
    channelCount_ = qBound(1, count, kMaxChannels);
    channels_.resize(kMaxChannels);
    for (int i = 0; i < kMaxChannels; ++i) {
        channels_[i].voltageHistory.resize(kMaxSamples);
        channels_[i].currentHistory.resize(kMaxSamples);
    }
    update();
}

void OscilloscopeWidget::setChannelEnabled(int channel, bool enabled) {
    if (channel >= 0 && channel < kMaxChannels) {
        channels_[channel].enabled = enabled;
        update();
    }
}

void OscilloscopeWidget::setSeriesVisible(int channel, bool voltageVisible, bool currentVisible) {
    if (channel >= 0 && channel < kMaxChannels) {
        channels_[channel].voltageVisible = voltageVisible;
        channels_[channel].currentVisible = currentVisible;
        update();
    }
}

void OscilloscopeWidget::updateVoltage(int channel, int milliVolt) {
    if (channel >= 0 && channel < kMaxChannels) {
        auto& data = channels_[channel];
        // Roll left and insert new value at end
        for (int i = 0; i < kMaxSamples - 1; ++i) {
            data.voltageHistory[i] = data.voltageHistory[i + 1];
        }
        data.voltageHistory[kMaxSamples - 1] = milliVolt;
        update();
    }
}

void OscilloscopeWidget::updateCurrent(int channel, int milliAmp) {
    if (channel >= 0 && channel < kMaxChannels) {
        auto& data = channels_[channel];
        // Roll left and insert new value at end
        for (int i = 0; i < kMaxSamples - 1; ++i) {
            data.currentHistory[i] = data.currentHistory[i + 1];
        }
        data.currentHistory[kMaxSamples - 1] = milliAmp;
        update();
    }
}

void OscilloscopeWidget::setChannelPowered(int channel, bool powered) {
    if (channel >= 0 && channel < kMaxChannels) {
        channels_[channel].powered = powered;
        update();
    }
}

void OscilloscopeWidget::toggleChannel(int channelIdx) {
    emit channelPowerToggled(channelIdx + 1, !channels_[channelIdx].powered);
}

void OscilloscopeWidget::toggleCurve(int channelIdx, const QString& dataType) {
    // This is handled externally in QPainter version
    Q_UNUSED(channelIdx);
    Q_UNUSED(dataType);
}

void OscilloscopeWidget::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    // Background - dark theme like Python version
    p.fillRect(rect(), QColor(50, 52, 58));

    const int w = width();
    const int h = height();
    if (w <= 0 || h <= 0) return;

    // Layout: each channel is a row
    // Channel rows are separated horizontally
    const int rowGap = 8;
    const int totalGap = rowGap * (channelCount_ + 1);
    const int rowH = (h - totalGap) / channelCount_;

    for (int ch = 0; ch < channelCount_; ++ch) {
        const int rowTop = rowGap + ch * (rowH + rowGap);
        QRect rowRect(8, rowTop, w - 16, rowH);
        drawChannel(p, ch, rowRect);
    }
}

void OscilloscopeWidget::drawChannel(QPainter& p, int channel, const QRect& rect) {
    if (!channels_[channel].enabled) return;

    const auto& data = channels_[channel];

    // Row background - slightly lighter
    p.fillRect(rect, QColor(45, 47, 52));

    // Row border
    p.setPen(QPen(QColor(70, 72, 78), 1));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    // Plot area - black background
    QRect plotRect(rect.left() + 8, rect.top() + 4, rect.width() - 16, rect.height() - 8);
    p.fillRect(plotRect, QColor(20, 22, 26));

    // Plot border
    p.setPen(QPen(QColor(80, 82, 88), 1));
    p.drawRect(plotRect.adjusted(0, 0, -1, -1));

    // Y-axis range labels (left side)
    p.setPen(QColor(160, 165, 175));
    QFont labelFont = p.font();
    labelFont.setPixelSize(10);
    p.setFont(labelFont);

    // Draw Y-axis labels: 5.5V at top, 0V at bottom
    const int labelPadding = 6;
    p.drawText(plotRect.left() - 28, plotRect.top() + 12, "5.5V");
    p.drawText(plotRect.left() - 28, plotRect.bottom() + 4, "0V");

    // Center line (at 2.75V)
    const int centerY = plotRect.bottom() - (kMaxMilliUnit / 2) * plotRect.height() / kMaxMilliUnit;
    p.setPen(QPen(QColor(60, 63, 70), 1, Qt::DashLine));
    p.drawLine(plotRect.left(), centerY, plotRect.right(), centerY);

    // X-axis labels at bottom
    p.drawText(plotRect.left() - 4, plotRect.bottom() + 14, "0");
    p.drawText(plotRect.center().x() - 4, plotRect.bottom() + 14, "50");
    p.drawText(plotRect.right() - 10, plotRect.bottom() + 14, "100");

    // Grid lines
    p.setPen(QPen(QColor(50, 52, 58), 1));
    // Horizontal grid (5 divisions)
    for (int i = 1; i < 5; ++i) {
        int y = plotRect.bottom() - i * plotRect.height() / 5;
        p.drawLine(plotRect.left(), y, plotRect.right(), y);
    }
    // Vertical grid (10 divisions)
    for (int i = 1; i < 10; ++i) {
        int x = plotRect.left() + i * plotRect.width() / 10;
        p.drawLine(x, plotRect.top(), x, plotRect.bottom());
    }

    // Draw waveforms
    if (data.voltageHistory.size() >= 2 && data.voltageVisible) {
        // Voltage waveform - yellow
        QPen glowPen(QColor(255, 255, 0, 40), 4);
        p.setPen(glowPen);
        for (int i = 1; i < data.voltageHistory.size(); ++i) {
            int x1 = plotRect.left() + (i - 1) * plotRect.width() / (data.voltageHistory.size() - 1);
            int x2 = plotRect.left() + i * plotRect.width() / (data.voltageHistory.size() - 1);
            int y1 = plotRect.bottom() - data.voltageHistory[i - 1] * plotRect.height() / kMaxMilliUnit;
            int y2 = plotRect.bottom() - data.voltageHistory[i] * plotRect.height() / kMaxMilliUnit;
            p.drawLine(x1, y1, x2, y2);
        }

        QPen mainPen(QColor(255, 255, 0), 1.5);
        p.setPen(mainPen);
        for (int i = 1; i < data.voltageHistory.size(); ++i) {
            int x1 = plotRect.left() + (i - 1) * plotRect.width() / (data.voltageHistory.size() - 1);
            int x2 = plotRect.left() + i * plotRect.width() / (data.voltageHistory.size() - 1);
            int y1 = plotRect.bottom() - data.voltageHistory[i - 1] * plotRect.height() / kMaxMilliUnit;
            int y2 = plotRect.bottom() - data.voltageHistory[i] * plotRect.height() / kMaxMilliUnit;
            p.drawLine(x1, y1, x2, y2);
        }

        // Voltage label on waveform
        int lastV = data.voltageHistory.last();
        int labelY = plotRect.bottom() - lastV * plotRect.height() / kMaxMilliUnit;
        p.setPen(QColor(255, 255, 0));
        p.drawText(plotRect.left() + 8, labelY - 8, QString::asprintf("%.3f V", lastV / 1000.0));
    }

    if (data.currentHistory.size() >= 2 && data.currentVisible) {
        // Current waveform - pink/magenta
        QPen glowPen(QColor(218, 0, 102, 40), 4);
        p.setPen(glowPen);
        for (int i = 1; i < data.currentHistory.size(); ++i) {
            int x1 = plotRect.left() + (i - 1) * plotRect.width() / (data.currentHistory.size() - 1);
            int x2 = plotRect.left() + i * plotRect.width() / (data.currentHistory.size() - 1);
            int y1 = plotRect.bottom() - data.currentHistory[i - 1] * plotRect.height() / kMaxMilliUnit;
            int y2 = plotRect.bottom() - data.currentHistory[i] * plotRect.height() / kMaxMilliUnit;
            p.drawLine(x1, y1, x2, y2);
        }

        QPen mainPen(QColor(218, 0, 102), 1.5);
        p.setPen(mainPen);
        for (int i = 1; i < data.currentHistory.size(); ++i) {
            int x1 = plotRect.left() + (i - 1) * plotRect.width() / (data.currentHistory.size() - 1);
            int x2 = plotRect.left() + i * plotRect.width() / (data.currentHistory.size() - 1);
            int y1 = plotRect.bottom() - data.currentHistory[i - 1] * plotRect.height() / kMaxMilliUnit;
            int y2 = plotRect.bottom() - data.currentHistory[i] * plotRect.height() / kMaxMilliUnit;
            p.drawLine(x1, y1, x2, y2);
        }

        // Current label on waveform
        int lastI = data.currentHistory.last();
        int labelY = plotRect.bottom() - lastI * plotRect.height() / kMaxMilliUnit;
        p.setPen(QColor(218, 0, 102));
        p.drawText(plotRect.left() + 8, labelY + 14, QString::asprintf("%.3f A", lastI / 1000.0));
    }

    // Channel label (top-left of plot area)
    p.setPen(data.powered ? QColor(255, 220, 50) : QColor(130, 135, 145));
    QFont chFont = p.font();
    chFont.setPixelSize(11);
    chFont.setBold(true);
    p.setFont(chFont);
    p.drawText(plotRect.left() + 8, plotRect.top() + 14, QString("Channel %1").arg(channel + 1));
}
#endif
