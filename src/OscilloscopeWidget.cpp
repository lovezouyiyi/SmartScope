#include "OscilloscopeWidget.h"
#include <QPainter>
#include <QDebug>

OscilloscopeWidget::OscilloscopeWidget(DataEngine* e, TriggerEngine* t)
    : engine(e), trigger(t) {
    qDebug() << "OscilloscopeWidget constructor called";
}

void OscilloscopeWidget::paintEvent(QPaintEvent*) {
    try {
        QPainter p(this);
        p.fillRect(rect(), Qt::black);

        int w = width(), h = height();
        
        if (w <= 0 || h <= 0) {
            qDebug() << "Invalid widget size:" << w << "x" << h;
            return;
        }

        // 缃戞牸
        p.setPen(QColor(50, 50, 50));
        for(int i = 0; i <= 10; i++)
            p.drawLine(i * w / 10, 0, i * w / 10, h);

        for(int i = 0; i <= 8; i++)
            p.drawLine(0, i * h / 8, w, i * h / 8);

        auto data = engine->get();
        
        if(data.size() < 2) {
            qDebug() << "Not enough data to draw, size:" << data.size();
            return;
        }

        p.setPen(QPen(QColor(0, 255, 0), 2));

        for(size_t i = 1; i < data.size(); i++) {
            int x1 = (i - 1) * w / data.size();
            int x2 = i * w / data.size();

            int y1 = h - data[i-1].v * h / 5000;
            int y2 = h - data[i].v * h / 5000;

            p.drawLine(x1, y1, x2, y2);
        }

        // 瑙﹀彂绾?        p.setPen(Qt::red);
        p.drawLine(w / 2, 0, w / 2, h);
        
        qDebug() << "Paint completed, data points:" << data.size();
    } catch (const std::exception& e) {
        qDebug() << "Exception in paintEvent:" << e.what();
    } catch (...) {
        qDebug() << "Unknown exception in paintEvent";
    }
}
