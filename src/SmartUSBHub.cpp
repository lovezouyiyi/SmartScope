#include "SmartUSBHub.h"

#include <QElapsedTimer>
#include <QSerialPortInfo>

QStringList SmartUSBHub::scanAvailablePorts() {
    QStringList ports;
    const auto infos = QSerialPortInfo::availablePorts();

    for (const auto& info : infos) {
        if (info.hasVendorIdentifier() && info.hasProductIdentifier() &&
            info.vendorIdentifier() == kVendorId && info.productIdentifier() == kProductId) {
            ports << info.portName();
        }
    }

    return ports;
}

std::unique_ptr<SmartUSBHub> SmartUSBHub::scanAndConnect() {
    const auto infos = QSerialPortInfo::availablePorts();

    for (const auto& info : infos) {
        if (!info.hasVendorIdentifier() || !info.hasProductIdentifier()) {
            continue;
        }

        if (info.vendorIdentifier() != kVendorId || info.productIdentifier() != kProductId) {
            continue;
        }

        auto hub = std::make_unique<SmartUSBHub>(info.portName());
        if (hub->isConnected()) {
            return hub;
        }
    }

    return nullptr;
}

SmartUSBHub::SmartUSBHub(const QString& portName) {
    openPort(portName);
}

SmartUSBHub::~SmartUSBHub() {
    disconnect();
}

bool SmartUSBHub::isConnected() const {
    return serial_.isOpen();
}

QString SmartUSBHub::portName() const {
    return serial_.portName();
}

void SmartUSBHub::disconnect() {
    if (serial_.isOpen()) {
        serial_.clear();
        serial_.close();
    }
}

bool SmartUSBHub::setChannelPower(int channel, bool on) {
    return setChannelPower(QVector<int>{channel}, on);
}

bool SmartUSBHub::setChannelPower(const QVector<int>& channels, bool on) {
    const auto mask = channelMask(channels);
    if (mask == 0) {
        return false;
    }

    const auto response = transact(CMD_SET_CHANNEL_POWER, mask, QVector<quint8>{static_cast<quint8>(on ? 1 : 0)});
    return response.has_value();
}

std::optional<int> SmartUSBHub::getChannelPowerStatus(int channel) {
    const auto mask = channelMask(QVector<int>{channel});
    if (mask == 0) {
        return std::nullopt;
    }

    const auto response = transact(CMD_GET_CHANNEL_POWER_STATUS, mask, QVector<quint8>{0x00});
    if (!response || response->value.isEmpty()) {
        return std::nullopt;
    }

    if (!frameContainsChannel(response->channel, channel)) {
        return std::nullopt;
    }

    return static_cast<int>(response->value[0]);
}

std::optional<int> SmartUSBHub::getChannelVoltage(int channel) {
    const auto mask = channelMask(QVector<int>{channel});
    if (mask == 0) {
        return std::nullopt;
    }

    const auto response = transact(CMD_GET_CHANNEL_VOLTAGE, mask, QVector<quint8>{0x00});
    if (!response || response->value.size() < 2) {
        return std::nullopt;
    }

    if (!frameContainsChannel(response->channel, channel)) {
        return std::nullopt;
    }

    return combineBigEndian(response->value[0], response->value[1]);
}

std::optional<int> SmartUSBHub::getChannelCurrent(int channel) {
    const auto mask = channelMask(QVector<int>{channel});
    if (mask == 0) {
        return std::nullopt;
    }

    const auto response = transact(CMD_GET_CHANNEL_CURRENT, mask, QVector<quint8>{0x00});
    if (!response || response->value.size() < 2) {
        return std::nullopt;
    }

    if (!frameContainsChannel(response->channel, channel)) {
        return std::nullopt;
    }

    return combineBigEndian(response->value[0], response->value[1]);
}

bool SmartUSBHub::openPort(const QString& portName) {
    serial_.setPortName(portName);
    serial_.setBaudRate(QSerialPort::Baud115200);
    serial_.setDataBits(QSerialPort::Data8);
    serial_.setParity(QSerialPort::NoParity);
    serial_.setStopBits(QSerialPort::OneStop);
    serial_.setFlowControl(QSerialPort::NoFlowControl);

    if (!serial_.open(QIODevice::ReadWrite)) {
        return false;
    }

    serial_.clear();
    return true;
}

quint8 SmartUSBHub::channelMask(const QVector<int>& channels) const {
    quint8 mask = 0;

    for (int ch : channels) {
        switch (ch) {
            case 1:
                mask |= CHANNEL_1;
                break;
            case 2:
                mask |= CHANNEL_2;
                break;
            case 3:
                mask |= CHANNEL_3;
                break;
            case 4:
                mask |= CHANNEL_4;
                break;
            default:
                break;
        }
    }

    return mask;
}

QByteArray SmartUSBHub::buildPacket(quint8 cmd, quint8 mask, const QVector<quint8>& data) const {
    QByteArray packet;
    packet.reserve(6 + data.size());

    packet.append(static_cast<char>(0x55));
    packet.append(static_cast<char>(0x5A));
    packet.append(static_cast<char>(cmd));
    packet.append(static_cast<char>(mask));

    quint32 checksum = cmd + mask;
    for (quint8 b : data) {
        packet.append(static_cast<char>(b));
        checksum += b;
    }

    packet.append(static_cast<char>(checksum & 0xFF));
    return packet;
}

std::optional<SmartUSBHub::Frame> SmartUSBHub::transact(quint8 cmd, quint8 mask, const QVector<quint8>& data) {
    if (!serial_.isOpen()) {
        return std::nullopt;
    }

    const auto packet = buildPacket(cmd, mask, data);
    if (serial_.write(packet) != packet.size()) {
        return std::nullopt;
    }

    if (!serial_.waitForBytesWritten(kIoTimeoutMs)) {
        return std::nullopt;
    }

    return readFrameForCmd(cmd, kIoTimeoutMs);
}

std::optional<SmartUSBHub::Frame> SmartUSBHub::readFrameForCmd(quint8 expectedCmd, int timeoutMs) {
    QByteArray buffer;
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < timeoutMs) {
        if (!serial_.waitForReadyRead(8)) {
            continue;
        }

        buffer += serial_.readAll();

        Frame frame;
        while (parseNextFrame(buffer, frame)) {
            if (frame.cmd == expectedCmd) {
                return frame;
            }
        }
    }

    return std::nullopt;
}

bool SmartUSBHub::parseNextFrame(QByteArray& buffer, Frame& outFrame) const {
    while (true) {
        if (buffer.size() < 6) {
            return false;
        }

        int headerPos = -1;
        for (int i = 0; i + 1 < buffer.size(); ++i) {
            if (static_cast<quint8>(buffer[i]) == 0x55 && static_cast<quint8>(buffer[i + 1]) == 0x5A) {
                headerPos = i;
                break;
            }
        }

        if (headerPos < 0) {
            buffer.clear();
            return false;
        }

        if (headerPos > 0) {
            buffer.remove(0, headerPos);
            if (buffer.size() < 6) {
                return false;
            }
        }

        const quint8 cmd = static_cast<quint8>(buffer[2]);
        const bool protoV2 = isProtocolV2(cmd);
        const int frameLen = protoV2 ? 7 : 6;

        if (buffer.size() < frameLen) {
            return false;
        }

        const quint8 channel = static_cast<quint8>(buffer[3]);
        const quint8 checksum = static_cast<quint8>(buffer[frameLen - 1]);

        quint32 calc = cmd + channel;
        QVector<quint8> value;

        if (protoV2) {
            const quint8 v0 = static_cast<quint8>(buffer[4]);
            const quint8 v1 = static_cast<quint8>(buffer[5]);
            calc += v0 + v1;
            value = {v0, v1};
        } else {
            const quint8 v0 = static_cast<quint8>(buffer[4]);
            calc += v0;
            value = {v0};
        }

        if ((calc & 0xFF) != checksum) {
            buffer.remove(0, 1);
            continue;
        }

        buffer.remove(0, frameLen);
        outFrame.cmd = cmd;
        outFrame.channel = channel;
        outFrame.value = value;
        return true;
    }
}

bool SmartUSBHub::isProtocolV2(quint8 cmd) {
    return cmd == CMD_GET_CHANNEL_VOLTAGE || cmd == CMD_GET_CHANNEL_CURRENT;
}

int SmartUSBHub::combineBigEndian(quint8 hi, quint8 lo) {
    return (static_cast<int>(hi) << 8) | static_cast<int>(lo);
}

bool SmartUSBHub::frameContainsChannel(quint8 channelMask, int channel) {
    if (channel < 1 || channel > 4) {
        return false;
    }

    const quint8 mask = static_cast<quint8>(1u << (channel - 1));
    return (channelMask & mask) != 0;
}
