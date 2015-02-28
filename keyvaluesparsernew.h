#ifndef KEYVALUESPARSERNEW_H
#define KEYVALUESPARSERNEW_H

#include <QObject>
#include "keyvaluestoken.h"
#include <QJsonDocument>

class KeyValuesParserNew : public QObject
{
    Q_OBJECT
public:
    explicit KeyValuesParserNew(QObject *parent = 0);
    
    QJsonParseError jsonFromKeyValues(const QByteArray &keyValues, QJsonDocument &document, QString* errorSnapshot = NULL);
    
signals:
    
public slots:
    
private:
    // If the keyvalues file is valid, the JSON file will be valid.
    // There are no guarantees the other way round.
    static void simpleKeyValuesToJson(const QByteArray &keyValues, QByteArray &output);
    
    // Returns false if the end of the array was reached.
    static bool getNextToken(const QByteArray &array, int from, KeyValuesToken &token);
    
    static inline bool isWhitespace(char ch)
    {
        return ( ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t' );
    }
    
    static inline bool isAlphaNumeric(char ch)
    {
        return ( (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= 0x30 && ch <= 0x39) || ch == '_' );
    }
    
    // Assumes ch is not null and that ch+1 is valid.
    static inline bool isCommentMarker(const char* ch)
    {
        return ( *ch == '/' && *(ch+1) == '/' );
    }
    
    // These assume the characters at position pos are correct for the token type.
    static bool handleCommentToken(const QByteArray &array, int pos, KeyValuesToken &token);
    static bool handlePushToken(const QByteArray &array, int pos, KeyValuesToken &token);
    static bool handlePopToken(const QByteArray &array, int pos, KeyValuesToken &token);
    static bool handleUnquotedStringToken(const QByteArray &array, int pos, KeyValuesToken &token);
    static bool handleQuotedStringToken(const QByteArray &array, int pos, KeyValuesToken &token);
    static bool handleInvalidToken(const QByteArray &array, int pos, KeyValuesToken &token);
    
    static void writeTokenToArray(QByteArray &array, const KeyValuesToken &token);
};

#endif // KEYVALUESPARSERNEW_H
