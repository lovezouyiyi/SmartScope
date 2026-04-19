#pragma once

#include <QSerialPort>
#include <QString>
#include <QStringList>
#include <QVector>

#include <memory>
#include <optional>

class SmartUSBHub {
public:
    static QStringList scanAvailablePorts();
    static std::unique_ptr<SmartUSBHub> scanAndConnect();

    explicit SmartUSBHub(const QString& portName);
    ~SmartUSBHub();

    SmartUSBHub(const SmartUSBHub&) = delete;
    SmartUSBHub& operator=(const SmartUSBHub&) = delete;

    bool isConnected() const;
    QString portName() const;
    void disconnect();

    bool setChannelPower(int channel, bool on);
    bool setChannelPower(const QVector<int>& channels, bool on);

    std::optional<int> getChannelPowerStatus(int channel);
    std::optional<int> getChannelVoltage(int channel);
    std::optional<int> getChannelCurrent(int channel);

private:
    struct Frame {
        quint8 cmd = 0;
        quint8 channel = 0;
        QVector<quint8> value;
    };

    static constexpr quint16 kVendorId = 0x1A86;
    static constexpr quint16 kProductId = 0xFE0C;

    static constexpr quint8 CMD_GET_CHANNEL_POWER_STATUS = 0x00;
    static constexpr quint8 CMD_SET_CHANNEL_POWER = 0x01;
    static constexpr quint8 CMD_GET_CHANNEL_VOLTAGE = 0x03;
    static constexpr quint8 CMD_GET_CHANNEL_CURRENT = 0x04;

    static constexpr quint8 CHANNEL_1 = 0x01;
    static constexpr quint8 CHANNEL_2 = 0x02;
    static constexpr quint8 CHANNEL_3 = 0x04;
    static constexpr quint8 CHANNEL_4 = 0x08;

    static constexpr int kIoTimeoutMs = 120;

    bool openPort(const QString& portName);

    quint8 channelMask(const QVector<int>& channels) const;
    QByteArray buildPacket(quint8 cmd, quint8 mask, const QVector<quint8>& data) const;

    std::optional<Frame> transact(quint8 cmd, quint8 mask, const QVector<quint8>& data);
    std::optional<Frame> readFrameForCmd(quint8 expectedCmd, int timeoutMs);
    bool parseNextFrame(QByteArray& buffer, Frame& outFrame) const;

    static bool isProtocolV2(quint8 cmd);
    static int combineBigEndian(quint8 hi, quint8 lo);
    static bool frameContainsChannel(quint8 channelMask, int channel);

    QSerialPort serial_;
};
