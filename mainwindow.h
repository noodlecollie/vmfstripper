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
class QTableWidget;

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
    void chooseVMFFile();
    void chooseExportFile();
    void handleLogFileStatusChange(int status);
    void importVMFFile();
    void showTreeView();
    void exportJson();
    void exportVMF();
    
    void handleReplacementTableCellChanged(int row, int column);
    void removeCurrentReplacementEntry();
    void clearReplacementTable();
    
    void handleParentRemovalTableCellChanged(int row, int column);
    void removeCurrentParentRemovalTableEntry();
    void clearParentRemovalTable();
    
protected:
    virtual void closeEvent(QCloseEvent *);

private:
    void clearTableRow(int row);
    void setUpReplacementTableHeaders();
    void setUpParentRemovalTableHeaders();
    static QString replaceNewlinesWithLineBreaks(const QString &str);
    void handleTableCellChanged(QTableWidget* table, int row, int column);
    void removeCurrentEntry(QTableWidget* table);
    void clearTable(QTableWidget* table);
    void setUpExportOrderList();

    Ui::MainWindow *ui;
    QString m_szDefaultDir;
    QFile* m_pLogFile;
    QJsonDocument m_Document;
    JsonWidget* m_pJsonWidget;
    bool m_bJsonWidgetNeedsUpdate;
};

#endif // MAINWINDOW_H
