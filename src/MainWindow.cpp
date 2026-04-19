#include "MainWindow.h"
#include "OscilloscopeWidget.h"
#include <QDebug>

MainWindow::MainWindow() {
    qDebug() << "MainWindow constructor started";
    
    try {
        auto* osc = new OscilloscopeWidget(&engine, &trigger);
        qDebug() << "OscilloscopeWidget created";
        
        setCentralWidget(osc);
        qDebug() << "Central widget set";
        
        resize(1000, 600);
        qDebug() << "Window resized to 1000x600";

        QTimer* t = new QTimer(this);
        qDebug() << "QTimer created";

        connect(t, &QTimer::timeout, [&]() {
            static int v = 0;
            v = (v + 300) % 5000;

            engine.push(v);
            trigger.push(v);

            if(trigger.ready()) {
                trigger.reset();
            }

            osc->update();
        });
        qDebug() << "Timer signal connected";

        t->start(20);
        qDebug() << "Timer started with 20ms interval";
        
        qDebug() << "MainWindow constructor completed successfully";
    } catch (const std::exception& e) {
        qDebug() << "Exception in MainWindow constructor:" << e.what();
        throw;
    } catch (...) {
        qDebug() << "Unknown exception in MainWindow constructor";
        throw;
    }
}
