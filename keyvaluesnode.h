#ifndef KEYVALUESNODE_H
#define KEYVALUESNODE_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QColor>
#include <QVariant>

class KeyValuesParser;

class KeyValuesNode : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString key READ key WRITE setKey NOTIFY keyChanged)
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)

    friend class KeyValuesParser;
public:
    explicit KeyValuesNode(KeyValuesNode* parent = 0);
    explicit KeyValuesNode(const QString &key, KeyValuesNode* parent = 0);
    explicit KeyValuesNode(const QString &key, const QVariant &value, KeyValuesNode* parent = 0);
    explicit KeyValuesNode(const QString &key, const QString &value, KeyValuesNode* parent = 0);
    explicit KeyValuesNode(const QString &key, int value, KeyValuesNode* parent = 0);
    explicit KeyValuesNode(const QString &key, float value, KeyValuesNode* parent = 0);
    explicit KeyValuesNode(const QString &key, const QColor &value, KeyValuesNode* parent = 0);
    
    inline QList<KeyValuesNode*> childNodes() const
    {
        return findChildren<KeyValuesNode*>(QString(), Qt::FindDirectChildrenOnly);
    }

    inline int childCount() const { return childNodes.count(); }
    
    inline KeyValuesNode* parentNode() const
    {
        return qobject_cast<KeyValuesNode*>(parent());
    }
    
    QVariant value() const;
    void setValue(const QVariant &value);
    void clearValue();
    
    void setValue(const QString &val);
    void setValue(int value);
    void setValue(float value);
    void setValue(const QColor &value);
    
    QString toString() const;
    int toInt(bool* ok = NULL) const;
    float toFloat(bool* ok = NULL) const;
    QColor toColor() const;
    
    QString key() const;
    void setKey(const QString &key);
    
    bool isKeyValid() const;
    bool isValueValid() const;
    inline bool isValid() const { return isKeyValid() && (isValueValid() || childCount() > 0 ); }

    inline bool containerNode() const { return childCount() > 0; }
    
signals:
    void valueChanged(const QVariant&);
    void keyChanged(const QString&);
    
public slots:
    
private:
    static QString cleanString(const QString &str);
    
    QString     m_szKey;
    QVariant    m_varValue;
};

#endif // KEYVALUESNODE_H
