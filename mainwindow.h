#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void removeHighlightedEntitiesFromList();
    void addEntityToList();
    void handleTableCellChanged(int row, int column);
    void removeCurrentReplacementEntry();
    void clearReplacementTable();

private:
    void clearTableRow(int row);
    void setUpTableHeaders();

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
