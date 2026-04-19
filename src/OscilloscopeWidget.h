#pragma once
#include <QWidget>
#include "DataEngine.h"
#include "TriggerEngine.h"

class OscilloscopeWidget : public QWidget {
public:
    OscilloscopeWidget(DataEngine* e, TriggerEngine* t);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    DataEngine* engine;
    TriggerEngine* trigger;
};

//sdsf
