#include "keyvaluesnode.h"
#include <QRegularExpression>

KeyValuesNode::KeyValuesNode(KeyValuesNode *parent) :
    QObject(parent), m_szKey(), m_varValue()
{
}

KeyValuesNode::KeyValuesNode(const QString &key, KeyValuesNode *parent) :
    QObject(parent), m_szKey()
{
    setKey(key);
}

KeyValuesNode::KeyValuesNode(const QString &key, const QColor &value, KeyValuesNode *parent) :
    QObject(parent), m_szKey(), m_varValue()
{
    setKey(key);
    setValue(value);
}

KeyValuesNode::KeyValuesNode(const QString &key, const QString &value, KeyValuesNode *parent) :
    QObject(parent), m_szKey(), m_varValue(value)
{
    setKey(key);
}

KeyValuesNode::KeyValuesNode(const QString &key, const QVariant &value, KeyValuesNode *parent) :
    QObject(parent), m_szKey(), m_varValue(value)
{
    setKey(key);
}

KeyValuesNode::KeyValuesNode(const QString &key, float value, KeyValuesNode *parent) :
    QObject(parent), m_szKey(), m_varValue(value)
{
    setKey(key);
}

KeyValuesNode::KeyValuesNode(const QString &key, int value, KeyValuesNode *parent) :
    QObject(parent), m_szKey(), m_varValue(value)
{
    setKey(key);
}

QString KeyValuesNode::toString() const
{
    return m_varValue.toString();
}

int KeyValuesNode::toInt(bool *ok) const
{
    return m_varValue.toInt(ok);
}

float KeyValuesNode::toFloat(bool *ok) const
{
    return m_varValue.toFloat(ok);
}

QColor KeyValuesNode::toColor() const
{
    return m_varValue.value<QColor>();
}

void KeyValuesNode::setValue(const QString &val)
{
    if ( toString() == val ) return;
    
    m_varValue.setValue<QString>(val);
    emit valueChanged(m_varValue);
}

void KeyValuesNode::setValue(const QColor &value)
{
    if ( toColor() == value ) return;
    
    m_varValue.setValue<QColor>(value);
    emit valueChanged(m_varValue);
}

void KeyValuesNode::setValue(float value)
{
    bool b = false;
    if ( toFloat(&b) == value && b ) return;
    
    m_varValue.setValue<float>(value);
    emit valueChanged(m_varValue);
}

void KeyValuesNode::setValue(int value)
{
    bool b = false;
    if ( toInt(&b) == value && b ) return;
    
    m_varValue.setValue<int>(value);
    emit valueChanged(m_varValue);
}

void KeyValuesNode::setValue(const QVariant &value)
{
    if ( m_varValue == value ) return;
    
    m_varValue = value;
    emit valueChanged(m_varValue);
}

QVariant KeyValuesNode::value() const
{
    return m_varValue;
}

void KeyValuesNode::clearValue()
{
    if ( !isValueValid() ) return;

    m_varValue = QVariant();
    emit valueChanged(m_varValue);
}

bool KeyValuesNode::isKeyValid() const
{
    return m_szKey.isNull();
}

bool KeyValuesNode::isValueValid() const
{
    return m_varValue.isNull();
}

QString KeyValuesNode::key() const
{
    return m_szKey;
}

void KeyValuesNode::setKey(const QString &key)
{
    QString str = cleanString(key);
    if ( str.isEmpty() ) return;
    
    if ( str == m_szKey ) return;
    
    m_szKey = str;
    emit keyChanged(m_szKey);
}

QString KeyValuesNode::cleanString(const QString &str)
{
    // What we want to do here:
    // - Trim the string.
    // - If the first character is not '"', truncate at the first control character.
    // - If the first character is '"', truncate at the next non-escaped '"'.
    
    QString s = str.trimmed();
    if ( s.at(0) == '"' )
    {
        // Find the next non-escaped '"'.
        // (?<!\\)\"
        QRegularExpression ex("(?<!\\\\)\\\"");
        int index = s.indexOf(ex, 1);
        if ( index >= 0 ) return s.mid(1, index-1);
        
        // If there was no non-escaped quote, strip the start character
        // and process as if the new string is unquoted.
        s = s.right(s.length() - 1);
    }
    
    // Find the first control character.
    QRegularExpression ex("[\\\"\\{\\}\\s]");
    int index = s.indexOf(ex);
    return index < 0 ? s : s.left(index);
}
