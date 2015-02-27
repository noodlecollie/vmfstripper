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
