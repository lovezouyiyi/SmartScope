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
    p.fillRect(rect(), QColor(45, 47, 52));

    const int w = width();
    const int h = height();
    if (w <= 0 || h <= 0) {
        return;
    }

    const int rowGap = 8;
    const int totalGap = rowGap * (kChannelCount + 1);
    const int rowH = (h - totalGap) / kChannelCount;

    for (int ch = 1; ch <= kChannelCount; ++ch) {
        const int idx = ch - 1;
        const int rowTop = rowGap + idx * (rowH + rowGap);
        const QRect panelRect(10, rowTop, w - 20, rowH);
        const QRect plotRect(panelRect.left() + 70, panelRect.top() + 8,
                             panelRect.width() - 88, panelRect.height() - 24);

        // 绘制面板背景 - 专业示波器深色
        p.fillRect(panelRect, QColor(30, 32, 36));
        
        // 面板边框
        p.setPen(QPen(powered_[idx] ? QColor(236, 183, 0, 120) : QColor(70, 72, 78), 1));
        p.drawRect(panelRect.adjusted(0, 0, -1, -1));

        // 绘制网格 - 主网格线（每5格）
        p.setPen(QPen(QColor(55, 58, 64), 1, Qt::SolidLine));
        for (int gy = 1; gy < 5; ++gy) {
            const int y = plotRect.bottom() - gy * plotRect.height() / 5;
            p.drawLine(plotRect.left(), y, plotRect.right(), y);
        }
        for (int gx = 1; gx < 10; ++gx) {
            if (gx % 2 == 0) continue;
            const int x = plotRect.left() + gx * plotRect.width() / 10;
            p.drawLine(x, plotRect.top(), x, plotRect.bottom());
        }

        // 绘制中心网格线（更亮）
        p.setPen(QPen(QColor(75, 78, 85), 1, Qt::SolidLine));
        const int centerY = plotRect.top() + plotRect.height() / 2;
        p.drawLine(plotRect.left(), centerY, plotRect.right(), centerY);
        const int centerX = plotRect.left() + plotRect.width() / 2;
        p.drawLine(centerX, plotRect.top(), centerX, plotRect.bottom());

        // 绘制边框
        p.setPen(QPen(QColor(95, 98, 105), 1));
        p.drawLine(plotRect.left(), plotRect.bottom(), plotRect.right(), plotRect.bottom());
        p.drawLine(plotRect.left(), plotRect.top(), plotRect.left(), plotRect.bottom());
        p.drawLine(plotRect.right(), plotRect.top(), plotRect.right(), plotRect.bottom());
        p.drawLine(plotRect.left(), plotRect.top(), plotRect.right(), plotRect.top());

        // Y轴刻度标签
        p.setPen(QColor(140, 145, 155));
        QFont labelFont = p.font();
        labelFont.setPointSize(8);
        p.setFont(labelFont);
        
        for (int mv = 0; mv <= 5000; mv += 1000) {
            const int y = yForValue(mv, plotRect);
            p.drawLine(plotRect.left(), y, plotRect.left() + 5, y);
            p.drawText(plotRect.left() - 42, y + 4, QString::number(mv / 1000) + ".0");
        }
        
        // X轴刻度标签
        for (int xt = 0; xt <= 100; xt += 25) {
            const int x = plotRect.left() + xt * plotRect.width() / 100;
            p.drawLine(x, plotRect.bottom(), x, plotRect.bottom() - 5);
            p.drawText(x - 10, plotRect.bottom() + 16, QString::number(xt));
        }

        // 通道标签 - 带电源状态指示
        QFont chFont = p.font();
        chFont.setPointSize(9);
        chFont.setBold(true);
        p.setFont(chFont);
        
        QRect chLabelRect(panelRect.left() + 8, panelRect.top() + 6, 54, 18);
        if (powered_[idx]) {
            p.fillRect(chLabelRect, QColor(236, 183, 0, 40));
            p.setPen(QColor(236, 183, 0));
        } else {
            p.fillRect(chLabelRect, QColor(80, 82, 88, 60));
            p.setPen(QColor(120, 125, 135));
        }
        p.drawText(chLabelRect, Qt::AlignCenter, QString("CH%1").arg(ch));

        const auto samples = engine_->samples(ch);
        if (samples.size() < 2) {
            continue;
        }

        // 绘制电压波形 - 带发光效果
        if (visibility_[idx].voltageVisible) {
            // 发光效果
            QPen glowPen(QColor(247, 215, 56, 60), 4);
            p.setPen(glowPen);
            for (int i = 1; i < static_cast<int>(samples.size()); ++i) {
                const int x1 = xForSampleIndex(i - 1, static_cast<int>(samples.size()), plotRect);
                const int x2 = xForSampleIndex(i, static_cast<int>(samples.size()), plotRect);
                const int y1 = yForValue(samples[i - 1].voltageMilliV, plotRect);
                const int y2 = yForValue(samples[i].voltageMilliV, plotRect);
                p.drawLine(x1, y1, x2, y2);
            }
            
            // 主波形线
            p.setPen(QPen(QColor(247, 215, 56), 2));
            for (int i = 1; i < static_cast<int>(samples.size()); ++i) {
                const int x1 = xForSampleIndex(i - 1, static_cast<int>(samples.size()), plotRect);
                const int x2 = xForSampleIndex(i, static_cast<int>(samples.size()), plotRect);
                const int y1 = yForValue(samples[i - 1].voltageMilliV, plotRect);
                const int y2 = yForValue(samples[i].voltageMilliV, plotRect);
                p.drawLine(x1, y1, x2, y2);
            }
        }

        // 绘制电流波形 - 带发光效果
        if (visibility_[idx].currentVisible) {
            // 发光效果
            QPen glowPen(QColor(0, 200, 255, 60), 4);
            p.setPen(glowPen);
            for (int i = 1; i < static_cast<int>(samples.size()); ++i) {
                const int x1 = xForSampleIndex(i - 1, static_cast<int>(samples.size()), plotRect);
                const int x2 = xForSampleIndex(i, static_cast<int>(samples.size()), plotRect);
                const int y1 = yForValue(samples[i - 1].currentMilliA, plotRect);
                const int y2 = yForValue(samples[i].currentMilliA, plotRect);
                p.drawLine(x1, y1, x2, y2);
            }
            
            // 主波形线
            p.setPen(QPen(QColor(0, 200, 255), 1.5));
            for (int i = 1; i < static_cast<int>(samples.size()); ++i) {
                const int x1 = xForSampleIndex(i - 1, static_cast<int>(samples.size()), plotRect);
                const int x2 = xForSampleIndex(i, static_cast<int>(samples.size()), plotRect);
                const int y1 = yForValue(samples[i - 1].currentMilliA, plotRect);
                const int y2 = yForValue(samples[i].currentMilliA, plotRect);
                p.drawLine(x1, y1, x2, y2);
            }
        }

        // 绘制最新数值
        const auto latest = samples.back();
        QFont valueFont = p.font();
        valueFont.setPointSize(9);
        valueFont.setBold(false);
        p.setFont(valueFont);
        
        int textY = plotRect.top() + 14;
        if (visibility_[idx].voltageVisible) {
            p.setPen(QColor(247, 215, 56));
            QString vText = QString::asprintf("%.3fV", latest.voltageMilliV / 1000.0);
            p.drawText(plotRect.left() + 8, textY, vText);
            textY += 16;
        }
        if (visibility_[idx].currentVisible) {
            p.setPen(QColor(0, 200, 255));
            QString aText = QString::asprintf("%.3fA", latest.currentMilliA / 1000.0);
            p.drawText(plotRect.left() + 8, textY, aText);
        }

        // 绘制图例
        int legendX = plotRect.right() - 80;
        int legendY = plotRect.top() + 10;
        
        if (visibility_[idx].voltageVisible) {
            p.setPen(QPen(QColor(247, 215, 56), 2));
            p.drawLine(legendX, legendY + 6, legendX + 12, legendY + 6);
            p.setPen(QColor(247, 215, 56));
            p.drawText(legendX + 16, legendY + 10, "V");
        }
        
        if (visibility_[idx].currentVisible) {
            int offset = visibility_[idx].voltageVisible ? 25 : 0;
            p.setPen(QPen(QColor(0, 200, 255), 1.5));
            p.drawLine(legendX + offset, legendY + 6, legendX + 12 + offset, legendY + 6);
            p.setPen(QColor(0, 200, 255));
            p.drawText(legendX + 16 + offset, legendY + 10, "A");
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
