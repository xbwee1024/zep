#include "zep/mode_standard.h"
#include "zep/mcommon/string/stringutils.h"

// Note:
// This is a version of the buffer that behaves like notepad.
// It is basic, but can easily be extended

// STANDARD:
//
// DONE:
// -----
// CTRLZ/Y Undo Redo
// Insert
// Delete/Backspace
// TAB
// Arrows - up,down, left, right
// Home (+Ctrl) move top/startline
// End (+Ctrol) move bottom/endline
// Shift == Select
// control+Shift == select word
// CTRL - CVX (copy paste, cut) + Delete Selection

namespace Zep
{

ZepMode_Standard::ZepMode_Standard(ZepEditor& editor)
    : ZepMode(editor)
{
    Init();
}

ZepMode_Standard::~ZepMode_Standard()
{
}

void ZepMode_Standard::Init()
{
    for (int i = 0; i <= 9; i++)
    {
        GetEditor().SetRegister('0' + (const char)i, "");
    }
    GetEditor().SetRegister('"', "");

    /*
    // Normal and Visual
    keymap_add({ &m_normalMap, &m_visualMap }, { "Y" }, id_YankLine);
    keymap_add({ &m_normalMap, &m_visualMap }, { "x", "<Del>" }, id_Delete);
    keymap_add({ &m_normalMap, &m_visualMap }, { "J" }, id_JoinLines);

    // Motions

    // Line Motions
    keymap_add({ &m_normalMap, &m_visualMap }, { "$" }, id_MotionLineEnd);
    keymap_add({ &m_normalMap, &m_visualMap }, { "0" }, id_MotionLineBegin);
    keymap_add({ &m_normalMap, &m_visualMap }, { "^" }, id_MotionLineFirstChar);

    // Page Motinos
    keymap_add({ &m_normalMap, &m_visualMap }, { "j", "<Down>" }, id_MotionDown);
    keymap_add({ &m_normalMap, &m_visualMap }, { "k", "<Up>" }, id_MotionUp);
    keymap_add({ &m_normalMap, &m_visualMap }, { "l", "<Right>" }, id_MotionRight);
    keymap_add({ &m_normalMap, &m_visualMap }, { "h", "<Left>", "<Backspace>" }, id_MotionLeft);
    keymap_add({ &m_normalMap, &m_visualMap }, { "<C-f>", "<PageDown>" }, id_MotionPageForward);
    keymap_add({ &m_normalMap, &m_visualMap }, { "<C-b>", "<PageUp>" }, id_MotionPageBackward);
    keymap_add({ &m_normalMap, &m_visualMap }, { "<C-d>" }, id_MotionHalfPageForward);
    keymap_add({ &m_normalMap, &m_visualMap }, { "<C-u>" }, id_MotionHalfPageBackward);
    keymap_add({ &m_normalMap, &m_visualMap }, { "G" }, id_MotionGotoLine);

    // Word motions
    keymap_add({ &m_normalMap, &m_visualMap }, { "w" }, id_MotionWord);
    keymap_add({ &m_normalMap, &m_visualMap }, { "b" }, id_MotionBackWord);
    keymap_add({ &m_normalMap, &m_visualMap }, { "W" }, id_MotionWORD);
    keymap_add({ &m_normalMap, &m_visualMap }, { "B" }, id_MotionBackWORD);
    keymap_add({ &m_normalMap, &m_visualMap }, { "e" }, id_MotionEndWord);
    keymap_add({ &m_normalMap, &m_visualMap }, { "E" }, id_MotionEndWORD);
    keymap_add({ &m_normalMap, &m_visualMap }, { "ge" }, id_MotionBackEndWord);
    keymap_add({ &m_normalMap, &m_visualMap }, { "gE" }, id_MotionBackEndWORD);
    keymap_add({ &m_normalMap, &m_visualMap }, { "gg" }, id_MotionGotoBeginning);

    // Not necessary?
    keymap_add({ &m_normalMap, &m_visualMap, &m_insertMap }, { "<Escape>" }, id_NormalMode);

    // Visual mode
    keymap_add(m_visualMap, "iW", id_VisualSelectInnerWORD);
    keymap_add(m_visualMap, "iw", id_VisualSelectInnerWord);

    // Normal mode only
    keymap_add(m_normalMap, "i", id_InsertMode);
    keymap_add(m_normalMap, "H", id_PreviousTabWindow);
    keymap_add(m_normalMap, "L", id_NextTabWindow);
    keymap_add(m_normalMap, "o", id_OpenLineBelow);
    keymap_add(m_normalMap, "O", id_OpenLineAbove);
    keymap_add(m_normalMap, "V", id_VisualLineMode);
    keymap_add(m_normalMap, "v", id_VisualMode);

    keymap_add(m_normalMap, "<F8>", id_MotionNextMarker);
    keymap_add(m_normalMap, "<S-F8>", id_MotionPreviousMarker);

    keymap_add(m_normalMap, "+", id_FontBigger);
    keymap_add(m_normalMap, "-", id_FontSmaller);

    keymap_add(m_normalMap, "<C-i><C-o>", id_SwitchToAlternateFile);

    keymap_add(m_normalMap, "<C-j>", id_MotionDownSplit);
    keymap_add(m_normalMap, "<C-l>", id_MotionRightSplit);
    keymap_add(m_normalMap, "<C-k>", id_MotionUpSplit);
    keymap_add(m_normalMap, "<C-h>", id_MotionLeftSplit);

    keymap_add({ &m_normalMap }, { "<C-p>", "<C-,>" }, id_QuickSearch);

    keymap_add({ &m_normalMap, &m_visualMap }, { ":", "/", "?" }, id_ExMode);

    /* Standard mode
    keymap_add({ &m_normalMap }, { "<C-y>"}, id_Redo);
    */

    // Insert Mode
    keymap_add({ &m_insertMap }, { "<Backspace>" }, id_Backspace);
    keymap_add({ &m_insertMap }, { "<Del>" }, id_Delete);
    keymap_add({ &m_insertMap }, { "<C-y>" }, id_Redo);
    keymap_add({ &m_insertMap }, { "<C-z>" }, id_Undo);

    keymap_add({ &m_insertMap }, { "<Left>" }, id_MotionStandardLeft);
    keymap_add({ &m_insertMap }, { "<Right>" }, id_MotionStandardRight);
    keymap_add({ &m_insertMap }, { "<Up>" }, id_MotionStandardUp);
    keymap_add({ &m_insertMap }, { "<Down>" }, id_MotionStandardDown);
    
    keymap_add({ &m_insertMap }, { "<C-Left>" }, id_MotionBackWord);
    keymap_add({ &m_insertMap }, { "<C-Right>" }, id_MotionWord);
}

void ZepMode_Standard::Begin()
{
    // This will also set the cursor type
    SwitchMode(EditorMode::Insert);
}

/*void ZepMode_Standard::AddKeyPress(uint32_t key, uint32_t modifierKeys)
{
    ZepMode::AddKeyPress(key, modifierKeys);

    /*
    std::string ch(1, (char)key);

    bool copyRegion = false;
    bool lineWise = false;
    auto& buffer = GetCurrentWindow()->GetBuffer();
    BufferLocation bufferCursor = GetCurrentWindow()->GetBufferCursor();
    BufferLocation startOffset = bufferCursor;
    BufferLocation endOffset = buffer.LocationFromOffsetByChars(bufferCursor, long(ch.length()));

    CommandOperation op = CommandOperation::None;

    auto normalizeOffsets = [&]() {
        // Clamp and orient the correct way around
        startOffset = buffer.Clamp(startOffset);
        endOffset = buffer.Clamp(endOffset);
        if (startOffset > endOffset)
        {
            std::swap(startOffset, endOffset);
        }
    };

    if (key == ExtKeys::ESCAPE)
    {
        SwitchMode(EditorMode::Insert);
        return spContext;
    }

    bool begin_shift = false;
    bool return_to_insert = false;
    switch (key)
    {
        case ExtKeys::DOWN:
        case ExtKeys::UP:
        case ExtKeys::LEFT:
        case ExtKeys::RIGHT:
        case ExtKeys::END:
        case ExtKeys::HOME:
        case ExtKeys::PAGEDOWN:
        case ExtKeys::PAGEUP:
            if (modifierKeys & ModifierKey::Shift)
            {
                begin_shift = (m_currentMode != EditorMode::Visual) ? true : false;
                SwitchMode(EditorMode::Visual);
            }
            else
            {
                // Immediate switch back to insert
                SwitchMode(EditorMode::Insert);
            }
            break;
        default:
            return_to_insert = true;
            break;
    }

    // CTRL + ...
    if (modifierKeys & ModifierKey::Ctrl)
    {
        // CTRL + keys common to modes
        bool needMoreChars = false; // TODO in standard mode!
        keyCache += (const char)key;
        if (HandleGlobalCtrlCommand(keyCache, modifierKeys, needMoreChars))
        {
            if (!needMoreChars)
            {
                keyCache.clear();
            }
            return spContext;
        }
        keyCache.clear();
       
        // Undo
        if (key == 'z')
        {
            Undo();
            return spContext;
        }
        // Redo
        else if (key == 'y')
        {
            Redo();
            return spContext;
        }
        // Motions fall through to selection code
        else if (key == ExtKeys::RIGHT)
        {
            auto target = buffer.StandardCtrlMotion(bufferCursor, SearchDirection::Forward);
            GetCurrentWindow()->SetBufferCursor(target.second);
        }
        else if (key == ExtKeys::LEFT)
        {
            auto target = buffer.StandardCtrlMotion(bufferCursor, SearchDirection::Backward);
            GetCurrentWindow()->SetBufferCursor(target.second);
        }
        else if (key == ExtKeys::HOME)
        {
            // CTRL HOME = top of file
            GetCurrentWindow()->SetBufferCursor(BufferLocation{0});
        }
        else if (key == ExtKeys::END)
        {
            // CTRL END = end of file
            GetCurrentWindow()->SetBufferCursor(buffer.EndLocation());
        }
        // Cut/Copy
        else if (key == 'x')
        {
            // A delete, but also a copy!
            op = CommandOperation::Delete;
            copyRegion = true;
            if (m_currentMode == EditorMode::Visual)
            {
                startOffset = m_visualBegin;
                endOffset = m_visualEnd;
                normalizeOffsets();
            }
            else
            {
                lineWise = true;
                startOffset = buffer.GetLinePos(bufferCursor, LineLocation::LineBegin);
                endOffset = buffer.GetLinePos(bufferCursor, LineLocation::BeyondLineEnd);
            }
        }
        // Copy
        else if (key == 'c')
        {
            op = CommandOperation::Copy;
            if (m_currentMode == EditorMode::Visual)
            {
                startOffset = m_visualBegin;
                endOffset = m_visualEnd;
                normalizeOffsets();
                return_to_insert = false;
            }
            else
            {
                lineWise = true;
                startOffset = buffer.GetLinePos(bufferCursor, LineLocation::LineBegin);
                endOffset = buffer.GetLinePos(bufferCursor, LineLocation::BeyondLineEnd);
            }
        }
        // Paste
        else if (key == 'v')
        {
            auto pRegister = &GetEditor().GetRegister('"');
            if (!pRegister->text.empty())
            {
                ch = pRegister->text;
            }
            else
            {
                ch.clear();
            }
            op = CommandOperation::Insert;
            startOffset = bufferCursor;
            endOffset = startOffset + BufferLocation(ch.length());
        }
    }
    else if (key == ExtKeys::HOME)
    {
        // Beginning of line
        auto pos = buffer.GetLinePos(bufferCursor, LineLocation::LineFirstGraphChar);
        GetCurrentWindow()->SetBufferCursor(pos);
        GetEditor().ResetCursorTimer();
    }
    else if (key == ExtKeys::END)
    {
        auto pos = buffer.GetLinePos(bufferCursor, LineLocation::LineCRBegin);
        GetCurrentWindow()->SetBufferCursor(pos);
        GetEditor().ResetCursorTimer();
    }
    else if (key == ExtKeys::RETURN)
    {
        ch = "\n";
        op = CommandOperation::Insert;
    }
    else if (key == ExtKeys::TAB)
    {
        // 4 Spaces, obviously :)
        ch = "    ";
        op = CommandOperation::Insert;
    }
    else if (key == ExtKeys::DEL)
    {
        op = CommandOperation::Delete;
        if (m_currentMode == EditorMode::Visual)
        {
            startOffset = m_visualBegin;
            endOffset = m_visualEnd;
            normalizeOffsets();
        }
        else
        {
            endOffset = buffer.LocationFromOffsetByChars(startOffset, 1);
        }
    }
    else if (key == ExtKeys::BACKSPACE)
    {
        op = CommandOperation::Delete;
        if (m_currentMode == EditorMode::Visual)
        {
            startOffset = m_visualBegin;
            endOffset = m_visualEnd;
            normalizeOffsets();
        }
        else
        {
            endOffset = startOffset;
            startOffset = buffer.LocationFromOffsetByChars(startOffset, -1);
        }
    }
    else
    {
        op = CommandOperation::Insert;
    }

    startOffset = buffer.Clamp(startOffset);
    endOffset = buffer.Clamp(endOffset);

    // Here we handle the logic of wether the visual range starts or ends before or
    // after the cursor!  It depends on which direction the user moved when they started the selection
    if (m_currentMode == EditorMode::Visual)
    {
        if (begin_shift)
        {
            if (GetCurrentWindow()->GetBufferCursor() <= startOffset)
            {
                m_visualBegin = startOffset; // buffer.LocationFromOffsetByChars(startOffset, -1);
            }
            else
            {
                m_visualBegin = startOffset;
            }
        }
        if (GetCurrentWindow()->GetBufferCursor() > m_visualBegin)
        {
            m_visualEnd = GetCurrentWindow()->GetBufferCursor();
        }
        else
        {
            m_visualEnd = GetCurrentWindow()->GetBufferCursor(); // buffer.LocationFromOffsetByChars(GetCurrentWindow()->GetBufferCursor(), -1);
        }
    }

    // Op is a copy or also requires the region to be copied
    if (copyRegion || op == CommandOperation::Copy)
    {
        // Grab it
        std::string str = std::string(buffer.GetText().begin() + startOffset, buffer.GetText().begin() + endOffset);
        GetEditor().GetRegister('"').text = str;
        GetEditor().GetRegister('"').lineWise = lineWise;
        GetEditor().GetRegister('0').text = str;
        GetEditor().GetRegister('0').lineWise = lineWise;

        // Copy doesn't clear the visual seletion
        return_to_insert = false;
    }

    // check other cases (might copy && delete!)
    if (op == CommandOperation::Insert)
    {
        bool boundary = false;
        if (m_currentMode == EditorMode::Visual)
        {
            startOffset = m_visualBegin;
            endOffset = m_visualEnd;
            normalizeOffsets();

            // Delete existing selection
            auto cmd = std::make_shared<ZepCommand_DeleteRange>(buffer, startOffset, endOffset, GetCurrentWindow()->GetBufferCursor());
            if (!ch.empty())
            {
                cmd->SetFlags(CommandFlags::GroupBoundary);
            }
            AddCommand(std::static_pointer_cast<ZepCommand>(cmd));
            boundary = true;
        }

        // Simple insert
        if (!ch.empty())
        {
            auto cmd = std::make_shared<ZepCommand_Insert>(buffer, startOffset, ch, GetCurrentWindow()->GetBufferCursor());
            if (boundary)
            {
                cmd->SetFlags(CommandFlags::GroupBoundary);
            }
            AddCommand(std::static_pointer_cast<ZepCommand>(cmd));
        }
        return_to_insert = true;
    }
    // Delete from buffer
    else if (op == CommandOperation::Delete)
    {
        // Delete
        auto cmd = std::make_shared<ZepCommand_DeleteRange>(buffer, startOffset, endOffset, GetCurrentWindow()->GetBufferCursor());
        AddCommand(std::static_pointer_cast<ZepCommand>(cmd));
        return_to_insert = true;
    }

    if (return_to_insert)
    {
        SwitchMode(EditorMode::Insert);
    }

    if (m_currentMode == EditorMode::Visual)
    {
        buffer.SetSelection(BufferRange{m_visualBegin, m_visualEnd});
    }
}
*/


} // namespace Zep
