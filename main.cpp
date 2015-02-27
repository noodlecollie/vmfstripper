#include "mainwindow.h"
#include <QApplication>
#include <QTextEdit>

#if 0
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
#include <keyvaluesparser.h>
#include <QFile>
#include <QDir>
#include <QtDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QDir d(qApp->applicationDirPath());
    d.cdUp();
    d.cdUp();
    d.cdUp();
    d.cdUp();
    d.cd("vmfstripper");

    QFile f(d.canonicalPath() + QString("/test.vmf"));

    return 0;
}
#endif
