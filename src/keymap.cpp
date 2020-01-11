#include "zep/keymap.h"

#include <cassert>

namespace Zep 
{

bool keymap_add(KeyMap& map, const std::string& strCommand, uint32_t commandId)
{
    auto spCurrent = map.spRoot;

    auto itrChar = strCommand.begin();
    while (itrChar != strCommand.end())
    {
        auto ch = *itrChar;
        auto itrRoot = spCurrent->children.find(ch);
        if (itrRoot == spCurrent->children.end())
        {
            auto spNode = std::make_shared<CommandNode>();
            spNode->ch = ch;
            spCurrent->children[ch] = spNode;
            spCurrent = spNode;
        }
        else
        {
            spCurrent = itrRoot->second;
        }
        itrChar++;
    }

    if (spCurrent->commandId != 0)
    {
        assert(!"Adding twice?");
        return false;
    }

    spCurrent->commandId = commandId;
    return true;
}

uint32_t keymap_find(KeyMap& map, const std::string& strCommand, bool& needMore)
{
    auto spCurrent = map.spRoot.get();

    auto itrChar = strCommand.begin();
    while (itrChar != strCommand.end())
    {
        auto ch = *itrChar;
        auto itrRoot = spCurrent->children.find(ch);
        if (itrRoot != spCurrent->children.end())
        {
            spCurrent = itrRoot->second.get();
        }
        else
        {
            // Not found?
            return 0;
        }
        itrChar++;
    }
    if (!spCurrent->children.empty())
    {
        needMore = true;
    }
    return spCurrent->commandId;
}

} // namespace Zep
