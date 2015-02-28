#include "loadvmfdialogue.h"
#include "ui_loadvmfdialogue.h"

LoadVmfDialogue::LoadVmfDialogue(bool marquee, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoadVmfDialogue)
{
    ui->setupUi(this);
    m_iCurrentBytes = -1;
    m_iTotalBytes = -1;
    
    setMarquee(marquee);
}

LoadVmfDialogue::~LoadVmfDialogue()
{
    delete ui;
}

void LoadVmfDialogue::updateProgressBar(float value)
{
    if ( ui->progressBar->minimum() == 0 && ui->progressBar->maximum() == 0 ) return;
    ui->progressBar->setValue((int)(value * (float)ui->progressBar->maximum()));
}

void LoadVmfDialogue::setMessage(const QString &msg)
{
    m_szMessage = msg;
    updateMessage();
}

void LoadVmfDialogue::updateByteProgress(int current, int total)
{
    m_iCurrentBytes = current;
    m_iTotalBytes = total;
    updateMessage();
}

void LoadVmfDialogue::updateMessage()
{
    if ( m_iCurrentBytes < 0 || m_iTotalBytes < 0 ) ui->label->setText(QString("<center>%0</center>").arg(m_szMessage));
    else
    {
        QString bytes = QString("Processed %0 of %1 bytes").arg(m_iCurrentBytes).arg(m_iTotalBytes);
        ui->label->setText(QString("<center>%0</center>\n<center>%1</center>").arg(m_szMessage).arg(bytes));
    }
}

void LoadVmfDialogue::setMarquee(bool enabled)
{
    if ( enabled )
    {
        ui->progressBar->setMinimum(0);
        ui->progressBar->setMaximum(0);
    }
    else
    {
        ui->progressBar->setMinimum(0);
        ui->progressBar->setMaximum(100);
    }
}
