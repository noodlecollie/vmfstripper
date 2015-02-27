#ifndef KEYVALUESPARSER_H
#define KEYVALUESPARSER_H

#include <QObject>
#include <QPair>
#include <QString>
#include <QStringRef>
#include <QStack>

class KeyValuesNode;

class KeyValuesParser : public QObject
{
    Q_OBJECT
public:
    enum ParseError
    {
        NoError = 0,            // Everything went OK.
        NoContentError,         // There was no parsable content passed.
        InvalidTokenError,      // An invalid token was encountered.
        StackUnderflowError,    // There were more pops than pushes in the file.
        UnnamedNodeError,       // A push or pop occurred before a node had been given a key.
        IncompleteNodeError,    // A key was given with no corresponding value.
        UnmatchedBraceError,    // Parsing terminated before the last push was matched with a pop.
        UnspecifiedError,       // An unspecified error occurred, something is probably wrong.
    };

    explicit KeyValuesParser(QObject *parent = 0);

    static ParseError parse(const QString &content, KeyValuesNode &container);

signals:

public slots:

private:
    enum TokenType
    {
        TokenInvalid,   // The token is not valid.
        TokenPlain,     // The token is an unquoted alphanumeric string.
        TokenQuoted,    // The token is a quoted string.
        TokenPush,      // The token is a push bracket {.
        TokenPop,       // The token is a pop bracket }.
    };

    enum State
    {
        StateBase,
        StateElevated,
        StateFail
    };

    // If the end of the string is reached, begin and end are set to -1.
    static void identifyNextToken(const QStringRef &content, int &begin, int &end);

    // Converts "\"" to '"', "\n" to '\n', "\t" to '\t'.
    static QString unescapeToken(const QStringRef &token);

    // Trims preceding and ending whitespace and removes surrounding quotes.
    static QString unquoteToken(const QStringRef &token);

    // Assumed token is an output from identifyNextToken().
    static TokenType classifyToken(const QStringRef &token);

    static inline QString segment(const QStringRef &str, int begin, int end)
    {
        return begin < 0 ? QString() :str.mid(begin, end - begin + 1);
    }

    static QStringRef segmentRef(const QStringRef &str, int begin, int end)
    {
        return begin < 0 ? QStringRef() :str.midRef(begin, end - begin + 1);
    }

    ParseError handleTokenPush(const QStringRef &str, QStack<KeyValuesNode*> &stack);
    ParseError handleTokenPop(const QStringRef &str, QStack<KeyValuesNode*> &stack);
    ParseError handleTokenPlain(const QStringRef &str, QStack<KeyValuesNode*> &stack);
    ParseError handleTokenQuoted(const QStringRef &str, QStack<KeyValuesNode*> &stack);
    ParseError handleTokenGeneric(const QStringRef &str, QStack<KeyValuesNode*> &stack);

    State   m_iState;
    QString m_szLastToken;
};

#endif // KEYVALUESPARSER_H
