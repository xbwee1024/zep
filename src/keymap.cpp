#include <cassert>

#include "zep/keymap.h"
#include "zep/mode.h"

#include "zep/mcommon/logger.h"

namespace Zep
{

// Keyboard mapping strings such as <PageDown> get converted here
ExtKeys::Key MapStringToExKey(const std::string& str)
{
#define COMPARE(a, b) \
    if (str == #a)    \
        return b;

    COMPARE(return, ExtKeys::RETURN)
    COMPARE(escape, ExtKeys::ESCAPE)
    COMPARE(backspace, ExtKeys::BACKSPACE)
    COMPARE(left, ExtKeys::LEFT)
    COMPARE(right, ExtKeys::RIGHT)
    COMPARE(up, ExtKeys::UP)
    COMPARE(down, ExtKeys::DOWN)
    COMPARE(tab, ExtKeys::TAB)
    COMPARE(del, ExtKeys::DEL)
    COMPARE(home, ExtKeys::HOME)
    COMPARE(end, ExtKeys::END)
    COMPARE(pagedown, ExtKeys::PAGEDOWN)
    COMPARE(pageup, ExtKeys::PAGEUP)
    COMPARE(f1, ExtKeys::F1)
    COMPARE(f2, ExtKeys::F2)
    COMPARE(f3, ExtKeys::F3)
    COMPARE(f4, ExtKeys::F4)
    COMPARE(f5, ExtKeys::F5)
    COMPARE(f6, ExtKeys::F6)
    COMPARE(f7, ExtKeys::F7)
    COMPARE(f8, ExtKeys::F8)
    COMPARE(f9, ExtKeys::F9)
    COMPARE(f10, ExtKeys::F10)
    COMPARE(f11, ExtKeys::F11)
    COMPARE(f12, ExtKeys::F12)

    return ExtKeys::NONE;
}

// Splitting the input into groups of <> or ch
std::string NextToken(std::string::const_iterator& itrChar, std::string::const_iterator itrEnd)
{
    std::ostringstream str;
    // Find a group
    if (*itrChar == '<')
    {
        bool lastShiftCtrl = true;
        itrChar++;

        // Walk the group
        while (itrChar != itrEnd && *itrChar != '>')
        {
            // Handle <c-s-, <C-s-, <C-, <c-, etc.
            // Always capitalize C and S when in keymaps
            if (lastShiftCtrl && (*itrChar == 's' || *itrChar == 'S'))
            {
                str << "S";
                lastShiftCtrl = false;
                itrChar++;
            }
            else if (lastShiftCtrl && (*itrChar == 'c' || *itrChar == 'C'))
            {
                str << "C";
                lastShiftCtrl = false;
                itrChar++;
            }
            else if (*itrChar == '-')
            {
                lastShiftCtrl = true;
                str << "-";
                itrChar++;
            }
            else
            {
                // Convert the alpha characters here 
                if (std::isalpha(*itrChar))
                {
                    auto itrTokenEnd = itrChar;
                    while (itrTokenEnd != itrEnd && std::isalpha(*itrTokenEnd))
                        itrTokenEnd++;

                    auto tok = string_tolower(std::string(itrChar, itrTokenEnd));
                    auto key = MapStringToExKey(tok);
                    if (key != ExtKeys::NONE)
                    {
                        str << (char)key;
                    }
                    else
                    {
                        // Just a random char/string, not a special character
                        str << tok;
                    }
                    itrChar = itrTokenEnd;
                }
                else
                {
                    str << *itrChar++;
                }
                lastShiftCtrl = false;
            }
        }
        if (itrChar != itrEnd && *itrChar == '>')
            itrChar++;
    }
    else
    {
        str << *itrChar++;
    }

    // Return the converted string
    return str.str();
}

// Add a collection of commands to a collection of mappings
bool keymap_add(const std::vector<KeyMap*>& maps, const std::vector<std::string>& commands, uint32_t commandId)
{
    bool ret = true;
    for (auto& map : maps)
    {
        for (auto& cmd : commands)
        {
            if (!keymap_add(*map, cmd, commandId))
                ret = false;
        }
    }
    return ret;
}

bool keymap_add(KeyMap& map, const std::string& strCommand, uint32_t commandId)
{
    auto spCurrent = map.spRoot;

    std::ostringstream str;
    auto itrChar = strCommand.begin();
    while (itrChar != strCommand.end())
    {
        auto search = NextToken(itrChar, strCommand.end());
        auto itrRoot = spCurrent->children.find(search);
        if (itrRoot == spCurrent->children.end())
        {
            auto spNode = std::make_shared<CommandNode>();
            spNode->token = search;
            spCurrent->children[search] = spNode;
            spCurrent = spNode;
        }
        else
        {
            spCurrent = itrRoot->second;
        }
    }

    if (spCurrent->commandId != 0)
    {
        assert(!"Adding twice?");
        return false;
    }

    spCurrent->commandId = commandId;
    return true;
}

// Walk the tree of tokens, figuring out which command this is
uint32_t keymap_find(KeyMap& map, const std::string& strCommand, bool& needMore)
{
    auto spCurrent = map.spRoot.get();

#ifdef _DEBUG
    std::ostringstream strDebugOut;
    for (auto& ch : strCommand)
    {
        if (ch < ' ')
        {
            strDebugOut << "\\x" << (int)(ch);
        }
        strDebugOut << ch;
    }
    LOG(DEBUG) << "keymap_find: " << strDebugOut.str();
#endif

    std::ostringstream str;
    auto itrChar = strCommand.begin();
    while (itrChar != strCommand.end())
    {
        auto search = NextToken(itrChar, strCommand.end());
        auto itrRoot = spCurrent->children.find(search);
        if (itrRoot != spCurrent->children.end())
        {
            spCurrent = itrRoot->second.get();
        }
        else
        {
            // Not found?
            needMore = false;
            return 0;
        }
    }
    if (spCurrent != map.spRoot.get() && !spCurrent->children.empty())
    {
        needMore = true;
    }
    return spCurrent->commandId;
}

} // namespace Zep
