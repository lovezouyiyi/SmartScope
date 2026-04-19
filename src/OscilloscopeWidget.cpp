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
    p.fillRect(rect(), QColor(58, 60, 66));

    const int w = width();
    const int h = height();
    if (w <= 0 || h <= 0) {
        return;
    }

    const int rowGap = 10;
    const int totalGap = rowGap * (kChannelCount + 1);
    const int rowH = (h - totalGap) / kChannelCount;

    for (int ch = 1; ch <= kChannelCount; ++ch) {
        const int idx = ch - 1;
        const int rowTop = rowGap + idx * (rowH + rowGap);
        const QRect panelRect(10, rowTop, w - 20, rowH);
        const QRect plotRect(panelRect.left() + 62, panelRect.top() + 10,
                             panelRect.width() - 80, panelRect.height() - 28);

        p.fillRect(panelRect, QColor(0, 0, 0));
        p.setPen(QPen(QColor(95, 95, 95), 1));
        p.drawRect(panelRect.adjusted(0, 0, -1, -1));

        p.setPen(QPen(QColor(80, 80, 80), 1));
        p.drawLine(plotRect.left(), plotRect.bottom(), plotRect.right(), plotRect.bottom());
        p.drawLine(plotRect.left(), plotRect.top(), plotRect.left(), plotRect.bottom());

        p.setPen(QPen(QColor(45, 45, 45), 1));
        for (int gy = 1; gy < 10; ++gy) {
            const int y = plotRect.bottom() - gy * plotRect.height() / 10;
            p.drawLine(plotRect.left(), y, plotRect.right(), y);
        }
        for (int gx = 1; gx < 10; ++gx) {
            const int x = plotRect.left() + gx * plotRect.width() / 10;
            p.drawLine(x, plotRect.top(), x, plotRect.bottom());
        }

        p.setPen(QColor(165, 165, 165));
        for (int mv = 0; mv <= 5000; mv += 1000) {
            const int y = yForValue(mv, plotRect);
            p.drawLine(plotRect.left(), y, plotRect.left() + 4, y);
            p.drawText(plotRect.left() - 40, y + 4, QString::number(mv));
        }
        for (int xt = 0; xt <= 100; xt += 20) {
            const int x = plotRect.left() + xt * plotRect.width() / 100;
            p.drawLine(x, plotRect.bottom(), x, plotRect.bottom() - 4);
            p.drawText(x - 8, plotRect.bottom() + 16, QString::number(xt));
        }

        p.setPen(powered_[idx] ? QColor(236, 183, 0) : QColor(128, 128, 128));
        p.drawText(panelRect.left() + 8, panelRect.top() + 18, QString("Channel %1").arg(ch));

        const auto samples = engine_->samples(ch);
        if (samples.size() < 2) {
            continue;
        }

        if (visibility_[idx].voltageVisible) {
            p.setPen(QPen(QColor(247, 215, 56), 1.8));
            for (int i = 1; i < static_cast<int>(samples.size()); ++i) {
                const int x1 = xForSampleIndex(i - 1, static_cast<int>(samples.size()), plotRect);
                const int x2 = xForSampleIndex(i, static_cast<int>(samples.size()), plotRect);
                const int y1 = yForValue(samples[i - 1].voltageMilliV, plotRect);
                const int y2 = yForValue(samples[i].voltageMilliV, plotRect);
                p.drawLine(x1, y1, x2, y2);
            }
        }

        if (visibility_[idx].currentVisible) {
            p.setPen(QPen(QColor(218, 0, 102), 1.2));
            for (int i = 1; i < static_cast<int>(samples.size()); ++i) {
                const int x1 = xForSampleIndex(i - 1, static_cast<int>(samples.size()), plotRect);
                const int x2 = xForSampleIndex(i, static_cast<int>(samples.size()), plotRect);
                const int y1 = yForValue(samples[i - 1].currentMilliA, plotRect);
                const int y2 = yForValue(samples[i].currentMilliA, plotRect);
                p.drawLine(x1, y1, x2, y2);
            }
        }

        const auto latest = samples.back();
        if (visibility_[idx].voltageVisible) {
            p.setPen(QColor(247, 215, 56));
            p.drawText(plotRect.left() + 6, plotRect.top() + 16,
                       QString::asprintf("%.3f V", latest.voltageMilliV / 1000.0));
        }
        if (visibility_[idx].currentVisible) {
            p.setPen(QColor(218, 0, 102));
            p.drawText(plotRect.left() + 6, plotRect.top() + 34,
                       QString::asprintf("%.3f A", latest.currentMilliA / 1000.0));
        }
    }
}

int OscilloscopeWidget::indexFromChannel(int channel) const {
    if (channel < 1 || channel > kChannelCount) {
        return -1;
    }
    return channel - 1;
}

int OscilloscopeWidget::yForValue(int value, const QRect& plotRect) const {
    const int clamped = qBound(0, value, kMaxMilliUnit);
    return plotRect.bottom() - (clamped * plotRect.height() / kMaxMilliUnit);
}

int OscilloscopeWidget::xForSampleIndex(int sampleIndex, int sampleCount, const QRect& plotRect) const {
    if (sampleCount <= 1) {
        return plotRect.left();
    }
    return plotRect.left() + sampleIndex * plotRect.width() / (sampleCount - 1);
}
