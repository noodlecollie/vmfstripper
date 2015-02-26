#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtDebug>
#include <QSet>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setUpTableHeaders();

    statusBar()->showMessage("Ready.");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::removeHighlightedEntitiesFromList()
{
    QList<QListWidgetItem*> list = ui->listObjectsToRemove->selectedItems();
    foreach ( QListWidgetItem* i, list ) delete i;
}

void MainWindow::addEntityToList()
{
    QString classname = ui->tbObjectToAdd->text().trimmed();
    if ( classname.isEmpty() ) return;

    classname = classname.toLower();
    ui->listObjectsToRemove->addItem(classname);
    ui->tbObjectToAdd->clear();
}

void MainWindow::handleTableCellChanged(int row, int)
{
    static bool shouldReturn = false;
    if ( shouldReturn ) return;

    // If there is no blank row at the end of the table, insert one.
    QTableWidget* t = ui->tableReplacement;
    int lastRow = t->rowCount() - 1;
    bool notEmpty = false;
    for ( int i = 0; i < t->columnCount(); i++ )
    {
        if ( !t->item(lastRow, i)->text().isEmpty() )
        {
            notEmpty = true;
            break;
        }
    }

    if ( notEmpty )
    {
        t->insertRow(t->rowCount());
        int newRow = t->rowCount() - 1;

        shouldReturn = true;
        for ( int i = 0; i < t->columnCount(); i++ )
        {
            t->setItem(newRow, i, new QTableWidgetItem());
        }
        shouldReturn = false;
    }

    // If all cells in the row are empty and this is not the last row, delete the row.
    if ( row == t->rowCount() - 1 ) return;
    for ( int i = 0; i < t->columnCount(); i++ )
    {
        if ( !t->item(row, i)->text().isEmpty() ) return;
    }

    shouldReturn = true;
    t->removeRow(row);
    shouldReturn = false;
}

void MainWindow::clearTableRow(int row)
{
    for ( int i = 0; i < ui->tableReplacement->columnCount(); i++ )
    {
        ui->tableReplacement->item(row, i)->setText(QString());
    }
}

void MainWindow::removeCurrentReplacementEntry()
{
    QTableWidget* t = ui->tableReplacement;

    QList<QTableWidgetSelectionRange> selection = t->selectedRanges();
    if ( selection.count() < 1 ) return;

    QSet<int> rowSet;
    foreach ( QTableWidgetSelectionRange range, selection )
    {
        for ( int i = range.topRow(); i <= range.bottomRow(); i++ )
        {
            rowSet.insert(i);
        }
    }

    QList<int> rowList = rowSet.toList();

    // We now have a list of rows to remove.
    // Remove them one at a time, fixing up those that are modified by each removal.
    while ( rowList.count() > 0 )
    {
        // Get the row to remove.
        int rowToRemove = rowList.takeFirst();

        // Remove it from the table.
        if ( rowToRemove < t->rowCount() - 1 ) t->removeRow(rowToRemove);
        else clearTableRow(rowToRemove);

        // For the rest of the rows in the list, decrease them by 1 if they were
        // greater than the row we just removed.
        for ( int i = 0; i < rowList.count(); i++ )
        {
            if ( rowList.at(i) > rowToRemove ) rowList.replace(i, rowList.at(i) - 1);
        }
    }
}

void MainWindow::setUpTableHeaders()
{
    QStringList headers;
    headers << "For key:" << "Replace:" << "With:";
    ui->tableReplacement->setHorizontalHeaderLabels(headers);
    ui->tableReplacement->horizontalHeader()->show();
}

void MainWindow::clearReplacementTable()
{
    ui->tableReplacement->clear();
    ui->tableReplacement->setRowCount(1);
    setUpTableHeaders();
}
