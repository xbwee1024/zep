#pragma once

#include <stack>

#include "zep/buffer.h"
#include "zep/display.h"
#include "zep/window.h"
#include "zep/commands.h"
#include "zep/keymap.h"

namespace Zep
{

class ZepEditor;

struct ModeSettings
{
    bool ShowNormalModeKeyStrokes = true;
};

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
}; // ModifierKey

enum class EditorMode
{
    None,
    Normal,
    Insert,
    Visual,
    Ex
};

enum class CommandOperation
{
    None,
    Delete,
    DeleteLines,
    Insert,
    Copy,
    CopyLines,
    Replace,
    Paste
};

namespace CommandResultFlags
{
enum
{
    None = 0,
    HandledCount = (1 << 2), // Command implements the count, no need to recall it.
    NeedMoreChars
};
} // CommandResultFlags

struct CommandResult
{
    uint32_t flags = CommandResultFlags::None;
    EditorMode modeSwitch = EditorMode::None;
    std::shared_ptr<ZepCommand> spCommand;
};

struct CommandContext
{
    CommandContext(const std::string& commandIn, ZepMode& md, uint32_t lastK, uint32_t modifierK, EditorMode editorMode);

    // Parse the command into:
    // [count1] opA [count2] opB
    // And generate (count1 * count2), opAopB
    void GetCommandAndCount();
    void GetCommandRegisters();
    void UpdateRegisters();

    ZepMode& owner;

    std::string commandText;
    std::string commandWithoutCount;
    std::string command;

    const SpanInfo* pLineInfo = nullptr;
    ReplaceRangeMode replaceRangeMode = ReplaceRangeMode::Fill;
    BufferLocation beginRange{-1};
    BufferLocation endRange{-1};
    ZepBuffer& buffer;

    // Cursor State
    BufferLocation bufferCursor{-1};
    BufferLocation cursorAfterOverride{-1};

    // Register state
    std::stack<char> registers;
    Register tempReg;
    const Register* pRegister = nullptr;

    // Input State
    uint32_t lastKey = 0;
    uint32_t modifierKeys = 0;
    EditorMode mode = EditorMode::None;
    int count = 1;
    bool foundCount = false;

    // Output result
    CommandResult commandResult;
    CommandOperation op = CommandOperation::None;

    // Did we get a command?
    bool foundCommand = false;
};
class ZepMode : public ZepComponent
{
public:
    ZepMode(ZepEditor& editor);
    virtual ~ZepMode();

    virtual std::shared_ptr<CommandContext> AddKeyPress(uint32_t key, uint32_t modifierKeys = ModifierKey::None) = 0;
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

    virtual const std::string& GetLastCommand() const;
    virtual bool GetCommand(CommandContext& context);
    virtual void ResetCommand();
    virtual int GetLastCount() const;

    virtual bool GetOperationRange(const std::string& op, EditorMode mode, BufferLocation& beginRange, BufferLocation& endRange) const;

    virtual void UpdateVisualSelection();

protected:
    virtual void SwitchMode(EditorMode mode);
    virtual void ClampCursorForMode();
    virtual bool HandleExCommand(std::string strCommand, const char key);
    virtual std::string ConvertInputToMapString(uint32_t key, uint32_t modifierKeys);

protected:
    std::stack<std::shared_ptr<ZepCommand>> m_undoStack;
    std::stack<std::shared_ptr<ZepCommand>> m_redoStack;
    EditorMode m_currentMode = EditorMode::Normal;
    bool m_lineWise = false;
    BufferLocation m_insertBegin = 0;
    BufferLocation m_visualBegin = 0;
    BufferLocation m_visualEnd = 0;
    std::string m_lastCommand;
    int m_lastCount;

    // Keyboard mappings
    KeyMap m_normalMap;
    KeyMap m_visualMap;
    KeyMap m_insertMap;
    KeyMap m_exMap;
    
    SearchDirection m_lastFindDirection = SearchDirection::Forward;
    SearchDirection m_lastSearchDirection = SearchDirection::Forward;

    std::string m_currentCommand;
    std::string m_lastInsertString;
    std::string m_lastFind;

    BufferLocation m_exCommandStartLocation = 0;
    bool m_pendingEscape = false;
    ModeSettings m_settings;
    std::string m_textHistory;
};

} // namespace Zep
