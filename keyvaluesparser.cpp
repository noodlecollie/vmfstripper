#include "keyvaluesparser.h"
#include "keyvaluesnode.h"
#include <QRegularExpression>
#include <QtDebug>

/**
 * A whitespace-agnostic keyvalues language is defined as follows:
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
}

void KeyValuesParser::identifyNextToken(const QString &contentRef, int &begin, int &end)
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
    int bIndex = content.indexOf(reBegin);

    // If there was no non-whitespace character, return nothing.
    if ( bIndex < 0 )
    {
        begin = -1;
        end = -1;
        return;
    }

    // If the character was at the end of the string, return it on its own.
    else if ( bIndex == content.length() - 1 )
    {
        begin = bIndex;
        end = bIndex;
        return;
    }

    // Do different things depending on the character we found.
    // If the character is a quote, we need to search for a matching unescaped quote.
    if ( content.at(bIndex) == '\"' )
    {
        // Find the next unescaped quote.
        // (?<!\\)"
        QRegularExpression findUnescapedQuote("(?<!\\\\)\"");
        int eIndex = content.indexOf(findUnescapedQuote, bIndex+1);

        // If we didn't find an unescaped quote, return the rest of the string.
        if ( eIndex < 0 )
        {
            begin = bIndex;
            end = content.length() - 1;
            return;
        }

        // Return the entire token, including quotes.
        begin = bIndex;
        end = eIndex;
        return;
    }

    // If the character is a letter, number or underscore, we need to search for the next character that isn't.
    else if ( content.at(bIndex).isLetterOrNumber() || content.at(bIndex) == '_' )
    {
        // Find the next character that's not a letter, number or an underscore.
        // [^(\w|_)]
        QRegularExpression invalidChar("[^(\\w|_)]");
        int eIndex = content.indexOf(invalidChar, bIndex+1);

        // If we didn't find one, return the rest of the string.
        if ( eIndex < 1 )
        {
            begin = bIndex;
            end = content.length() - 1;
            return;
        }

        // Return the token.
        begin = bIndex;
        end = eIndex - 1;
        return;
    }

    // If the character is anything else, just return it.
    else
    {
        begin = bIndex;
        end = bIndex;
        return;
    }
}

QString KeyValuesParser::unquoteToken(const QStringRef &token)
{
    QString str = token.trimmed();
    if ( str.at(0) == '"' ) str = str.right(str.length() - 1);
    if ( str.at(str.length() -1 ) == '"' ) str = str.left(str.length() - 1);

    return str;
}

QString KeyValuesParser::unescapeToken(const QStringRef &token)
{
    QString str = token;

    // Convert "\"" to '"'
    str.replace(QRegularExpression("\\\\\""), '"');

    // Convert "\n" to '\n'.
    str.replace(QRegularExpression("\\\\n"), '\n');

    // Convert "\t" to '\t'.
    str.replace(QRegularExpression("\\\\t"), '\t');

    return str;
}

KeyValuesParser::TokenType KeyValuesParser::classifyToken(const QStringRef &token)
{
    if ( token == "{" ) return TokenPush;
    if ( token == "}" ) return TokenPop;
    if ( token.at(0) == '"' && token.at(token.length() - 1) == '"' ) return TokenQuoted;

    // [(\w|_]+
    QRegularExpression plainToken("[(\\w|_]+");
    if ( plainToken.match(token, 0, QRegularExpression::AnchoredMatchOption).hasMatch() ) return TokenPlain;

    return TokenInvalid;
}

KeyValuesParser::ParseError KeyValuesParser::parse(const QString &content, KeyValuesNode &container)
{
    // Keep a stack of our node hierarchy.
    QStack<KeyValuesNode*> nodeStack;

    // Create a new node to wrap the file, since the file
    // may have multiple root-level nodes (in the case of a VMF).
    KeyValuesNode* first = new KeyValuesNode("wrapper");
    nodeStack.push(first);

    // Check to see if we can start parsing.
    int begin = -1, end = -1;
    identifyNextToken(content, begin, end);
    if ( begin < 0)
    {
        delete first;
        m_szLastToken = QString();
        return NoContentError;
    }

    // Continually parse the content.
    do
    {
        // Get the next token we've found.
        QStringRef token = segmentRef(content, begin, end);

        // Classify the token.
        TokenType type = classifyToken(token);

        // Act on its type.
        ParseError error = NoError;
        switch (type)
        {
            case TokenPush:
            {
                error = handleTokenPush(token, nodeStack);
                break;
            }

            case TokenPop:
            {
                error = handleTokenPop(token, nodeStack);
                break;
            }

            case TokenPlain:
            {
                error = handleTokenPlain(token, nodeStack);
                break;
            }

            case TokenQuoted:
            {
                error = handleTokenQuoted(token, nodeStack);
                break;
            }

            default:
            {
                error = InvalidTokenError;
                break;
            }
        }

        // If we got an error, return it.
        if ( error != NoError )
        {
            delete first;
            m_szLastToken = QString();
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
        return UnmatchedBraceError;
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

    return NoError;
}

KeyValuesParser::ParseError KeyValuesParser::handleTokenPush(const QStringRef &, QStack<KeyValuesNode *> &stack)
{
    // Do things depending on our state.
    switch ( m_iState )
    {
        // We received a push without having a key beforehand.
        // This is invalid.
        case StateBase:
        {
            m_iState = StateFail;
            m_szLastToken = QString();
            return UnnamedNodeError;
        }

        // We received a push after having received a key.
        // This means we're creating a container node.
        case StateElevated:
        {
            KeyValuesNode* node = new KeyValuesNode(m_szLastToken, stack.top());
            stack.push(node);
            m_iState = StateBase;
            m_szLastToken = QString();
            return NoError;
        }

        // We shouldn't be here!
        default:
        {
            Q_ASSERT(false);
            m_iState = StateFail;
            m_szLastToken = QString();
            return UnspecifiedError;
        }
    }
}

KeyValuesParser::ParseError KeyValuesParser::handleTokenPop(const QStringRef &, QStack<KeyValuesNode *> &stack)
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
                m_iState = StateFail;
                m_szLastToken = QString();
                return StackUnderflowError;
            }

            stack.pop();
            m_iState = StateBase;
            m_szLastToken = QString();
            return NoError;
        }

        // We received a pop after only having received one token.
        // This is always invalid.
        case StateElevated:
        {
            m_iState = StateFail;
            m_szLastToken = QString();
            return IncompleteNodeError;
        }

        // We shouldn't be here!
        default:
        {
            Q_ASSERT(false);
            m_iState = StateFail;
            m_szLastToken = QString();
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
    token = unescapeToken(token);

    // Forward it on.
    return handleTokenGeneric(token, stack);
}

KeyValuesParser::ParseError KeyValuesParser::handleTokenGeneric(const QStringRef &str, QStack<KeyValuesNode *> &stack)
{
    // Do things depending on our state.
    switch ( m_iState )
    {
        // Store the token in preparation for future operations.
        case StateBase:
        {
            m_iState = StateElevated;
            m_szLastToken = str.toString();
            return NoError;
        }

        // Use our stored token to add a key-value child to the current node on the stack.
        case StateElevated:
        {
            KeyValuesNode* node = new KeyValuesNode(stack.top());
            node->setKey(m_szLastToken);
            node->setValue(str.toString());
            m_iState = StateElevated;
            m_szLastToken = QString();
            return NoError;
        }

        // We shouldn't be here!
        default:
        {
            Q_ASSERT(false);
            m_iState = StateFail;
            m_szLastToken = QString();
            return UnspecifiedError;
        }
    }
}
