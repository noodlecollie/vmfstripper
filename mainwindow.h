#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtMsgHandler>
#include <QFile>

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
    void handleLogFileStatusChange(int status);

private:
    void clearTableRow(int row);
    void setUpTableHeaders();

    Ui::MainWindow *ui;
    QString m_szDefaultDir;
    QFile* m_pLogFile;
};

#endif // MAINWINDOW_H
