#pragma once

#include <functional>
#include <string>

#include "mcommon/string/stringutils.h"

namespace Zep
{

class ZepMode;

#define DECLARE_COMMANDID(name) const StringId id_##name(#name);

// Normal mode commands 
DECLARE_COMMANDID(YankLine)
DECLARE_COMMANDID(InsertMode)

DECLARE_COMMANDID(VisualSelectInnerWORD)
DECLARE_COMMANDID(VisualSelectInnerWord)

DECLARE_COMMANDID(MotionLineEnd)
DECLARE_COMMANDID(MotionLineBegin)
DECLARE_COMMANDID(MotionLineFirstChar)

struct CommandNode
{
    char ch = 0;
    uint32_t commandId;
    std::unordered_map<char, std::shared_ptr<CommandNode>> children;
};

struct KeyMap
{
    std::shared_ptr<CommandNode> spRoot = std::make_shared<CommandNode>();
};

bool keymap_add(KeyMap& map, const std::string& strCommand, const uint32_t commandId);
uint32_t keymap_find(KeyMap& map, const std::string& strCommand, bool& needMore);

} // namespace Zep
