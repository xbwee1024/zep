#include "zep/syntax_orca.h"
#include "zep/editor.h"
#include "zep/theme.h"

#include "zep/mcommon/logger.h"
#include "zep/mcommon/string/stringutils.h"

#include <string>
#include <vector>

namespace Zep
{

ZepSyntax_Orca::ZepSyntax_Orca(ZepBuffer& buffer,
    const std::set<std::string>& keywords,
    const std::set<std::string>& identifiers,
    uint32_t flags)
    : ZepSyntax(buffer, keywords, identifiers, flags)
{
    // Don't need default
    m_adornments.clear();
}

void ZepSyntax_Orca::UpdateSyntax()
{
    auto& buffer = m_buffer.GetText();
    auto itrCurrent = buffer.begin();
    auto itrEnd = buffer.end();

    assert(std::distance(itrCurrent, itrEnd) <= int(m_syntax.size()));
    assert(m_syntax.size() == buffer.size());

    // Mark a region of the syntax buffer with the correct marker
    auto mark = [&](GapBuffer<utf8>::const_iterator itrA, GapBuffer<utf8>::const_iterator itrB, ThemeColor type, ThemeColor background) {
        std::fill(m_syntax.begin() + (itrA - buffer.begin()), m_syntax.begin() + (itrB - buffer.begin()), SyntaxData{ type, background });
    };

    auto markSingle = [&](GapBuffer<utf8>::const_iterator itrA, ThemeColor type, ThemeColor background) {
        (m_syntax.begin() + (itrA - buffer.begin()))->foreground = type;
        (m_syntax.begin() + (itrA - buffer.begin()))->background = background;
    };

    // Walk backwards to previous delimiter
    while (itrCurrent != itrEnd)
    {
        if (m_stop == true)
        {
            return;
        }

        // Update start location
        m_processedChar = long(itrCurrent - buffer.begin());

        if (*itrCurrent == '.' || *itrCurrent == ' ')
        {
            mark(itrCurrent, itrCurrent + 1, ThemeColor::Whitespace, ThemeColor::None);
        }

        auto token = std::string(itrCurrent, itrCurrent + 1);
        if (m_flags & ZepSyntaxFlags::CaseInsensitive)
        {
            token = string_tolower(token);
        }
        
        if (token[0] >= 'a' && token[0] <= 'z')
        {
            mark(itrCurrent, itrCurrent + 1, ThemeColor::Keyword, ThemeColor::None);
        }
        else if (token[0] == '#')
        {
            auto itrNext = itrCurrent + 1;
            while (itrNext != itrEnd &&
                *itrNext != '#' && *itrNext != '\n')
            {
                itrNext++;
            }
            mark(itrCurrent, itrNext + 1, ThemeColor::Comment, ThemeColor::None);
            itrCurrent = itrNext;
        }
        else if (token.find_first_not_of("0123456789") == std::string::npos)
        {
            mark(itrCurrent, itrCurrent+1, ThemeColor::Number, ThemeColor::None);
        }
        else
        {
            mark(itrCurrent, itrCurrent+1, ThemeColor::Normal, ThemeColor::None);
        }

        // Find String
        /*
        auto findString = [&](utf8 ch) {
            auto itrString = itrFirst;
            if (*itrString == ch)
            {
                itrString++;

                while (itrString < buffer.end())
                {
                    // handle end of string
                    if (*itrString == ch)
                    {
                        itrString++;
                        mark(itrFirst, itrString, ThemeColor::String, ThemeColor::None);
                        itrLast = itrString + 1;
                        break;
                    }

                    if (itrString < (buffer.end() - 1))
                    {
                        auto itrNext = itrString + 1;
                        // Ignore quoted
                        if (*itrString == '\\' && *itrNext == ch)
                        {
                            itrString++;
                        }
                    }

                    itrString++;
                }
            }
        };
        findString('\"');
        findString('\'');

        std::string commentStr = "/";
        auto itrComment = buffer.find_first_of(itrFirst, itrLast, commentStr.begin(), commentStr.end());
        if (itrComment != buffer.end())
        {
            auto itrCommentStart = itrComment++;
            if (itrComment < buffer.end())
            {
                if (*itrComment == '/')
                {
                    itrLast = buffer.find_first_of(itrCommentStart, buffer.end(), lineEnd.begin(), lineEnd.end());
                    mark(itrCommentStart, itrLast, ThemeColor::Comment, ThemeColor::None);
                }
            }
        }
        */

        itrCurrent++;
    }

    // If we got here, we sucessfully completed
    // Reset the target to the beginning
    m_targetChar = long(0);
    m_processedChar = long(buffer.size() - 1);
}

} // namespace Zep
