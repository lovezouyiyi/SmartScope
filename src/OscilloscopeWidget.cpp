#include "OscilloscopeWidget.h"

#include <QPainter>
#include <QPen>

OscilloscopeWidget::OscilloscopeWidget(DataEngine* engine, QWidget* parent)
    : QWidget(parent), engine_(engine) {
    setMinimumSize(960, 520);
}

void OscilloscopeWidget::setSeriesVisible(int channel, bool voltageVisible, bool currentVisible) {
    const int idx = indexFromChannel(channel);
    if (idx < 0) {
        return;
    }

    visibility_[idx].voltageVisible = voltageVisible;
    visibility_[idx].currentVisible = currentVisible;
}

void OscilloscopeWidget::setChannelPowered(int channel, bool powered) {
    const int idx = indexFromChannel(channel);
    if (idx < 0) {
        return;
    }

    powered_[idx] = powered;
}

void OscilloscopeWidget::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);

    if (!engine_) {
        return;
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), QColor(12, 14, 20));

    const int w = width();
    const int h = height();
    if (w <= 0 || h <= 0) {
        return;
    }

    const int rowH = h / kChannelCount;

    for (int ch = 1; ch <= kChannelCount; ++ch) {
        const int idx = ch - 1;
        const int rowTop = idx * rowH;
        const QRect rowRect(0, rowTop, w, rowH);

        p.setPen(QPen(QColor(36, 41, 52), 1));
        for (int gx = 0; gx <= 10; ++gx) {
            const int x = gx * w / 10;
            p.drawLine(x, rowTop, x, rowTop + rowH);
        }
        for (int gy = 0; gy <= 4; ++gy) {
            const int y = rowTop + gy * rowH / 4;
            p.drawLine(0, y, w, y);
        }

        const QColor channelColor = powered_[idx] ? QColor(0, 220, 120) : QColor(190, 82, 82);
        p.setPen(channelColor);
        p.drawText(rowRect.adjusted(8, 4, -8, -4), Qt::AlignLeft | Qt::AlignTop,
                   QString("CH%1 %2").arg(ch).arg(powered_[idx] ? "ON" : "OFF"));

        const auto samples = engine_->samples(ch);
        if (samples.size() < 2) {
            continue;
        }

        if (visibility_[idx].voltageVisible) {
            p.setPen(QPen(QColor(247, 215, 56), 1.8));
            for (int i = 1; i < static_cast<int>(samples.size()); ++i) {
                const int x1 = (i - 1) * w / static_cast<int>(samples.size() - 1);
                const int x2 = i * w / static_cast<int>(samples.size() - 1);
                const int y1 = yForValue(samples[i - 1].voltageMilliV, rowTop, rowH);
                const int y2 = yForValue(samples[i].voltageMilliV, rowTop, rowH);
                p.drawLine(x1, y1, x2, y2);
            }
        }

        if (visibility_[idx].currentVisible) {
            p.setPen(QPen(QColor(218, 0, 102), 1.2));
            for (int i = 1; i < static_cast<int>(samples.size()); ++i) {
                const int x1 = (i - 1) * w / static_cast<int>(samples.size() - 1);
                const int x2 = i * w / static_cast<int>(samples.size() - 1);
                const int y1 = yForValue(samples[i - 1].currentMilliA, rowTop, rowH);
                const int y2 = yForValue(samples[i].currentMilliA, rowTop, rowH);
                p.drawLine(x1, y1, x2, y2);
            }
        }
    }
}

int OscilloscopeWidget::indexFromChannel(int channel) const {
    if (channel < 1 || channel > kChannelCount) {
        return -1;
    }
    return channel - 1;
}

int OscilloscopeWidget::yForValue(int value, int rowTop, int rowHeight) const {
    const int clamped = qBound(0, value, kMaxMilliUnit);
    return rowTop + rowHeight - (clamped * rowHeight / kMaxMilliUnit);
}
