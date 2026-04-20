#include "OscilloscopeWidget.h"

#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QPen>
#include <QDebug>

QT_CHARTS_USE_NAMESPACE

OscilloscopeWidget::OscilloscopeWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumSize(900, 600);
    channelCount_ = kMaxChannels;
    
    // Initialize data arrays - matching Python np.zeros((4, 100))
    voltageData_.resize(kMaxChannels);
    currentData_.resize(kMaxChannels);
    channelEnabled_.resize(kMaxChannels, true);
    channelWidgets_.resize(kMaxChannels);
    
    for (int i = 0; i < kMaxChannels; ++i) {
        voltageData_[i].resize(kMaxSamples, 0.0);
        currentData_[i].resize(kMaxSamples, 0.0);
    }
    
    // Main layout - matching Python QVBoxLayout
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    
    // Create UI for each channel - matching Python for loop
    for (int ch = 0; ch < kMaxChannels; ++ch) {
        setupChannelUI(ch);
        
        // Each channel gets a horizontal row layout - matching Python QHBoxLayout
        auto* rowLayout = new QHBoxLayout();
        rowLayout->setSpacing(12);
        
        // Left side: vertical layout for controls - matching Python checkbox_layout
        auto* controlLayout = new QVBoxLayout();
        controlLayout->setSpacing(6);
        controlLayout->setContentsMargins(0, 0, 0, 0);
        controlLayout->insertSpacing(0, 50);  // Matching Python insertSpacing(0, 50)
        
        // Add button
        controlLayout->addWidget(channelWidgets_[ch].button);
        
        // Add checkboxes
        controlLayout->addWidget(channelWidgets_[ch].voltageCheck);
        controlLayout->addWidget(channelWidgets_[ch].currentCheck);
        
        rowLayout->addLayout(controlLayout);
        
        // Right side: chart view - matching Python plot_widget
        rowLayout->addWidget(channelWidgets_[ch].chartView, 1);
        
        mainLayout->addLayout(rowLayout);
    }
}

OscilloscopeWidget::~OscilloscopeWidget() {
}

void OscilloscopeWidget::setupChannelUI(int channelIdx) {
    auto& widgets = channelWidgets_[channelIdx];
    int chNum = channelIdx + 1;
    
    // Colors matching Python: voltage=(255,255,0), current=(218,0,102)
    const QColor voltageColor(255, 255, 0);
    const QColor currentColor(218, 0, 102);
    
    // Create button for channel toggle - matching Python
    widgets.button = new QPushButton(QString("Channel %1").arg(chNum), this);
    widgets.button->setCheckable(true);
    widgets.button->setChecked(true);
    widgets.button->setFixedWidth(100);
    widgets.button->setStyleSheet(
        "QPushButton { "
        "  background-color: #6f7278; "
        "  color: #f1f1f1; "
        "  border: 1px solid #80848a; "
        "  border-radius: 6px; "
        "  padding: 6px 12px; "
        "  font-weight: bold;"
        "}"
        "QPushButton:checked { "
        "  background-color: #e9b507; "
        "  color: #1a1a1a; "
        "  border-color: #f0c63c;"
        "}"
    );
    connect(widgets.button, &QPushButton::clicked, this, [this, channelIdx]() {
        toggleChannel(channelIdx);
    });
    
    // Create checkboxes for voltage/current visibility - matching Python
    widgets.voltageCheck = new QCheckBox("Voltage", this);
    widgets.voltageCheck->setChecked(true);
    widgets.voltageCheck->setStyleSheet(
        "QCheckBox { color: #d8d8d8; spacing: 5px; }"
        "QCheckBox::indicator { width: 14px; height: 14px; }"
        "QCheckBox::indicator:unchecked { background: #2a2d33; border: 1px solid #80848a; }"
        "QCheckBox::indicator:checked { background: #e9b507; border: 1px solid #f0c63c; }"
    );
    connect(widgets.voltageCheck, &QCheckBox::stateChanged, this, [this, channelIdx](int state) {
        toggleCurve(channelIdx, "voltage");
    });
    
    widgets.currentCheck = new QCheckBox("Current", this);
    widgets.currentCheck->setChecked(true);
    widgets.currentCheck->setStyleSheet(
        "QCheckBox { color: #d8d8d8; spacing: 5px; }"
        "QCheckBox::indicator { width: 14px; height: 14px; }"
        "QCheckBox::indicator:unchecked { background: #2a2d33; border: 1px solid #80848a; }"
        "QCheckBox::indicator:checked { background: #e9b507; border: 1px solid #f0c63c; }"
    );
    connect(widgets.currentCheck, &QCheckBox::stateChanged, this, [this, channelIdx](int state) {
        toggleCurve(channelIdx, "current");
    });
    
    // Create chart - matching Python pg.PlotWidget()
    QChart* chart = new QChart();
    chart->setBackgroundBrush(QBrush(QColor(20, 22, 26)));
    chart->setPlotAreaBackgroundBrush(QBrush(QColor(20, 22, 26)));
    chart->setPlotAreaBackgroundVisible(true);
    chart->legend()->setVisible(false);
    chart->setTitle("");
    chart->setMargins(QMargins(5, 5, 5, 5));
    
    // Voltage series - matching Python plot(pen=pg.mkPen(color=(255,255,0), width=1))
    widgets.voltageSeries = new QLineSeries();
    QPen voltagePen(voltageColor);
    voltagePen.setWidth(1);
    widgets.voltageSeries->setPen(voltagePen);
    chart->addSeries(widgets.voltageSeries);
    
    // Current series - matching Python plot(pen=pg.mkPen(color=(218,0,102), width=1))
    widgets.currentSeries = new QLineSeries();
    QPen currentPen(currentColor);
    currentPen.setWidth(1);
    widgets.currentSeries->setPen(currentPen);
    chart->addSeries(widgets.currentSeries);
    
    // X axis - sample index 0-99
    widgets.axisX = new QValueAxis();
    widgets.axisX->setRange(0, kMaxSamples - 1);
    widgets.axisX->setLabelFormat("%d");
    widgets.axisX->setTickCount(6);
    widgets.axisX->setTitleText("");
    widgets.axisX->setGridLineVisible(false);
    widgets.axisX->setLineVisible(true);
    widgets.axisX->setLinePen(QPen(QColor(100, 105, 115), 1));
    widgets.axisX->setLabelsColor(QColor(160, 165, 175));
    widgets.axisX->setLabelsFont(QFont("Arial", 8));
    chart->addAxis(widgets.axisX, Qt::AlignBottom);
    widgets.voltageSeries->attachAxis(widgets.axisX);
    widgets.currentSeries->attachAxis(widgets.axisX);
    
    // Y axis - matching Python setYRange(0, 5500)
    widgets.axisY = new QValueAxis();
    widgets.axisY->setRange(0, 5500);
    widgets.axisY->setLabelFormat("%d");
    widgets.axisY->setTickCount(6);
    widgets.axisY->setTitleText("");
    widgets.axisY->setGridLineVisible(false);
    widgets.axisY->setLineVisible(true);
    widgets.axisY->setLinePen(QPen(QColor(100, 105, 115), 1));
    widgets.axisY->setLabelsColor(QColor(160, 165, 175));
    widgets.axisY->setLabelsFont(QFont("Arial", 8));
    chart->addAxis(widgets.axisY, Qt::AlignLeft);
    widgets.voltageSeries->attachAxis(widgets.axisY);
    widgets.currentSeries->attachAxis(widgets.axisY);
    
    // Set left label - matching Python setLabel("left", f"Channel {channel}")
    // Qt Charts doesn't have direct equivalent, we use axis title
    widgets.axisY->setTitleText(QString("Channel %1").arg(chNum));
    widgets.axisY->setTitleBrush(QBrush(QColor(160, 165, 175)));
    
    // Create chart view - matching Python PlotWidget
    widgets.chartView = new QChartView(chart, this);
    widgets.chartView->setRenderHint(QPainter::Antialiasing);
    widgets.chartView->setBackgroundBrush(QBrush(QColor(20, 22, 26)));
    widgets.chartView->setStyleSheet("border: none;");
    widgets.chartView->setMinimumHeight(130);
}

void OscilloscopeWidget::setChannelCount(int count) {
    channelCount_ = qBound(1, count, kMaxChannels);
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
        // Roll data - matching Python np.roll(self.data["voltage"][i], -1)
        auto& data = voltageData_[channel];
        for (int i = 0; i < kMaxSamples - 1; ++i) {
            data[i] = data[i + 1];
        }
        data[kMaxSamples - 1] = static_cast<double>(milliVolt);
        
        // Update chart
        updateChartData(channel);
    }
}

void OscilloscopeWidget::updateCurrent(int channel, int milliAmp) {
    if (channel >= 0 && channel < kMaxChannels) {
        // Roll data - matching Python np.roll
        auto& data = currentData_[channel];
        for (int i = 0; i < kMaxSamples - 1; ++i) {
            data[i] = data[i + 1];
        }
        data[kMaxSamples - 1] = static_cast<double>(milliAmp);
        
        // Update chart
        updateChartData(channel);
    }
}

void OscilloscopeWidget::setChannelPowered(int channel, bool powered) {
    if (channel >= 0 && channel < kMaxChannels) {
        auto& widgets = channelWidgets_[channel];
        if (widgets.button) {
            if (powered) {
                widgets.button->setChecked(true);
            } else {
                widgets.button->setChecked(false);
            }
        }
    }
}

void OscilloscopeWidget::toggleChannel(int channelIdx) {
    // Emit signal to MainWindow to handle power toggle
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
            widgets.voltageSeries->append(i, voltageData_[channelIdx][i]);
        }
    }
    
    // Update current series - matching Python setData
    if (widgets.currentSeries) {
        widgets.currentSeries->clear();
        for (int i = 0; i < kMaxSamples; ++i) {
            widgets.currentSeries->append(i, currentData_[channelIdx][i]);
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
        channels_[i].voltageHistory.resize(kMaxSamples);
        channels_[i].currentHistory.resize(kMaxSamples);
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
