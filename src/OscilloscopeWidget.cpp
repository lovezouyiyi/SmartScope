#include "OscilloscopeWidget.h"

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

QT_CHARTS_USE_NAMESPACE

OscilloscopeWidget::OscilloscopeWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumSize(900, 600);
    channelCount_ = kMaxChannels;
    
    // Initialize data arrays
    voltageData_.resize(kMaxChannels);
    currentData_.resize(kMaxChannels);
    channelEnabled_.resize(kMaxChannels, true);
    channelWidgets_.resize(kMaxChannels);
    
    for (int i = 0; i < kMaxChannels; ++i) {
        voltageData_[i].resize(kMaxSamples, 0.0);
        currentData_[i].resize(kMaxSamples, 0.0);
    }
    
    // Main layout - vertical layout for all channels (matching Python version)
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    
    // Create UI for each channel - matching Python layout exactly
    for (int ch = 0; ch < kMaxChannels; ++ch) {
        setupChannelUI(ch);
        
        // Each channel gets a horizontal row layout
        auto* rowLayout = new QHBoxLayout();
        rowLayout->setSpacing(12);
        
        // Vertical layout for controls (button + checkboxes)
        auto* controlLayout = new QVBoxLayout();
        controlLayout->setSpacing(6);
        controlLayout->addStretch(50);  // Add spacing at top like Python version
        
        // Add button
        controlLayout->addWidget(channelWidgets_[ch].button);
        
        // Add checkboxes
        controlLayout->addWidget(channelWidgets_[ch].voltageCheck);
        controlLayout->addWidget(channelWidgets_[ch].currentCheck);
        
        // Add labels if they exist in the struct (need to be added to header/struct)
        if (channelWidgets_[ch].voltageLabel) {
            controlLayout->addWidget(channelWidgets_[ch].voltageLabel);
        }
        if (channelWidgets_[ch].currentLabel) {
            controlLayout->addWidget(channelWidgets_[ch].currentLabel);
        }
        
        rowLayout->addLayout(controlLayout);
        
        // Add chart view (takes most space)
        rowLayout->addWidget(channelWidgets_[ch].chartView, 1);
        
        mainLayout->addLayout(rowLayout);
    }
}

OscilloscopeWidget::~OscilloscopeWidget() {
}

void OscilloscopeWidget::setupChannelUI(int channelIdx) {
    auto& widgets = channelWidgets_[channelIdx];
    
    // Create button for channel toggle - matching Python version
    widgets.button = new QPushButton(QString("Channel %1").arg(channelIdx + 1), this);
    widgets.button->setCheckable(true);
    widgets.button->setChecked(true);
    widgets.button->setFixedWidth(100);
    connect(widgets.button, &QPushButton::clicked, this, [this, channelIdx]() {
        toggleChannel(channelIdx);
    });
    
    // Create checkboxes for voltage/current visibility - matching Python version
    widgets.voltageCheck = new QCheckBox("Voltage", this);
    widgets.voltageCheck->setChecked(true);
    connect(widgets.voltageCheck, &QCheckBox::stateChanged, this, [this, channelIdx](int state) {
        toggleCurve(channelIdx, "voltage");
    });
    
    widgets.currentCheck = new QCheckBox("Current", this);
    widgets.currentCheck->setChecked(true);
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
    
    // Create chart and series - matching Python PlotWidget
    QChart* chart = new QChart();
    chart->setBackgroundBrush(QBrush(QColor(20, 22, 26)));
    chart->setPlotAreaBackgroundBrush(QBrush(QColor(30, 32, 36)));
    chart->setPlotAreaBackgroundVisible(true);
    chart->legend()->setVisible(false);
    
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
    currentPen.setWidth(2);
    widgets.currentSeries->setPen(currentPen);
    chart->addSeries(widgets.currentSeries);
    
    // X axis - sample index (0-99)
    widgets.axisX = new QValueAxis();
    widgets.axisX->setRange(0, kMaxSamples - 1);
    widgets.axisX->setLabelFormat("%d");
    widgets.axisX->setTitleText("Sample");
    widgets.axisX->setGridLineVisible(true);
    widgets.axisX->setGridLineColor(QColor(50, 52, 58));
    widgets.axisX->setLabelsColor(QColor(160, 165, 175));
    chart->addAxis(widgets.axisX, Qt::AlignBottom);
    widgets.voltageSeries->attachAxis(widgets.axisX);
    widgets.currentSeries->attachAxis(widgets.axisX);
    
    // Y axis - voltage/current range (0-5500 mV) - matching Python YRange(0, 5500)
    widgets.axisY = new QValueAxis();
    widgets.axisY->setRange(0, 5500);
    widgets.axisY->setLabelFormat("%d");
    widgets.axisY->setTitleText("mV / mA");
    widgets.axisY->setGridLineVisible(true);
    widgets.axisY->setGridLineColor(QColor(50, 52, 58));
    widgets.axisY->setLabelsColor(QColor(160, 165, 175));
    chart->addAxis(widgets.axisY, Qt::AlignLeft);
    widgets.voltageSeries->attachAxis(widgets.axisY);
    widgets.currentSeries->attachAxis(widgets.axisY);
    
    // Create chart view - matching Python PlotWidget
    widgets.chartView = new QChartView(chart, this);
    widgets.chartView->setRenderHint(QPainter::Antialiasing);
    widgets.chartView->setBackgroundBrush(QBrush(QColor(45, 47, 52)));
    widgets.chartView->setMinimumHeight(150);
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
        auto& data = voltageData_[channel];
        for (int i = 0; i < kMaxSamples - 1; ++i) {
            data[i] = data[i + 1];
        }
        data[kMaxSamples - 1] = static_cast<double>(milliVolt);
        
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
        auto& data = currentData_[channel];
        for (int i = 0; i < kMaxSamples - 1; ++i) {
            data[i] = data[i + 1];
        }
        data[kMaxSamples - 1] = static_cast<double>(milliAmp);
        
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
    
    // Update voltage series
    if (widgets.voltageSeries) {
        widgets.voltageSeries->clear();
        for (int i = 0; i < kMaxSamples; ++i) {
            widgets.voltageSeries->append(i, voltageData_[channelIdx][i]);
        }
    }
    
    // Update current series
    if (widgets.currentSeries) {
        widgets.currentSeries->clear();
        for (int i = 0; i < kMaxSamples; ++i) {
            widgets.currentSeries->append(i, currentData_[channelIdx][i]);
        }
    }
}
