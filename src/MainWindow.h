#pragma once
#include <QMainWindow>
#include <QTimer>
#include "DataEngine.h"
#include "TriggerEngine.h"

class OscilloscopeWidget;

class MainWindow : public QMainWindow {
public:
    MainWindow();
private:
    DataEngine engine;
    TriggerEngine trigger;
};
