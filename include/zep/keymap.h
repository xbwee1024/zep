#pragma once

#include <functional>
#include <string>

#include "mcommon/string/stringutils.h"

namespace Zep
{

class ZepMode;

#define DECLARE_COMMANDID(name) const StringId id_##name(#name);

DECLARE_COMMANDID(YankLine)
DECLARE_COMMANDID(InsertMode)

DECLARE_COMMANDID(VisualSelectInnerWORD)
DECLARE_COMMANDID(VisualSelectInnerWord)

DECLARE_COMMANDID(JoinLines)
DECLARE_COMMANDID(OpenLineBelow)
DECLARE_COMMANDID(OpenLineAbove)

DECLARE_COMMANDID(Delete)

DECLARE_COMMANDID(VisualMode)
DECLARE_COMMANDID(VisualLineMode)

DECLARE_COMMANDID(SwitchToAlternateFile)

DECLARE_COMMANDID(FontBigger)
DECLARE_COMMANDID(FontSmaller)

DECLARE_COMMANDID(QuickSearch)

DECLARE_COMMANDID(Undo)
DECLARE_COMMANDID(Redo)

DECLARE_COMMANDID(MotionNextMarker)
DECLARE_COMMANDID(MotionPreviousMarker)

DECLARE_COMMANDID(MotionLineEnd)
DECLARE_COMMANDID(MotionLineBegin)
DECLARE_COMMANDID(MotionLineFirstChar)

DECLARE_COMMANDID(MotionRightSplit)
DECLARE_COMMANDID(MotionLeftSplit)
DECLARE_COMMANDID(MotionUpSplit)
DECLARE_COMMANDID(MotionDownSplit)

DECLARE_COMMANDID(MotionDown)
DECLARE_COMMANDID(MotionUp)
DECLARE_COMMANDID(MotionLeft)
DECLARE_COMMANDID(MotionRight)

DECLARE_COMMANDID(MotionWord)
DECLARE_COMMANDID(MotionBackWord)
DECLARE_COMMANDID(MotionWORD)
DECLARE_COMMANDID(MotionBackWORD)
DECLARE_COMMANDID(MotionEndWord)
DECLARE_COMMANDID(MotionEndWORD)
DECLARE_COMMANDID(MotionBackEndWord)
DECLARE_COMMANDID(MotionBackEndWORD)
DECLARE_COMMANDID(MotionGotoBeginning)

DECLARE_COMMANDID(MotionPageForward)
DECLARE_COMMANDID(MotionPageBackward)

DECLARE_COMMANDID(MotionHalfPageForward)
DECLARE_COMMANDID(MotionHalfPageBackward)

DECLARE_COMMANDID(MotionGotoLine)

DECLARE_COMMANDID(PreviousTabWindow)
DECLARE_COMMANDID(NextTabWindow)


// Insert Mode
DECLARE_COMMANDID(Backspace)
struct CommandNode
{
    std::string token;
    uint32_t commandId;
    std::unordered_map<std::string, std::shared_ptr<CommandNode>> children;
};

struct KeyMap
{
    std::shared_ptr<CommandNode> spRoot = std::make_shared<CommandNode>();
};

bool keymap_add(const std::vector<KeyMap*>& maps, const std::vector<std::string>& strCommand, uint32_t commandId);
bool keymap_add(KeyMap& map, const std::string& strCommand, const uint32_t commandId);
uint32_t keymap_find(KeyMap& map, const std::string& strCommand, bool& needMore);

} // namespace Zep
