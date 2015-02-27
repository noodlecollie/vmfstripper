#include "keyvaluesparser.h"
#include "keyvaluesnode.h"
#include <QRegularExpression>
#include <QtDebug>
#include <QApplication>
#include <QTime>

/**
 * A whitespace-agnostic keyvalues language (without catering for comments) is defined as follows:
 *
 * <string> terminal is an unquoted sequence of alphanumeric characters (including underscores).
 * <qstring> terminal is a quoted sequence of characters that does not include any unescaped quotes within
 *  its enclosing quotes.
 * <{> terminal corresponds to an opening brace.
 * <}> terminal corresponds to a closing brace.
 * No other terminals are allowed. Whitespace is not a terminal but can be used to delimit terminals.
 *
 * + is a "stack push". + -> <{>
 * - is a "stack pop". - -> <}>
 * T is a "token". T -> <string> | <qstring>
 * E is an "entry". E -> TT This is a standard "key" "value" pair.
 * C is a "container". C -> T+R- This is a key{ ..subkeys.. } group.
 * R is a "recursion". R -> <nothing> | ER | CR
 *
 * This gives us all the keyvalues structures that are allowed, excluding if newlines are to be used strictly.
 * For example:
 *
 * node
 * {
 *      key1    value1
 *      key2    value2
 *      subnode
 *      {
 *          key3    value3
 *      }
 * }
 *
 * Is given as:
 * T+TTTTT+TT--
 *
 * Through this grammar, we can parse things entirely in terms of string tokens.
 * We just need to keep a counter of how many pushes vs pops there have been: the
 * file we've parsed is only acceptable if this counter is 0.
 * If we also want to exclude empty files, this should be trivial.
 *
 * The following state diagram demonstrates the parsing.
 * The base state accepts if stack counter is zero.
 * No other states accept.
 *
 *           Read push, increment stack
 *             +---------||---------+
 *             |                    |
 *             |    Read single T   |          +----+
 *      +--v   V    v-----------+   |          |    v Read any symbol
 * Read =  [[ base ]]           [elevated]     [fail]
 * pop, +--+      | +-----------^        +-----^  ^
 * decrement      | Read single T        Read pop |
 * stack          |                               |
 *                |                               |
 *                +-------------------------------+
 *                            Read push
 */

KeyValuesParser::KeyValuesParser(QObject *parent) :
    QObject(parent)
{
    m_iState = StateBase;
    m_bSendUpdates = false;
    m_flProgress = 0.0f;
    m_bShouldCancel = false;
    m_bInterruptable = false;
    m_bParsing = false;
    m_iLastNewlinePosition = -1;
    m_iNewlineCount = 0;
    m_iErrorChar;
    m_iErrorCode = NoError;
}

void KeyValuesParser::characterPosition(int pos, int &line, int &character) const
{
    if ( m_iLastNewlinePosition < 0 )
    {
        line = 0;
        character = pos;
        return;
    }
    
    line = m_iNewlineCount+1;
    character = pos - m_iLastNewlinePosition - 1;
}

void KeyValuesParser::identifyNextToken(const QStringRef &content, int &begin, int &end)
{
    // If the string is empty, return nothing.
    if ( content.isEmpty() )
    {
        begin = -1;
        end = -1;
        return;
    }

    // Begin by looking for the index of the next valid character that could start a token.
    // Whitespace is skipped.
    QRegularExpression reBegin("[^\\s]");
    int bIndex = content.string()->indexOf(reBegin, content.position());

    // If there was no non-whitespace character, return nothing.
    if ( bIndex < 0 )
    {
        begin = -1;
        end = -1;
        return;
    }

    // If the character was at the end of the string, return it on its own.
    else if ( bIndex == content.string()->length() - 1 )
    {
        begin = bIndex;
        end = begin;
        return;
    }

    // Do different things depending on the character we found.
    // If the character is a quote, we need to search for a matching unescaped quote.
    if ( content.string()->at(bIndex) == '\"' )
    {
        // Find the next unescaped quote.
        // (?<!\\)"
        QRegularExpression findUnescapedQuote("(?<!\\\\)\"");
        int eIndex = content.string()->indexOf(findUnescapedQuote, bIndex + 1);

        // If we didn't find an unescaped quote, return the rest of the string.
        if ( eIndex < 0 )
        {
            begin = bIndex;
            end = content.string()->length()- 1;
            return;
        }

        // Return the entire token, including quotes.
        begin = bIndex;
        end = eIndex;
        return;
    }

    // If the character is a letter, number or underscore, we need to search for the next character that isn't.
    else if ( content.string()->at(bIndex).isLetterOrNumber() || content.string()->at(bIndex) == '_' )
    {
        // Find the next character that's not a letter, number or an underscore.
        // [^(\w|_)]
        QRegularExpression invalidChar("[^(\\w|_)]");
        int eIndex = content.string()->indexOf(invalidChar, bIndex + 1);

        // If we didn't find one, return the rest of the string.
        if ( eIndex < 1 )
        {
            begin = bIndex;
            end = content.string()->length() - 1;
            return;
        }

        // Return the token.
        begin = bIndex;
        end = eIndex - 1;
        return;
    }
    
    // If the character and the one after are both forward slashes, this is a line comment.
    // Find the next newline after them.
    else if ( content.string()->at(bIndex) == '/' && content.string()->at(bIndex+1) == '/' )
    {
        // If the second slash is at the end of the string, just return them both.
        if ( bIndex + 1 == content.string()->length() - 1 )
        {
            begin = bIndex;
            end = content.string()->length() - 1;
            return;
        }
        
        // Find the next newline.
        int eIndex = content.string()->indexOf('\n', bIndex + 2);
        
        // If we didn't find one, return the rest of the string.
        if ( eIndex < 1 )
        {
            begin = bIndex;
            end = content.string()->length() - 1;
            return;
        }
        
        // If we did, return the entire section.
        // The newline can be stripped later.
        begin = bIndex;
        end = eIndex;
        return;
    }

    // If the character is anything else, just return it.
    else
    {
        begin = bIndex;
        end = begin;
        return;
    }
}

QString KeyValuesParser::unquoteToken(const QStringRef &token)
{
    QString str = token.toString().trimmed();
    if ( str.at(0) == '"' ) str = str.right(str.length() - 1);
    if ( str.at(str.length() -1 ) == '"' ) str = str.left(str.length() - 1);

    return str;
}

QString KeyValuesParser::unescapeToken(const QStringRef &token)
{
    QString str = token.toString();

    // Convert "\"" to '"'
    str.replace(QRegularExpression("\\\\\""), "\"");

    // Convert "\n" to '\n'.
    str.replace(QRegularExpression("\\\\n"), "\n");

    // Convert "\t" to '\t'.
    str.replace(QRegularExpression("\\\\t"), "\t");

    return str;
}

QString KeyValuesParser::extractComment(const QStringRef &token)
{
    QString comment = token.toString();
    
    int ss = comment.indexOf("//");
    if ( ss < 0 ) return comment;
    
    int nl = comment.indexOf('\n');
    if ( nl < 0 )
    {
        return comment.mid(ss+2).trimmed();
    }
    
    return comment.mid(ss+2, nl-ss-2).trimmed();
}

QString KeyValuesParser::stringListToMultilineString(const QStringList &list)
{
    QString output;
    bool first = true;
    foreach ( QString s, list )
    {
        if ( !first ) output += '\n';
        output += s;
    }
    
    return output;
}

KeyValuesParser::TokenType KeyValuesParser::classifyToken(const QStringRef &token)
{
    if ( token == "{" ) return TokenPush;
    if ( token == "}" ) return TokenPop;
    if ( token.at(0) == '"' && token.at(token.length() - 1) == '"' ) return TokenQuoted;
    if ( token.at(0) == '/' && token.at(1) == '/' ) return TokenComment;

    // [(\w|_]+
    QRegularExpression plainToken("[(\\w|_]+");
    if ( plainToken.match(token.toString(), 0, QRegularExpression::NormalMatch,
                          QRegularExpression::AnchoredMatchOption).hasMatch() ) return TokenPlain;

    m_iErrorChar = token.position();
    m_iErrorCode = InvalidTokenError;
    
    return TokenInvalid;
}

KeyValuesParser::ParseError KeyValuesParser::parse(const QString &content, KeyValuesNode &container)
{
    setState(StateBase);
    m_szLastToken = QString();
    m_PendingComments.clear();
    m_iLastNewlinePosition = -1;
    m_iNewlineCount = 0;
    updateProgress(0);

    if ( content.isEmpty() ) return NoContentError;

    // Keep a stack of our node hierarchy.
    QStack<KeyValuesNode*> nodeStack;

    // Create a new node to wrap the file, since the file
    // may have multiple root-level nodes (in the case of a VMF).
    KeyValuesNode* first = new KeyValuesNode("wrapper");
    nodeStack.push(first);

    // Check to see if we can start parsing.
    int begin = -1, end = -1;
    identifyNextToken(content.midRef(0), begin, end);
    if ( begin < 0)
    {
        delete first;
        m_szLastToken = QString();
        updateProgress(1);
        
        m_iErrorCode = NoContentError;
        m_iErrorChar = 0;
        
        return NoContentError;
    }
    
    QTime timer;
    timer.start();
    
    m_bShouldCancel = false;
    m_bParsing = true;
    bool haveNonComment = false;

    // Continually parse the content.
    do
    {
        updateNewlineBookkeeping(content, end);
        
        if ( m_bInterruptable || m_bSendUpdates )
        {
            QApplication::processEvents(QEventLoop::AllEvents, 10);
            
            if ( m_bShouldCancel )
            {
                delete first;
                m_szLastToken = QString();
                updateProgress(1);
                m_bParsing = false;
                
                m_iErrorCode = OperationCancelledError;
                m_iErrorChar = begin < 0 ? 0 : begin;
                
                return OperationCancelledError;
            }
            
            if ( timer.elapsed() >= 500 )
            {
                // Send an update on how far we are through the parsing.
                if ( m_bSendUpdates ) emit byteProgress(end, content.length());
                updateProgress((float)end/(float)content.length());
                
                timer.restart();
            }
        }
        
        if ( timer.elapsed() >= 500 )
        {
            // Send an update on how far we are through the parsing.
            if ( m_bSendUpdates ) emit byteProgress(end, content.length());
            updateProgress((float)end/(float)content.length());
            
            timer.restart();
        }

        // Get the next token we've found.
        QStringRef token = segmentRef(content.midRef(0), begin, end);

        // Classify the token.
        TokenType type = classifyToken(token);

        // Act on its type.
        ParseError error = NoError;
        switch (type)
        {
            case TokenPush:
            {
                haveNonComment = true;
                error = handleTokenPush(token, nodeStack);
                break;
            }

            case TokenPop:
            {
                haveNonComment = true;
                error = handleTokenPop(token, nodeStack);
                break;
            }

            case TokenPlain:
            {
                haveNonComment = true;
                error = handleTokenPlain(token, nodeStack);
                break;
            }

            case TokenQuoted:
            {
                haveNonComment = true;
                error = handleTokenQuoted(token, nodeStack);
                break;
            }
            
            case TokenComment:
            {
                error = NoError;
                break;
            }

            default:
            {
                haveNonComment = true;
                error = InvalidTokenError;
                break;
            }
        }

        // If we got an error, return it.
        if ( error != NoError )
        {
            delete first;
            m_szLastToken = QString();
            updateProgress(1);
            m_bParsing = false;
            return error;
        }

        // Get the string ref for the next token.
        QStringRef nextToken = content.midRef(end+1);
        
        // Identify the token.
        identifyNextToken(nextToken, begin, end);
    }
    while ( begin >= 0 );

    // Check that the stack has only one item,
    // otherwise our pushes and pops were mismatched.
    if ( nodeStack.size() > 1 )
    {
        delete first;
        m_szLastToken = QString();
        m_bParsing = false;
        
        m_iErrorChar = content.size() - 1;
        m_iErrorCode = UnmatchedBraceError;
        
        return UnmatchedBraceError;
    }
    
    // Check whether we got any tokens other than comments.
    if ( haveNonComment == false )
    {
        delete first;
        m_szLastToken = QString();
        m_bParsing = false;
        
        m_iErrorChar = content.size() - 1;
        m_iErrorCode = NoContentError;
        
        return NoContentError;
    }

    // Change the parent of the wrapper's children.
    QList<KeyValuesNode*> children = first->childNodes();
    foreach ( KeyValuesNode* n, children )
    {
        n->setParent(&container);
    }

    // Delete the wrapper.
    delete first;
    m_szLastToken = QString();
    m_bParsing = false;

    m_iErrorChar = 0;
    return NoError;
}

KeyValuesParser::ParseError KeyValuesParser::handleTokenPush(const QStringRef &ref, QStack<KeyValuesNode *> &stack)
{
    // Do things depending on our state.
    switch ( m_iState )
    {
        // We received a push without having a key beforehand.
        // This is invalid.
        case StateBase:
        {
            setState(StateFail);
            m_szLastToken = QString();
            
            m_iErrorChar = ref.position();
            m_iErrorCode = UnnamedNodeError;
            
            return UnnamedNodeError;
        }

        // We received a push after having received a key.
        // This means we're creating a container node.
        case StateElevated:
        {
            KeyValuesNode* node = new KeyValuesNode(m_szLastToken, stack.top());
            node->setComment(stringListToMultilineString(m_PendingComments));
            m_PendingComments.clear();
            stack.push(node);
            setState(StateBase);
            m_szLastToken = QString();
            return NoError;
        }

        // We shouldn't be here!
        default:
        {
            Q_ASSERT(false);
            setState(StateFail);
            m_szLastToken = QString();
            
            m_iErrorChar = ref.position();
            m_iErrorCode = UnspecifiedError;
            
            return UnspecifiedError;
        }
    }
}

KeyValuesParser::ParseError KeyValuesParser::handleTokenPop(const QStringRef &ref, QStack<KeyValuesNode *> &stack)
{
    // Do things depending on our state.
    switch ( m_iState )
    {
        // We received a pop. This means we're returning to the parent node.
        case StateBase:
        {
            // Check if the stack will end up with no items.
            // This means our original wrapper node will be popped,
            // which implies more pops than pushes were encountered.
            if ( stack.size() < 2 )
            {
                setState(StateFail);
                m_szLastToken = QString();
                
                m_iErrorChar = ref.position();
                m_iErrorCode = StackUnderflowError;
                
                return StackUnderflowError;
            }

            stack.pop();
            setState(StateBase);
            m_szLastToken = QString();
            return NoError;
        }

        // We received a pop after only having received one token.
        // This is always invalid.
        case StateElevated:
        {
            setState(StateFail);
            m_szLastToken = QString();
            
            m_iErrorChar = ref.position();
            m_iErrorCode = IncompleteNodeError;
            
            return IncompleteNodeError;
        }

        // We shouldn't be here!
        default:
        {
            Q_ASSERT(false);
            setState(StateFail);
            m_szLastToken = QString();
            
            m_iErrorChar = ref.position();
            m_iErrorCode = UnspecifiedError;
            
            return UnspecifiedError;
        }
    }
}

KeyValuesParser::ParseError KeyValuesParser::handleTokenPlain(const QStringRef &str, QStack<KeyValuesNode *> &stack)
{
    // Just forward it straight on.
    return handleTokenGeneric(str, stack);
}

KeyValuesParser::ParseError KeyValuesParser::handleTokenQuoted(const QStringRef &str, QStack<KeyValuesNode *> &stack)
{
    // Unquote the token.
    QString token = unquoteToken(str);

    // Convert escape sequences.
    token = unescapeToken(token.midRef(0));

    // Forward it on.
    return handleTokenGeneric(token.midRef(0), stack);
}

KeyValuesParser::ParseError KeyValuesParser::handleTokenGeneric(const QStringRef &str, QStack<KeyValuesNode *> &stack)
{
    // Do things depending on our state.
    switch ( m_iState )
    {
        // Store the token in preparation for future operations.
        case StateBase:
        {
            setState(StateElevated);
            m_szLastToken = str.toString();
            return NoError;
        }

        // Use our stored token to add a key-value child to the current node on the stack.
        case StateElevated:
        {
            KeyValuesNode* node = new KeyValuesNode(stack.top());
            node->setComment(stringListToMultilineString(m_PendingComments));
            m_PendingComments.clear();
            node->setKey(m_szLastToken);
            node->setValue(str.toString());
            setState(StateBase);
            m_szLastToken = QString();
            return NoError;
        }

        // We shouldn't be here!
        default:
        {
            Q_ASSERT(false);
            setState(StateFail);
            m_szLastToken = QString();
            
            m_iErrorChar = str.position();
            m_iErrorCode = UnspecifiedError;
            
            return UnspecifiedError;
        }
    }
}

KeyValuesParser::ParseError KeyValuesParser::handleTokenComment(const QStringRef &str, QStack<KeyValuesNode *> &)
{
    if ( !parseComments() ) return NoError; // How do I feel about this? I cannot pass comment.
    
    // Extract the comment.
    QString comment = extractComment(str);
    
    // Add the comment to our pending list.
    m_PendingComments.append(comment);
    
    return NoError;
}

void KeyValuesParser::setState(State s)
{
    m_iState = s;
}

bool KeyValuesParser::sendUpdates() const
{
    return m_bSendUpdates;
}

void KeyValuesParser::setSendUpdates(bool enabled)
{
    if ( m_bSendUpdates == enabled ) return;

    m_bSendUpdates = enabled;
    emit sendUpdatesStatusChanged(m_bSendUpdates);
}

void KeyValuesParser::updateProgress(float progress)
{
    if ( m_flProgress == progress) return;

    m_flProgress = progress;
    if ( m_bSendUpdates ) emit parseUpdate(m_flProgress);
}

void KeyValuesParser::cancelParsing()
{
    m_bShouldCancel = true;
}

bool KeyValuesParser::interruptable() const
{
    return m_bInterruptable;
}

void KeyValuesParser::setInterruptable(bool enabled)
{
    if ( m_bParsing || m_bInterruptable == enabled ) return;
    
    m_bInterruptable = enabled;
    emit interruptableStatusChanged(m_bInterruptable);
}

bool KeyValuesParser::parseComments() const
{
    return m_bParseComments;
}

void KeyValuesParser::setParseComments(bool enabled)
{
    if ( m_bParseComments == enabled ) return;
    
    m_bParseComments = enabled;
    emit parseCommentsStatusChanged(m_bParseComments);
}

void KeyValuesParser::stringForParseError(ParseError error, QString &title, QString &description)
{
    switch (error)
    {
        case NoError:
        {
            title = "No error";
            description = "No error occurred.";
            break;
        }
        
        case NoContentError:
        {
            title = "No content";
            description = "No keyvalues content was present in the file provided.";
            break;
        }
        
        case InvalidTokenError:
        {
            title = "Invalid token";
            description = "Invalid syntax was encountered in the file.";
            break;
        }
        
        case StackUnderflowError:
        {
            title = "Stack underflow";
            description = "More ending braces than beginning braces were present in the file.";
            break;
        }
        
        case UnnamedNodeError:
        {
            title = "Unnamed node";
            description = "A new subkey was started within the file before its parent's key had been specified.";
            break;
        }
        
        case IncompleteNodeError:
        {
            title = "Incomplete node";
            description = "The key of a key-value pair was specified without a corresponding value.";
            break;
        }
        
        case UnmatchedBraceError:
        {
            title = "Unmatched brace";
            description = "The end of the file was reached before all brace pairs had been closed.";
            break;
        }
        
        case OperationCancelledError:
        {
            title = "Operation cancelled";
            description = "The parsing operation was cancelled before it was able to finish.";
            break;
        }
        
        default:
        {
            title = "Unspecified error";
            description = "An unknown error was encountered. Congratulations! You must have been doing something pretty special.";
            break;
        }
    }
}

void KeyValuesParser::updateNewlineBookkeeping(const QString &content, int furthestChar)
{
    // Find out how many newlines there are between our last checked position and this one.
    for ( int i = m_iLastNewlinePosition < 0 ? 0 : m_iLastNewlinePosition + 1; i <= furthestChar; i++ )
    {
        if ( content.at(i) == '\n' )
        {
            m_iLastNewlinePosition = i;
            m_iNewlineCount++;
        }
    }
}

void KeyValuesParser::parseError(int &error, QString &title, QString &description, int &lineNo, int &charNo) const
{
    error = m_iErrorCode;
    stringForParseError(m_iErrorCode, title, description);
    characterPosition(m_iErrorChar, lineNo, charNo);
}
