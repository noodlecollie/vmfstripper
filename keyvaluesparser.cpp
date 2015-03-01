#include "keyvaluesparser.h"
#include <QtDebug>
#include <QStack>
#include <QJsonDocument>
#include <QJsonObject>

KeyValuesParser::KeyValuesParser(QObject *parent) :
    QObject(parent)
{
}

QJsonParseError KeyValuesParser::jsonFromKeyValues(const QByteArray &keyValues, QJsonDocument &document, QString *errorSnapshot)
{
    QByteArray json;
    simpleKeyValuesToJson(keyValues, json);
    
    QJsonParseError error;
    document = QJsonDocument::fromJson(json, &error);
    
    if ( error.error != QJsonParseError::NoError )
    {
        if ( errorSnapshot )
        {
            int begin = error.offset - 10;
            int end = error.offset + 10;
            if ( begin < 0 ) begin = 0;
            if ( end >= json.length() ) end = json.length();
            *errorSnapshot = QString(json.mid(begin, end-begin+1));
        }
    }
    
    return error;
}

bool KeyValuesParser::getNextToken(const QByteArray &array, int from, KeyValuesToken &token)
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
        token.setNextReadPosition(length);
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

bool KeyValuesParser::handleInvalidToken(const QByteArray &array, int pos, KeyValuesToken &token)
{
    token.invalidate();
    token.setNextReadPosition(pos+1);
    
    return pos < array.length() - 1;
}

bool KeyValuesParser::handleCommentToken(const QByteArray &array, int pos, KeyValuesToken &token)
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

bool KeyValuesParser::handlePopToken(const QByteArray &array, int pos, KeyValuesToken &token)
{
    token.setType(KeyValuesToken::TokenPop);
    token.setBegin(pos);
    token.setLength(1);
    token.setNextReadPosition(pos+1);
    
    return pos < array.length() - 1;
}

bool KeyValuesParser::handlePushToken(const QByteArray &array, int pos, KeyValuesToken &token)
{
    token.setType(KeyValuesToken::TokenPush);
    token.setBegin(pos);
    token.setLength(1);
    token.setNextReadPosition(pos+1);
    
    return pos < array.length() - 1;
}

bool KeyValuesParser::handleQuotedStringToken(const QByteArray &array, int pos, KeyValuesToken &token)
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


bool KeyValuesParser::handleUnquotedStringToken(const QByteArray &array, int pos, KeyValuesToken &token)
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

void KeyValuesParser::writeTokenToArray(QByteArray &array, const KeyValuesToken &token, int stackValue)
{
    if ( token.type() == KeyValuesToken::TokenInvalid )
    {
        return;
    }
    
    switch ( token.type() )
    {
        case KeyValuesToken::TokenStringQuoted:
        case KeyValuesToken::TokenStringUnquoted:
        {
            // If the stack value is odd, write it as an identifier.
            if ( stackValue % 2 == 1 )
            {
                array.append(QString("\"%0_%1\"").arg(stackValue/2).arg(QString(token.arraySection())));
            }
            else
            {
                array.append(QString("\"%0\"").arg(QString(token.arraySection())));
            }
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

void KeyValuesParser::simpleKeyValuesToJson(const QByteArray &keyValues, QByteArray &output)
{
    output.clear();
    
    // This stack holds how many strings have been written to the current section.
    // Each time we receive a push, push a 0 onto the stack.
    // Each time we receive a string, increment the top element.
    // Every time we receive a pop, pop the top element and increment the new top.
    // We use this to work out when to prepend a ';' or ',' as well as writing identifier
    // prefixes to keys (because the QJsonObject doesn't allow duplicate keys).
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
        
        writeTokenToArray(output, token, braceStack.top());
        from = token.nextReadPosition();
        Q_ASSERT(from >= 0);
    }
    
    // Add an ending brace.
    output.append('}');
    braceStack.pop();
}

void KeyValuesParser::simpleJsonToKeyValues(const QByteArray &json, QByteArray &output)
{
    output.clear();
    int numBraces = 0;
    int spacesSinceNewline = 0;
    bool inQuote = false;
    QStack<int> braceStack;
    for ( int i = 0; i < json.length(); i++ )
    {
        const char* ch = static_cast<const char*>(json.constData() + i);
        
        if ( *ch == '{' )
        {
            numBraces++;
            braceStack.push(0);
            
            // Only write if this was not a root brace.
            if ( numBraces > 1 )
            {
                output.append('\n');
                
                for ( int j = 0; j < spacesSinceNewline-5; j++ )
                {
                    output.append(' ');
                }
                
                output.append(*ch);
            }
            
            continue;
        }
        
        if ( *ch == '}' )
        {
            numBraces--;
            braceStack.pop();
            if ( braceStack.size() > 0 ) braceStack.top()++;
            
            // Only write if this was not a root brace.
            if ( numBraces > 0 )
            {
                output.append(*ch);
            }

            continue;
        }
        
        // Don't write anything if we're not inside the root braces.
        // We don't want to include these in the KV file.
        if ( numBraces < 1 ) continue;
        
        // If we found an unescaped quote, note this and write it out.
        if ( *ch == '"' && (i == 0 || *(ch-1) != '\\') )
        {
            inQuote = !inQuote;
            
            // If we're now in a quote and the stack count is even (ie. this is a key)
            // find the next underscore.
            if ( inQuote && braceStack.top() % 2 == 0 )
            {
                int j = 1;
                int length = json.length();
                while ( i + j < length &&  *(static_cast<const char*>(json.constData()+i+j)) != '_' ) j++;
                
                // i will be incremented by the loop, so set to the position of the underscore.
                i += j;
            }
            
            // If we're no longer in a quote, increment the number of strings we've written.
            else if ( !inQuote )
            {
                braceStack.top()++;
            }
            
            output.append(*ch);
            continue;
        }
        
        // If we found a newline, reset the number of spaces.
        if ( *ch == '\n' )
        {
            spacesSinceNewline = 0;
            
            // Don't output if we're just before a root brace.
            if ( i > 0 && *(ch-1) == '{' && numBraces == 1 ) continue;
            
            output.append(*ch);
            continue;
        }
        
        // If we found a space, only output it if the number of spaces since the last newline is at least 4.
        // This is purely for visual purposes: because the root braces are removed, the indentation looks off.
        if ( *ch == ' ' )
        {
            if ( spacesSinceNewline >= 4 ) output.append(*ch);
            spacesSinceNewline++;
            continue;
        }
        
        // If we're not in a quote, replace any colons or commas by spaces.
        if ( !inQuote )
        {
            if ( *ch == ':' || *ch == ',' )
            {
                output.append(' ');
                continue;
            }
        }
        
        output.append(*ch);
        continue;
    }
}

void KeyValuesParser::keyvaluesFromJson(const QJsonDocument &document, QByteArray &keyValues)
{
    keyValues.clear();
    if ( document.isNull() || document.isEmpty() ) return;
    
    QByteArray arr = document.toJson();
    simpleJsonToKeyValues(arr, keyValues);
}

QString KeyValuesParser::stripIdentifier(const QString &key)
{
    // If the string doesn't begin with a number, just return it.
    QChar c = key.at(0);
    if ( c < '0' || c > '9' ) return key;
    
    // Find the first underscore.
    for ( int i = 1; i < key.length(); i++ )
    {
        if ( key.at(i) == '_' )
        {
            if ( i == key.length()-1 ) return QString("");
            return key.mid(i+1);
        }
    }
    
    return key;
}
