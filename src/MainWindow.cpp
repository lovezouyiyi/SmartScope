#include "MainWindow.h"

#include <QHBoxLayout>
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

    // Connection row - matching Python version
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

    // Oscilloscope widget - contains all channel controls and charts (matching Python version)
    oscilloscope_ = new OscilloscopeWidget(rootWidget_);
    rootLayout->addWidget(oscilloscope_, 1);

    setCentralWidget(rootWidget_);
    setWindowTitle("Smart USB Hub Oscilloscope");
    resize(1000, 700);

    // Style sheet - dark theme matching Python version
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

    // Connect oscilloscope signals
    connect(oscilloscope_, &OscilloscopeWidget::channelPowerToggled, this, [this](int channel, bool checked) {
        onPowerToggled(channel, checked);
    });

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
    //gfgfg
    

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
        // If failed, revert the button state
        oscilloscope_->setChannelPowered(channel - 1, !checked);
        return;
    }

    oscilloscope_->setChannelPowered(channel - 1, checked);
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

    if (power.has_value()) {
        oscilloscope_->setChannelPowered(idx, power.value() != 0);
    }

    if (voltage.has_value()) {
        oscilloscope_->updateVoltage(idx, voltage.value());
    }

    if (current.has_value()) {
        oscilloscope_->updateCurrent(idx, current.value());
    }
}
