#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtDebug>
#include <QSet>
#include <QFileDialog>
#include <QFileInfo>
#include <QTextStream>
#include <QTime>
#include <QDate>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_pLogFile = NULL;
    handleLogFileStatusChange(ui->cbLogToFile->checkState());
    setUpTableHeaders();
    m_szDefaultDir = qApp->applicationDirPath();

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

void MainWindow::chooseVMFFile()
{
    QString file = QFileDialog::getOpenFileName(this, "Chose file", m_szDefaultDir, "*.vmf");
    if ( file.isNull() )
    {
        ui->tbFilename->setText(QString());
        ui->labelFileSize->setText("0 bytes");
        statusBar()->showMessage("Choosing file failed.");
        return;
    }
    
    ui->tbFilename->setText(file);
    QFileInfo info(file);
    m_szDefaultDir = info.canonicalFilePath();
    ui->labelFileSize->setText(QString("%0 bytes").arg(info.size()));
    
    statusBar()->showMessage(QString("Chose file: ") + ui->tbFilename->text());
    qDebug() << "Chose file:" << ui->tbFilename->text();
}

void MainWindow::receiveLogMessage(QtMsgType type, const QString &msg)
{
    QString rich;
    
    switch (type)
    {
        case QtWarningMsg:
        {
            rich = QString("<span style='color:#510099;font-family:\"Lucida Console\",monospace;'>[%0-%1] %2</span>")
                    .arg(QDate::currentDate().toString("yyyy:MM:dd")).arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
            break;
        }
        case QtCriticalMsg:
        {
            rich = QString("<span style='color:#F00;font-family:\"Lucida Console\",monospace;'>[%0-%1] %2</span>")
                    .arg(QDate::currentDate().toString("yyyy:MM:dd")).arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
            break;
        }
        case QtFatalMsg:
        {
            rich = QString("<span style='color:#F00;font-family:\"Lucida Console\",monospace;'>[%0-%1] %2</span>")
                    .arg(QDate::currentDate().toString("yyyy:MM:dd")).arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
            break;
        }
        default:
        {
            rich = QString("<span style='color:#000;font-family:\"Lucida Console\",monospace;'>[%0-%1] %2</span>")
                    .arg(QDate::currentDate().toString("yyyy:MM:dd")).arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
            break;
        }
    }
    
    ui->logWindow->setText(ui->logWindow->toHtml() + rich);
    ui->logWindow->ensureCursorVisible();
    
    if ( m_pLogFile && m_pLogFile->isOpen() )
    {
        QTextStream stream(m_pLogFile);
        stream << msg;
    }
}

void MainWindow::handleLogFileStatusChange(int status)
{
    // Status is 2 for checked and 0 for unchecked.
    if ( status == 2 )
    {
        if ( !m_pLogFile )
        {
            m_pLogFile = new QFile(qApp->applicationDirPath() + QString("output.log"));
        }
        
        if ( !m_pLogFile->isOpen() )
        {
            if ( !m_pLogFile->open(QIODevice::Append) ) return;
        }
    }
    else
    {
        if ( m_pLogFile && m_pLogFile->isOpen() )
        {
            m_pLogFile->close();
            delete m_pLogFile;
            m_pLogFile = NULL;
        }
    }
}
