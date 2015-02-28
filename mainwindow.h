#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtMsgHandler>
#include <QFile>
#include <QJsonDocument>
#include "jsonwidget.h"

namespace Ui {
class MainWindow;
}

class QTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
    void receiveLogMessage(QtMsgType type, const QString &msg);

public slots:
    void removeHighlightedEntitiesFromList();
    void addEntityToList();
    void handleTableCellChanged(int row, int column);
    void removeCurrentReplacementEntry();
    void clearReplacementTable();
    void chooseVMFFile();
    void chooseExportFile();
    void handleLogFileStatusChange(int status);
    void importVMFFile();
    void showTreeView();
    
protected:
    virtual void closeEvent(QCloseEvent *);

private:
    void clearTableRow(int row);
    void setUpTableHeaders();
    static QString replaceNewlinesWithLineBreaks(const QString &str);

    Ui::MainWindow *ui;
    QString m_szDefaultDir;
    QFile* m_pLogFile;
    QJsonDocument m_Document;
    JsonWidget* m_pJsonWidget;
};

#endif // MAINWINDOW_H
