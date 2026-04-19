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

void MainWindow::setupUi() {
    rootWidget_ = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(rootWidget_);
    rootLayout->setContentsMargins(10, 10, 10, 10);
    rootLayout->setSpacing(8);

    connectionLabel_ = new QLabel("Device: scanning...", rootWidget_);
    rootLayout->addWidget(connectionLabel_);

    auto* controls = new QGridLayout();
    controls->setHorizontalSpacing(12);
    controls->setVerticalSpacing(6);

    for (int ch = 1; ch <= kChannelCount; ++ch) {
        const int row = ch - 1;
        auto& ui = channelUi_[row];

        ui.powerButton = new QPushButton(QString("CH%1 Power").arg(ch), rootWidget_);
        ui.powerButton->setCheckable(true);

        ui.voltageCheck = new QCheckBox("Voltage", rootWidget_);
        ui.voltageCheck->setChecked(true);

        ui.currentCheck = new QCheckBox("Current", rootWidget_);
        ui.currentCheck->setChecked(true);

        ui.voltageLabel = new QLabel("-- V", rootWidget_);
        ui.currentLabel = new QLabel("-- A", rootWidget_);

        controls->addWidget(ui.powerButton, row, 0);
        controls->addWidget(ui.voltageCheck, row, 1);
        controls->addWidget(ui.currentCheck, row, 2);
        controls->addWidget(ui.voltageLabel, row, 3);
        controls->addWidget(ui.currentLabel, row, 4);

        connect(ui.powerButton, &QPushButton::toggled, this, [this, ch](bool checked) {
            onPowerToggled(ch, checked);
        });

        connect(ui.voltageCheck, &QCheckBox::checkStateChanged, this, [this, ch](Qt::CheckState) {
            onSeriesVisibilityChanged(ch);
        });
        connect(ui.currentCheck, &QCheckBox::checkStateChanged, this, [this, ch](Qt::CheckState) {
            onSeriesVisibilityChanged(ch);
        });
    }

    rootLayout->addLayout(controls);

    oscilloscope_ = new OscilloscopeWidget(&engine_, rootWidget_);
    rootLayout->addWidget(oscilloscope_, 1);

    setCentralWidget(rootWidget_);
    setWindowTitle("SmartScope (C++ Refactor)");
    resize(1180, 760);
}

void MainWindow::setupHub() {
    hub_ = SmartUSBHub::scanAndConnect();
    updateConnectionText();
}

void MainWindow::tryReconnect() {
    if (hub_ && hub_->isConnected()) {
        return;
    }

    hub_ = SmartUSBHub::scanAndConnect();
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
    oscilloscope_->update();
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

    oscilloscope_->setChannelPowered(channel, checked);
}

void MainWindow::onSeriesVisibilityChanged(int channel) {
    auto& ui = channelUi_[channel - 1];
    oscilloscope_->setSeriesVisible(
        channel,
        ui.voltageCheck->isChecked(),
        ui.currentCheck->isChecked());
    oscilloscope_->update();
}

void MainWindow::updateConnectionText() {
    if (hub_ && hub_->isConnected()) {
        connectionLabel_->setText(QString("Device: connected on %1").arg(hub_->portName()));
    } else {
        connectionLabel_->setText("Device: not connected (plug SmartUSBHub and wait)");
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
        oscilloscope_->setChannelPowered(channel, power.value() != 0);
    }

    if (voltage.has_value()) {
        latestVoltage_[idx] = voltage;
        ui.voltageLabel->setText(QString::asprintf("%.3f V", voltage.value() / 1000.0));
    }

    if (current.has_value()) {
        latestCurrent_[idx] = current;
        ui.currentLabel->setText(QString::asprintf("%.3f A", current.value() / 1000.0));
    }

    if (latestVoltage_[idx].has_value() && latestCurrent_[idx].has_value()) {
        engine_.push(channel, latestVoltage_[idx].value(), latestCurrent_[idx].value());
    }
}
