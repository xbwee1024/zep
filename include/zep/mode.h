#pragma once

#include "buffer.h"
#include "display.h"
#include <stack>

namespace Zep
{

class ZepEditor;
class ZepCommand;
class ZepWindow;

// NOTE: These are input keys mapped to Zep's internal keymapping; they live below 'space'/32
// Key mapping needs a rethink for international keyboards.  But for modes, this is the remapped key definitions for anything that isn't
// basic ascii symbol.  ASCII 0-31 are mostly ununsed these days anyway.
struct ExtKeys
{
    enum Key
    {
        RETURN = 0, // NOTE: Do not change this value
        ESCAPE = 1,
        BACKSPACE = 2,
        LEFT = 3,
        RIGHT = 4,
        UP = 5,
        DOWN = 6,
        TAB = 7,
        DEL = 8,
        HOME = 9,
        END = 10,
        PAGEDOWN = 11,
        PAGEUP = 12,
        F1 = 13,
        F2 = 14,
        F3 = 15,
        F4 = 16,
        F5 = 17,
        F6 = 18,
        F7 = 19,
        F8 = 20,
        F9 = 21,
        F10 = 22,
        F11 = 23,
        F12 = 24,

        // Note: No higher than 31
        LAST = 31,
        NONE = 32
    };
};

struct ModifierKey
{
    enum Key
    {
        None = (0),
        Ctrl = (1 << 0),
        Alt = (1 << 1),
        Shift = (1 << 2)
    };
};

enum class EditorMode
{
    None,
    Normal,
    Insert,
    Visual,
    Ex
};

class ZepMode : public ZepComponent
{
public:
    ZepMode(ZepEditor& editor);
    virtual ~ZepMode();

    virtual void AddKeyPress(uint32_t key, uint32_t modifierKeys = ModifierKey::None) = 0;
    virtual const char* Name() const = 0;
    virtual void Begin() = 0;
    virtual void Notify(std::shared_ptr<ZepMessage> message) override {}

    // Keys handled by modes
    virtual void AddCommandText(std::string strText);
    virtual void AddCommand(std::shared_ptr<ZepCommand> spCmd);
    virtual EditorMode GetEditorMode() const;
    virtual void SetEditorMode(EditorMode mode);

    // ZepComponent
    virtual void PreDisplay(){};

    // Called when we begin editing in this mode

    virtual void Undo();
    virtual void Redo();

    virtual ZepWindow* GetCurrentWindow() const;

    virtual NVec2i GetVisualRange() const;

    virtual bool HandleGlobalCtrlCommand(const std::string& cmd, uint32_t modifiers, bool& needMoreChars);
    virtual bool HandleGlobalCommand(const std::string& cmd, uint32_t modifiers, bool& needMoreChars);

protected:
    std::stack<std::shared_ptr<ZepCommand>> m_undoStack;
    std::stack<std::shared_ptr<ZepCommand>> m_redoStack;
    EditorMode m_currentMode = EditorMode::Normal;
    bool m_lineWise = false;
    BufferLocation m_insertBegin = 0;
    BufferLocation m_visualBegin = 0;
    BufferLocation m_visualEnd = 0;
};

} // namespace Zep
