#include "MainWindow.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "OscilloscopeWidget.h"
#include "SmartUSBHub.h"

MainWindow::MainWindow() {
    setupUi();
    setupHub();

    connect(&pollTimer_, &QTimer::timeout, this, [this]() { pollOneChannel(); });
    pollTimer_.start(80);

    connect(&reconnectTimer_, &QTimer::timeout, this, [this]() { tryReconnect(); });
    reconnectTimer_.start(1200);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    rootWidget_ = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(rootWidget_);
    rootLayout->setContentsMargins(10, 10, 10, 10);
    rootLayout->setSpacing(8);

    // Connection row
    auto* connectRow = new QHBoxLayout();
    connectRow->setSpacing(8);

    connectRow->addWidget(new QLabel("Port:", rootWidget_));
    portEdit_ = new QLineEdit(rootWidget_);
    portEdit_->setPlaceholderText("COM5");
    connectRow->addWidget(portEdit_, 1);

    connectButton_ = new QPushButton("Connect", rootWidget_);
    connectRow->addWidget(connectButton_);
    rootLayout->addLayout(connectRow);

    connectionLabel_ = new QLabel("Device: not connected (input serial port and click Connect)", rootWidget_);
    rootLayout->addWidget(connectionLabel_);

    // Channel controls - matching Python version layout
    auto* controlsLayout = new QVBoxLayout();
    controlsLayout->setSpacing(6);

    for (int ch = 0; ch < kChannelCount; ++ch) {
        auto& ui = channelUi_[ch];

        // Channel row: button + voltage checkbox + current checkbox + value labels
        auto* channelRow = new QHBoxLayout();
        channelRow->setSpacing(12);

        // Power button
        ui.powerButton = new QPushButton(QString("Channel %1").arg(ch + 1), rootWidget_);
        ui.powerButton->setFixedWidth(90);
        ui.powerButton->setCheckable(true);
        ui.powerButton->setChecked(true);
        channelRow->addWidget(ui.powerButton);

        // Voltage checkbox and label
        ui.voltageCheck = new QCheckBox("Voltage", rootWidget_);
        ui.voltageCheck->setChecked(true);
        channelRow->addWidget(ui.voltageCheck);

        ui.voltageLabel = new QLabel("-- V", rootWidget_);
        ui.voltageLabel->setMinimumWidth(80);
        ui.voltageLabel->setStyleSheet("color: rgb(255, 255, 0); font-weight: bold;");
        channelRow->addWidget(ui.voltageLabel);

        // Current checkbox and label
        ui.currentCheck = new QCheckBox("Current", rootWidget_);
        ui.currentCheck->setChecked(true);
        channelRow->addWidget(ui.currentCheck);

        ui.currentLabel = new QLabel("-- A", rootWidget_);
        ui.currentLabel->setMinimumWidth(80);
        ui.currentLabel->setStyleSheet("color: rgb(218, 0, 102); font-weight: bold;");
        channelRow->addWidget(ui.currentLabel);

        channelRow->addStretch();
        controlsLayout->addLayout(channelRow);

        connect(ui.powerButton, &QPushButton::toggled, this, [this, ch](bool checked) {
            onPowerToggled(ch + 1, checked);
        });

        connect(ui.voltageCheck, &QCheckBox::stateChanged, this, [this, ch](int) {
            onSeriesVisibilityChanged(ch + 1);
        });
        connect(ui.currentCheck, &QCheckBox::stateChanged, this, [this, ch](int) {
            onSeriesVisibilityChanged(ch + 1);
        });
    }

    rootLayout->addLayout(controlsLayout);

    // Oscilloscope widget - matching Python PlotWidget
    oscilloscope_ = new OscilloscopeWidget(rootWidget_);
    rootLayout->addWidget(oscilloscope_, 1);

    setCentralWidget(rootWidget_);
    setWindowTitle("Smart USB Hub Oscilloscope");
    resize(1000, 700);

    // Style sheet - dark theme
    setStyleSheet(
        "QWidget { background-color: #323436; color: #d8d8d8; }"
        "QLabel { color: #d8d8d8; }"
        "QLineEdit { background: #202226; border: 1px solid #5e6168; border-radius: 4px; padding: 5px 8px; color: #eeeeee; }"
        "QPushButton { background: #6f7278; border: 1px solid #80848a; border-radius: 6px; padding: 6px 12px; color: #f1f1f1; }"
        "QPushButton:hover { background: #7d8086; }"
        "QPushButton:checked { background: #e9b507; color: #1a1a1a; border-color: #f0c63c; font-weight: 600; }"
        "QCheckBox { spacing: 6px; color: #d8d8d8; }"
        "QCheckBox::indicator { width: 14px; height: 14px; }"
        "QCheckBox::indicator:unchecked { background: #2a2d33; border: 1px solid #80848a; }"
        "QCheckBox::indicator:checked { background: #e9b507; border: 1px solid #f0c63c; }");

    connect(connectButton_, &QPushButton::clicked, this, [this]() { onConnectClicked(); });
    connect(portEdit_, &QLineEdit::returnPressed, this, [this]() { onConnectClicked(); });
}

void MainWindow::setupHub() {
    hub_.reset();
    updateConnectionText();
}

void MainWindow::tryReconnect() {
    if (hub_ && hub_->isConnected()) {
        return;
    }

    if (desiredPort_.isEmpty()) {
        updateConnectionText();
        return;
    }

    auto candidate = std::make_unique<SmartUSBHub>(desiredPort_);
    if (candidate->isConnected()) {
        hub_ = std::move(candidate);
    } else {
        hub_.reset();
    }

    updateConnectionText();
}

void MainWindow::pollOneChannel() {
    if (!hub_ || !hub_->isConnected()) {
        updateConnectionText();
        return;
    }

    const int channel = (pollIndex_ % kChannelCount) + 1;
    ++pollIndex_;

    const auto power = hub_->getChannelPowerStatus(channel);
    const auto voltage = hub_->getChannelVoltage(channel);
    const auto current = hub_->getChannelCurrent(channel);

    if (!power && !voltage && !current) {
        hub_->disconnect();
        updateConnectionText();
        return;
    }

    updateChannelUiValues(channel, power, voltage, current);
}

void MainWindow::onPowerToggled(int channel, bool checked) {
    if (!hub_ || !hub_->isConnected()) {
        return;
    }

    const bool ok = hub_->setChannelPower(channel, checked);
    if (!ok) {
        auto& button = *channelUi_[channel - 1].powerButton;
        QSignalBlocker blocker(&button);
        button.setChecked(!checked);
        return;
    }

    oscilloscope_->setChannelPowered(channel - 1, checked);
}

void MainWindow::onSeriesVisibilityChanged(int channel) {
    auto& ui = channelUi_[channel - 1];
    oscilloscope_->setSeriesVisible(
        channel - 1,
        ui.voltageCheck->isChecked(),
        ui.currentCheck->isChecked());
}

void MainWindow::onConnectClicked() {
    desiredPort_ = portEdit_->text().trimmed();
    if (desiredPort_.isEmpty()) {
        connectionLabel_->setText("Device: please input serial port (example: COM5)");
        return;
    }

    if (hub_) {
        hub_->disconnect();
        hub_.reset();
    }

    auto candidate = std::make_unique<SmartUSBHub>(desiredPort_);
    if (candidate->isConnected()) {
        hub_ = std::move(candidate);
    } else {
        hub_.reset();
    }

    updateConnectionText();
}

void MainWindow::updateConnectionText() {
    if (hub_ && hub_->isConnected()) {
        connectionLabel_->setText(QString("Device: connected on %1").arg(hub_->portName()));
    } else {
        if (desiredPort_.isEmpty()) {
            connectionLabel_->setText("Device: not connected (input serial port and click Connect)");
        } else {
            connectionLabel_->setText(QString("Device: not connected (target port: %1)").arg(desiredPort_));
        }
    }
}

void MainWindow::updateChannelUiValues(
    int channel,
    std::optional<int> power,
    std::optional<int> voltage,
    std::optional<int> current) {
    const int idx = channel - 1;
    auto& ui = channelUi_[idx];

    if (power.has_value()) {
        QSignalBlocker blocker(ui.powerButton);
        ui.powerButton->setChecked(power.value() != 0);
        oscilloscope_->setChannelPowered(idx, power.value() != 0);
    }

    if (voltage.has_value()) {
        latestVoltage_[idx] = voltage;
        ui.voltageLabel->setText(QString::asprintf("%.3f V", voltage.value() / 1000.0));
        oscilloscope_->updateVoltage(idx, voltage.value());
    }

    if (current.has_value()) {
        latestCurrent_[idx] = current;
        ui.currentLabel->setText(QString::asprintf("%.3f A", current.value() / 1000.0));
        oscilloscope_->updateCurrent(idx, current.value());
    }
}
