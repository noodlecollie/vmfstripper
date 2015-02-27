#ifndef LOADVMFDIALOGUE_H
#define LOADVMFDIALOGUE_H

#include <QDialog>

namespace Ui {
class LoadVmfDialogue;
}

class LoadVmfDialogue : public QDialog
{
    Q_OBJECT
    
public:
    explicit LoadVmfDialogue(QWidget *parent = 0);
    ~LoadVmfDialogue();
    
signals:
    void cancelPressed();
    
public slots:
    void updateProgressBar(float value);
    void updateByteProgress(int current, int total);
    void setMessage(const QString &msg);
    
private:
    void updateMessage();
    
    Ui::LoadVmfDialogue *ui;
    QString m_szMessage;
    int m_iCurrentBytes;
    int m_iTotalBytes;
};

#endif // LOADVMFDIALOGUE_H
