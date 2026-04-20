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
