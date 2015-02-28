#include "keyvaluesparsernew.h"
#include <QtDebug>
#include <QStack>

KeyValuesParserNew::KeyValuesParserNew(QObject *parent) :
    QObject(parent)
{
}

bool KeyValuesParserNew::getNextToken(const QByteArray &array, int from, KeyValuesToken &token)
{
    Q_ASSERT(token.array() == &array);
    
    int index = from;
    int length = array.length();
    while ( index < length )
    {
        if ( isWhitespace(array.at(index)) ) index++;
        else break;
    }
    
    if ( index >= length )
    {
        token.invalidate();
        return false;
    }
    
    // We found some non-whitespace - handle it appropriately.
    const char* ch = static_cast<const char*>(array.constData() + index);
    if ( isAlphaNumeric(*ch) )
    {
        return handleUnquotedStringToken(array, index, token);
    }
    else if ( *ch == '"' )
    {
        return handleQuotedStringToken(array, index, token);
    }
    else if ( *ch == '{' )
    {
        return handlePushToken(array, index, token);
    }
    else if ( *ch == '}' )
    {
        return handlePopToken(array, index, token);
    }
    else if ( index < length-1 && isCommentMarker(ch) )
    {
        return handleCommentToken(array, index, token);
    }
    else
    {
        return handleInvalidToken(array, index, token);
    }
}

bool KeyValuesParserNew::handleInvalidToken(const QByteArray &array, int pos, KeyValuesToken &token)
{
    token.invalidate();
    token.setNextReadPosition(pos+1);
    
    return pos < array.length() - 1;
}

bool KeyValuesParserNew::handleCommentToken(const QByteArray &array, int pos, KeyValuesToken &token)
{
    // The comment begins at pos+2. Look for the next newline at this position or after.
    int begin = pos+2;
    int length = array.length();
    for ( int i = begin; i < length; i++ )
    {
        if ( array.at(i) == '\n' )
        {
            // Set the token appropriately.
            token.setType(KeyValuesToken::TokenComment);
            token.setBegin(begin);
            token.setLength(i-begin);
            token.setNextReadPosition(i+1);
            return true;
        }
    }
    
    // We reached the end of the array.
    token.setType(KeyValuesToken::TokenComment);
    token.setBegin(begin);
    token.setLength(length - begin);
    token.setNextReadPosition(length);
    return false;
}

bool KeyValuesParserNew::handlePopToken(const QByteArray &array, int pos, KeyValuesToken &token)
{
    token.setType(KeyValuesToken::TokenPop);
    token.setBegin(pos);
    token.setLength(1);
    token.setNextReadPosition(pos+1);
    
    return pos < array.length() - 1;
}

bool KeyValuesParserNew::handlePushToken(const QByteArray &array, int pos, KeyValuesToken &token)
{
    token.setType(KeyValuesToken::TokenPush);
    token.setBegin(pos);
    token.setLength(1);
    token.setNextReadPosition(pos+1);
    
    return pos < array.length() - 1;
}

bool KeyValuesParserNew::handleQuotedStringToken(const QByteArray &array, int pos, KeyValuesToken &token)
{
    // The string itself begins at pos+1.
    int begin = pos+1;
    int length = array.length();
    for ( int i = begin; i < length; i++ )
    {
        if ( array.at(i) == '"' && array.at(i-1) != '\\' )
        {
            token.setType(KeyValuesToken::TokenStringQuoted);
            token.setBegin(begin);
            token.setLength(i-begin);
            token.setNextReadPosition(i+1);
            return true;
        }
    }
    
    // We reached the end of the array.
    token.setType(KeyValuesToken::TokenStringQuoted);
    token.setBegin(begin);
    token.setLength(length - begin);
    token.setNextReadPosition(length);
    return false;
}


bool KeyValuesParserNew::handleUnquotedStringToken(const QByteArray &array, int pos, KeyValuesToken &token)
{
    int length = array.length();
    for ( int i = pos+1; i < length; i++ )
    {
        // The first non-alphanumeric character is our terminator.
        if ( !isAlphaNumeric(array.at(i)) )
        {
            token.setType(KeyValuesToken::TokenStringUnquoted);
            token.setBegin(pos);
            token.setLength(i-pos);
            token.setNextReadPosition(i);
            return true;
        }
    }
    
    // We reached the end of the array.
    token.setType(KeyValuesToken::TokenStringUnquoted);
    token.setBegin(pos);
    token.setLength(length - pos);
    token.setNextReadPosition(length);
    return false;
}

void KeyValuesParserNew::writeTokenToArray(QByteArray &array, const KeyValuesToken &token)
{
    if ( !token.isValid() ) return;
    
    switch ( token.type() )
    {
        case KeyValuesToken::TokenStringQuoted:
        {
            array.append(QString("\"%0\"").arg(QString(token.arraySection())));
            break;
        }
        
        case KeyValuesToken::TokenStringUnquoted:
        {
            array.append(QString("\"%0\"").arg(QString(token.arraySection())));
            break;
        }
        
        case KeyValuesToken::TokenPush:
        {
            array.append(token.arraySection());
            break;
        }
        
        case KeyValuesToken::TokenPop:
        {
            array.append(token.arraySection());
            break;
        }
        
        default: break;
    }
}

void KeyValuesParserNew::simpleKeyValuesToJson(const QByteArray &keyValues, QByteArray &output)
{
    output.clear();
    
    // Each time we receive a push, push a 0 onto the stack.
    // Each time we receive a string, increment the top element.
    // Every time we receive a pop, pop the top element and increment the new top.
    QStack<int> braceStack;
    
    // Add a beginning brace, as required by JSON.
    output.append('{');
    braceStack.push(0);
    
    int from = 0;
    int inputLength = keyValues.length();
    while ( from < inputLength )
    {
        // Get the next token.
        KeyValuesToken token(&keyValues);
        getNextToken(keyValues, from, token);
        
        if ( token.isPush() )
        {
            braceStack.push(0);
        }
        else if ( token.isPop() )
        {
            braceStack.pop();
            if ( braceStack.size() > 0 ) braceStack.top()++;
        }

        
        // Handle prepending.
        if ( token.isString() && braceStack.size() > 0 )
        {
            if ( braceStack.top() == 0 )
            {
                braceStack.top()++;
            }
            else if ( braceStack.top() % 2 == 0 )
            {
                output.append(',');
                braceStack.top()++;
            }
            else
            {
                output.append(':');
                braceStack.top()++;
            }
        }
        else if ( token.isPush() )
        {
            output.append(':');
        }
        
        writeTokenToArray(output, token);
        from = token.nextReadPosition();
    }
    
    // Add an ending brace.
    output.append('}');
    braceStack.pop();
}
