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
    if (string_tolower(str) == #a)    \
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

// Keyboard mapping strings such as <PageDown> get converted here
std::string keymap_string(const std::string& str)
{
    return str;
}

// Splitting the input into groups of <> or ch
std::string NextToken(std::string::const_iterator& itrChar, std::string::const_iterator itrEnd)
{
    std::ostringstream str;
    // Find a group
    if (*itrChar == '<')
    {
        str << "<";

        bool lastShiftCtrl = true;
        itrChar++;

        // Walk the group, ensuring we consistently output (C-)(S-)foo
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

                    str << std::string(itrChar, itrTokenEnd);
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
        str << ">";
    }
    else
    {
        str << *itrChar++;
    }

    // Return the converted string
    return str.str();
}

// Add a collection of commands to a collection of mappings
bool keymap_add(const std::vector<KeyMap*>& maps, const std::vector<std::string>& commands, const StringId& commandId)
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

bool keymap_add(KeyMap& map, const std::string& strCommand, const StringId& commandId)
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

void keymap_dump(const KeyMap& map)
{
#ifdef _DEBUG 
    std::function<void(std::shared_ptr<CommandNode>, int)> fnDump;
    fnDump = [&](std::shared_ptr<CommandNode> node, int depth) {
        std::ostringstream str;
        for (int i = 0; i < depth; i++)
        {
            str << " ";
        }
        str << node->token;
        if (node->commandId != 0)
            str << " : " << node->commandId.ToString();
        LOG(DEBUG) << str.str();

        for (auto& child : node->children)
        {
            fnDump(child.second, depth + 2);
        }
    };
    fnDump(map.spRoot, 0);
#endif
}

// Walk the tree of tokens, figuring out which command this is
void keymap_find(const KeyMap& map, const std::string& strCommand, KeyMapResult& result)
{
    auto spCurrent = map.spRoot.get();

#ifdef _DEBUG
    LOG(DEBUG) << "keymap_find: " << strCommand;
    std::ostringstream str;
#endif

    auto itrChar = strCommand.begin();
    while (itrChar != strCommand.end())
    {
        std::string token;
        if (*itrChar == '<')
        {
            auto itrStart = itrChar;
            do 
            {
                itrChar++;
            } while (itrChar != strCommand.end() && *itrChar != '>');

            token = std::string(itrStart, itrChar);
        }
        else if (std::isdigit(*itrChar))
        {
            auto itrStart = itrChar;
            do
            { 
                itrChar++;
            } while (itrChar != strCommand.end() && std::isdigit(*itrChar));

            auto digit = std::stoi(std::string(itrStart, itrChar));
            result.countGroups.push_back(digit);
            token = std::string("<n>");
        }
        else
        {
            token = *itrChar++;
        }

        auto itrRoot = spCurrent->children.find(token);
        if (itrRoot != spCurrent->children.end())
        {
#ifdef _DEBUG
            str << "/" << itrRoot->first;
            if (itrRoot->second->commandId != 0)
                str << " (" << itrRoot->second->commandId.ToString() << ")";
#endif
            spCurrent = itrRoot->second.get();
        }
        else
        {
            // Not found?
            result.needMoreChars = false;
#ifdef _DEBUG
            LOG(DEBUG) << str.str();
#endif
            return;
        }
    }

    if (spCurrent != map.spRoot.get() && !spCurrent->children.empty())
    {
        result.needMoreChars = true;
    }

    // Calculate the total count
    if (!result.countGroups.empty())
    {
        result.totalCount = 1;
        for (auto& g : result.countGroups)
        {
            result.totalCount *= g;
        }

    }
    result.foundMapping = spCurrent->commandId;
            
#ifdef _DEBUG
    LOG(DEBUG) << str.str();
#endif
}

} // namespace Zep
