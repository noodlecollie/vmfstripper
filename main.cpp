#if 1
#include "mainwindow.h"
#include <QApplication>
#include <QTextEdit>

MainWindow* mainWin = NULL;

void messageHandler(QtMsgType, const QMessageLogContext &, const QString &);

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    
    mainWin = &w;
    qInstallMessageHandler(messageHandler);
    
    w.show();

    return a.exec();
}

void messageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    if ( !mainWin ) return;
    
    switch (type)
    {
        case QtDebugMsg:
        {
            mainWin->receiveLogMessage(type, msg);
            break;
        }
        case QtWarningMsg:
        {
            mainWin->receiveLogMessage(type, msg);
            break;
        }
        case QtCriticalMsg:
        {
            mainWin->receiveLogMessage(type, msg);
            break;
        }
        case QtFatalMsg:
        {
            mainWin->receiveLogMessage(type, msg);
            abort();
        }
    }
}
#else
#include <QApplication>
#include <QJsonObject>
#include <QtDebug>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QJsonObject o;
    o.insert("A key", QJsonValue("A value"));

    *(o.begin()) = QJsonValue("Changed value");

    qDebug() << o;

    return 0;
}
#endif
