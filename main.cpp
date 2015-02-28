#if 0
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
#include <QFile>
#include "keyvaluesparsernew.h"
#include <QDir>
#include <QtDebug>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    QDir dir(qApp->applicationDirPath());
    dir.cdUp();
    dir.cdUp();
    dir.cd("vmfstripper");
    
    QFile file(dir.canonicalPath() + "/test_original.vmf");
    if ( file.open(QIODevice::ReadOnly) )
    {
        QByteArray json;
        KeyValuesParserNew::simpleKeyValuesToJson(file.readAll(), json);
        
        QFile out(dir.canonicalPath() + "/output.json");
        if ( out.open(QIODevice::WriteOnly) )
        {
            out.write(json);
            out.close();
        }
        
        file.close();
    }
    
    return 0;
}
#endif
