#ifndef KEYVALUESPARSER_H
#define KEYVALUESPARSER_H

#include <QObject>
#include <QPair>
#include <QString>
#include <QStringRef>
#include <QStack>
#include <QStringList>

class KeyValuesNode;

class KeyValuesParser : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool sendUpdates READ sendUpdates WRITE setSendUpdates NOTIFY sendUpdatesStatusChanged)
    Q_PROPERTY(bool interruptable READ interruptable WRITE setInterruptable NOTIFY interruptableStatusChanged)
    Q_PROPERTY(bool parseComments READ parseComments WRITE setParseComments NOTIFY parseCommentsStatusChanged)
public:
    enum ParseError
    {
        NoError = 0,                // Everything went OK.
        NoContentError,             // There was no parsable content passed.
        InvalidTokenError,          // An invalid token was encountered.
        StackUnderflowError,        // There were more pops than pushes in the file.
        UnnamedNodeError,           // A push or pop occurred before a node had been given a key.
        IncompleteNodeError,        // A key was given with no corresponding value.
        UnmatchedBraceError,        // Parsing terminated before the last push was matched with a pop.
        OperationCancelledError,    // The parsing was cancelled externally.
        UnspecifiedError,           // An unspecified error occurred, something is probably wrong.
    };

    explicit KeyValuesParser(QObject *parent = 0);

    ParseError parse(const QString &content, KeyValuesNode &container);

    bool sendUpdates() const;
    void setSendUpdates(bool enabled);
    
    bool interruptable() const;
    void setInterruptable(bool enabled);
    
    bool parseComments() const;
    void setParseComments(bool enabled);
    
    void parseError(int &error, QString &title, QString &description, int &lineNo, int &charNo) const;
    
    static void stringForParseError(ParseError error, QString &title, QString &description);

signals:
    void sendUpdatesStatusChanged(bool);
    void interruptableStatusChanged(bool);
    void parseCommentsStatusChanged(bool);

    // Emitted each time the base state is reached again from another state.
    // Note that the progress value of 1 will be emitted before the parse() function has returned -
    // do not use this signal as a cue that the entire parsing process has finished.
    void parseUpdate(float);
    void byteProgress(int,int); // current, total

public slots:
    void cancelParsing();

private:
    enum TokenType
    {
        TokenInvalid,   // The token is not valid.
        TokenPlain,     // The token is an unquoted alphanumeric string.
        TokenQuoted,    // The token is a quoted string.
        TokenPush,      // The token is a push bracket {.
        TokenPop,       // The token is a pop bracket }.
        TokenComment,   // The token is a line comment
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
    
    // Takes a token beginning with "//" and extracts the comment content up until the newline.
    static QString extractComment(const QStringRef &token);
    
    // Used to convert the pending comment list to a single string;
    static QString stringListToMultilineString(const QStringList &list);

    // Assumed token is an output from identifyNextToken().
    // This is not supposed to be a foolproof classification for just any old token.
    TokenType classifyToken(const QStringRef &token);

    static inline QString segment(const QStringRef &str, int begin, int end)
    {
        return begin < 0 ? QString() :str.toString().mid(begin, end - begin + 1);
    }

    static QStringRef segmentRef(const QStringRef &str, int begin, int end)
    {
        return begin < 0 ? QStringRef() :str.mid(begin, end - begin + 1);
    }

    ParseError handleTokenPush(const QStringRef &str, QStack<KeyValuesNode*> &stack);
    ParseError handleTokenPop(const QStringRef &str, QStack<KeyValuesNode*> &stack);
    ParseError handleTokenPlain(const QStringRef &str, QStack<KeyValuesNode*> &stack);
    ParseError handleTokenQuoted(const QStringRef &str, QStack<KeyValuesNode*> &stack);
    ParseError handleTokenGeneric(const QStringRef &str, QStack<KeyValuesNode*> &stack);
    ParseError handleTokenComment(const QStringRef &str, QStack<KeyValuesNode*> &stack);

    void setState(State s);
    void updateProgress(float progress);
    void characterPosition(int pos, int& line, int& character) const;
    void updateNewlineBookkeeping(const QString &content, int furthestChar);

    State   m_iState;
    QString m_szLastToken;
    bool    m_bSendUpdates;
    float   m_flProgress;
    bool    m_bShouldCancel;
    bool    m_bInterruptable;
    bool    m_bParsing;
    bool    m_bParseComments;
    QStringList m_PendingComments;
    int     m_iLastNewlinePosition;
    int     m_iNewlineCount;
    int     m_iErrorChar;
    ParseError m_iErrorCode;
};

#endif // KEYVALUESPARSER_H
