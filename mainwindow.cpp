#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtDebug>
#include <QSet>
#include <QFileDialog>
#include <QFileInfo>
#include <QTextStream>
#include <QTime>
#include <QDate>
#include "keyvaluesnode.h"
#include <QMessageBox>
#include "keyvaluesparsernew.h"
#include "loadvmfdialogue.h"
#include <QTime>
#include <QCloseEvent>

#define STYLESHEET_FAILED       "QLabel { background-color : #D63742; }"
#define STYLESHEET_SUCCEEDED    "QLabel { background-color : #6ADB64; }"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_pJsonWidget = new JsonWidget();
    m_pJsonWidget->setMinimumSize(QSize(800,600));
    m_pJsonWidget->setMaximumSize(QSize(800,600));
    m_pJsonWidget->setObjectName("Tree View");
    
    m_pLogFile = NULL;
    ui->labelIsImported->setStyleSheet(STYLESHEET_FAILED);
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
    QString file = QFileDialog::getOpenFileName(this, "Chose file", m_szDefaultDir, tr("Valve Map File (*.vmf)"));
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
    
    ui->btnChooseOutput->setEnabled(true);
    ui->btnImport->setEnabled(true);
    QString newFileName = info.completeBaseName() + QString("_stripped.") + info.suffix();
    ui->tbOutputFile->setText(info.canonicalFilePath() + QString("/") + newFileName);
    
    m_Document = QJsonDocument();
    
    ui->labelIsImported->setText("Not Imported");
    ui->labelIsImported->setStyleSheet(STYLESHEET_FAILED);
    
    statusBar()->showMessage(QString("Chose file: ") + ui->tbFilename->text());
    qDebug() << "Chose file:" << ui->tbFilename->text();
    qDebug() << "Output file:" << ui->tbOutputFile->text();
}

void MainWindow::chooseExportFile()
{
    QString file = QFileDialog::getSaveFileName(this, "Choose output file", m_szDefaultDir + QString("/%0").arg(ui->tbOutputFile->text()), tr("Valve Map File (*.vmf)"));
    if ( file.isNull() ) return;
    
    ui->tbOutputFile->setText(file);
    statusBar()->showMessage(QString("Chose output file: ") + ui->tbOutputFile->text());
    qDebug() << "Chose output file:" << ui->tbOutputFile->text();
}

QString MainWindow::replaceNewlinesWithLineBreaks(const QString &str)
{
    QString s = str;
    s.replace('\n', "<br/>");
    return s;
}

void MainWindow::receiveLogMessage(QtMsgType type, const QString &msg)
{
    QString rich;
    QString newmsg = replaceNewlinesWithLineBreaks(msg);
    
    switch (type)
    {
        case QtWarningMsg:
        {
            rich = QString("<span style='color:#510099;font-family:\"Lucida Console\",monospace;'>[%0-%1] %2</span>")
                    .arg(QDate::currentDate().toString("yyyy:MM:dd")).arg(QTime::currentTime().toString("hh:mm:ss")).arg(newmsg);
            break;
        }
        case QtCriticalMsg:
        {
            rich = QString("<span style='color:#F00;font-family:\"Lucida Console\",monospace;'>[%0-%1] %2</span>")
                    .arg(QDate::currentDate().toString("yyyy:MM:dd")).arg(QTime::currentTime().toString("hh:mm:ss")).arg(newmsg);
            break;
        }
        case QtFatalMsg:
        {
            rich = QString("<span style='color:#F00;font-family:\"Lucida Console\",monospace;'>[%0-%1] %2</span>")
                    .arg(QDate::currentDate().toString("yyyy:MM:dd")).arg(QTime::currentTime().toString("hh:mm:ss")).arg(newmsg);
            break;
        }
        default:
        {
            rich = QString("<span style='color:#000;font-family:\"Lucida Console\",monospace;'>[%0-%1] %2</span>")
                    .arg(QDate::currentDate().toString("yyyy:MM:dd")).arg(QTime::currentTime().toString("hh:mm:ss")).arg(newmsg);
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

void MainWindow::importVMFFile()
{
    QString filename = ui->tbFilename->text().trimmed();
    if ( filename.isEmpty() ) return;
    
    QFile file(filename);
    if ( !file.open(QIODevice::ReadOnly) )
    {
        QMessageBox::critical(this, "Error", "Unable to open the specified file for reading.");
        
        statusBar()->showMessage("Import failed.");
        qDebug() << "Import failed: unable to open file for reading.";
        ui->labelIsImported->setText("Not Imported");
        ui->labelIsImported->setStyleSheet(STYLESHEET_FAILED);
        m_Document = QJsonDocument();
    }
    
    QFileInfo info(file);
    qint64 fileSize = info.size();
    
    statusBar()->showMessage(QString("Import initiated."));
    qDebug() << "Import initiated.";
    
    KeyValuesParserNew parser;
    
    LoadVmfDialogue dialogue(false, this);
    dialogue.setModal(true);
    dialogue.setMessage("Importing...");
    dialogue.update();
    dialogue.show();
    QApplication::processEvents();
    
    QByteArray content = file.readAll();
    file.close();
    
    QTime timer;
    timer.start();
    QString snapshot;
    QJsonParseError error = parser.jsonFromKeyValues(content, m_Document, &snapshot);
    int elapsed = timer.elapsed();
    
    if ( error.error != QJsonParseError::NoError )
    {
        QMessageBox::critical(this, "Import failed", "The VMF import failed - see the log for a full description.");
        
        statusBar()->showMessage(QString("Import failed, reason: \"%0\"").arg(error.errorString()));
        qDebug().nospace() << "VMF import failed. The JSON parser reported: " << error.errorString() <<  " at position " << error.offset << " after keyvalues conversion to JSON.\n"
                           << "The related portion of the generated JSON is:\n\n"
                           << "" << snapshot.toLatin1().constData() << "\n"
                           << "----------^----------\n\n"
                           << "This is probably due to a malformed VMF file. At some point there'll be keyvalues syntax checking performed beforehand, but for now make sure the"
                           << "files provided to the importer are valid.";
        
        ui->labelIsImported->setText("Not Imported");
        ui->labelIsImported->setStyleSheet(STYLESHEET_FAILED);
        m_Document = QJsonDocument();
        dialogue.close();
        return;
    }
    
    dialogue.setMessage("Populating tree view...");
    dialogue.updateProgressBar(0.8f);
    dialogue.update();
    QApplication::processEvents();
    m_pJsonWidget->readFrom(m_Document);
    
    ui->labelIsImported->setText("Imported");
    ui->labelIsImported->setStyleSheet(STYLESHEET_SUCCEEDED);
    statusBar()->showMessage("Import succeeded.");
    float seconds = (float)elapsed/1000.0f;
    qDebug().nospace() << "Import succeeded: processed " << fileSize << " bytes in " << seconds << "seconds (" << (float)fileSize/seconds << "bytes/sec)";
    dialogue.close();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    m_pJsonWidget->close();
    QMainWindow::closeEvent(e);
}

void MainWindow::showTreeView()
{
    LoadVmfDialogue dialogue(true, this);
    dialogue.setMessage("Loading tree view...");
    dialogue.show();
    QApplication::processEvents();
    m_pJsonWidget->show();
    dialogue.close();
}
