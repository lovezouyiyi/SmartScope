#include "OscilloscopeWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QDebug>

OscilloscopeWidget::OscilloscopeWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumSize(900, 400);
    channelCount_ = kMaxChannels;
    channels_.resize(kMaxChannels);
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
