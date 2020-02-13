#include <cassert>
#include <regex>

#include "zep/keymap.h"
#include "zep/mode.h"

#include "zep/mcommon/logger.h"

namespace Zep
{

// Keyboard mapping strings such as <PageDown> get converted here
ExtKeys::Key MapStringToExKey(const std::string& str)
{
#define COMPARE(a, b)              \
    if (string_tolower(str) == #a) \
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
        itrChar++;
        auto itrStart = itrChar;

        // Walk the group, ensuring we consistently output (C-)(S-)foo
        while (itrChar != itrEnd && *itrChar != '>')
        {
            itrChar++;
        }

        // Ensure <C-S- or variants, ie. capitalize for consistency if the mapping
        // was badly supplied
        auto strGroup = std::string(itrStart, itrChar);
        string_replace_in_place(strGroup, "c-", "C-");
        string_replace_in_place(strGroup, "s-", "S-");

        // Skip to the next
        if (itrChar != itrEnd)
            itrChar++;

        str << "<" << strGroup << ">";
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

void keymap_dump(const KeyMap& map, std::ostringstream& str)
{
    std::function<void(std::shared_ptr<CommandNode>, int)> fnDump;
    fnDump = [&](std::shared_ptr<CommandNode> node, int depth) {
        for (int i = 0; i < depth; i++)
        {
            str << " ";
        }
        str << node->token;
        if (node->commandId != 0)
            str << " : " << node->commandId.ToString();
        str << std::endl;

        for (auto& child : node->children)
        {
            fnDump(child.second, depth + 2);
        }
    };
    fnDump(map.spRoot, 0);
}

// Walk the count groups in the keymap and extract the counts, removing them from the input
void FindCounts(std::string& input, const KeyMap& map, KeyMapResult& res, std::ostringstream& str)
{
    // First map the groups
    for (auto& g : map.m_countGroups)
    {
        std::smatch match;
        while (std::regex_search(input, match, g))
        {
            if (match.size() > 1)
            {
                try
                {
                    res.countGroups.push_back(std::stoi(match[1].str()));
                    input.erase(match.position(1), match.length(1));
                    str << "(count: " << res.countGroups[res.countGroups.size() - 1] << ")";
                }
                catch (std::exception& e)
                {
                    LOG(DEBUG) << e.what();
                }
            }
        }
    }
    
    // Calculate the total count
    if (!res.countGroups.empty())
    {
        res.totalCount = 1;
        for (auto& g : res.countGroups)
        {
            res.totalCount *= g;
        }
    }
}

// Walk the count groups in the keymap and extract the counts, removing them from the input
void FindRegisters(std::string& input, const KeyMap& map, KeyMapResult& res, std::ostringstream& str)
{
    // First map the groups
    for (auto& g : map.m_registerGroups)
    {
        std::smatch match;
        while (std::regex_search(input, match, g))
        {
            if (match.size() > 1)
            {
                if (match[1].length() > 1)
                {
                    res.registerName = match[1].str()[1];
                    input.erase(match.position(1), match.length(1));
                    str << "(register: " << res.registerName << ")";
                }
            }
        }
    }
}

// Erase unfinished strings from the input, leaving an empty string that will return
// more chars required
void IgnoreUnfinished(std::string& input, const KeyMap& map)
{
    // First map the groups
    for (auto& g : map.m_unfinishedGroups)
    {
        input = std::regex_replace(input, g, "");
    }
}

// Walk the tree of tokens, figuring out which command this is
void keymap_find(const KeyMap& map, const std::string& strCommand, KeyMapResult& result)
{
    auto spCurrent = map.spRoot.get();

    std::string input = strCommand;

    std::ostringstream str;
    str << input << " - ";

    // Special case for now...
    if (strCommand != "0")
    {
        FindCounts(input, map, result, str);
        FindRegisters(input, map, result, str);
        IgnoreUnfinished(input, map);
    }

    result.commandWithoutGroups = input;

    // Did the regex eat the groups?  If so, we haven't got to the command yet
    if (input.empty())
    {
        str << " : ...";
        LOG(DEBUG) << str.str();
        result.needMoreChars = true;
        return;
    }

    // Walk the remaining input
    auto itrChar = input.begin();
    while (itrChar != input.end())
    {
        // Grab full <> tokens
        std::string token = string_slurp_if(input, itrChar, '<', '>');
        if (token.empty())
        {
            token = std::string(itrChar, itrChar + 1);
            string_eat_char(input, itrChar);
        }

        // Search..
        auto checkToken = [&](const std::string& token) {
            if (token.empty())
            {
                return false;
            }

            auto itrRoot = spCurrent->children.find(token);
            if (itrRoot != spCurrent->children.end())
            {
                str << "(" << itrRoot->first << ")";
                if (itrRoot->second->commandId != 0)
                    str << " : " << itrRoot->second->commandId.ToString();

                spCurrent = itrRoot->second.get();
                return true;
            }
            else
            {
                return false;
            }
        };

        // Didn't find this token in the tree.
        if (!checkToken(token))
        {
            // Check for a wildcard match
            if (!checkToken("<.>"))
            {
                // Not found either; no joy, unless this is a trailing number and there is more to come
                if (map.ignoreFinalDigit && std::isdigit(input[input.size() - 1]))
                {
                    str << " : ...";
                    result.needMoreChars = true;
                }
                else
                {
                    str << " : Not recognized";
                    result.needMoreChars = false;
                }
                LOG(DEBUG) << str.str();
                return;
            }
            else
            {
                // Found a match for a capture, so that's OK, keep going
                result.captureChars.push_back(token[0]);
                str << "(capture: " << token << ")";
            }
        }
    }

    if (spCurrent != map.spRoot.get() && !spCurrent->children.empty())
    {
        result.needMoreChars = true;
    }

    result.foundMapping = spCurrent->commandId;

    LOG(DEBUG) << str.str();
}

} // namespace Zep
