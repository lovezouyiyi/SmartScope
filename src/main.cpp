#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include "MainWindow.h"

// 鏃ュ織鏂囦欢璺緞
static const QString LOG_FILE = "smartscope_debug.log";

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QFile logFile(LOG_FILE);
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&logFile);
        QString typeName;
        switch (type) {
            case QtDebugMsg: typeName = "DEBUG"; break;
            case QtInfoMsg: typeName = "INFO"; break;
            case QtWarningMsg: typeName = "WARNING"; break;
            case QtCriticalMsg: typeName = "CRITICAL"; break;
            case QtFatalMsg: typeName = "FATAL"; break;
        }
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
            << " [" << typeName << "] " << msg << "\n";
        logFile.close();
    }
}

int main(int argc, char *argv[]) {
    // 瀹夎鑷畾涔夋秷鎭鐞嗗櫒
    qInstallMessageHandler(messageHandler);
    
    qDebug() << "=== SmartScope Starting ===";
    qDebug() << "Qt Version:" << QT_VERSION_STR;
    
    try {
        QApplication a(argc, argv);
        qDebug() << "QApplication created successfully";
        
        MainWindow w;
        qDebug() << "MainWindow created successfully";
        
        w.show();
        qDebug() << "MainWindow shown";
        
        int result = a.exec();
        qDebug() << "Application exited with code:" << result;
        return result;
    } catch (const std::exception& e) {
        qDebug() << "Exception caught:" << e.what();
        return -1;
    } catch (...) {
        qDebug() << "Unknown exception caught";
        return -1;
    }
}
