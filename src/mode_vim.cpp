#include <cctype>
#include <sstream>

#include "zep/keymap.h"
#include "zep/mode_search.h"
#include "zep/mode_vim.h"
#include "zep/tab_window.h"
#include "zep/theme.h"

#include "zep/mcommon/animation/timer.h"
#include "zep/mcommon/logger.h"
#include "zep/mcommon/string/stringutils.h"

// Note:
// This is a very basic implementation of the common Vim commands that I use: the bare minimum I can live with.
// I do use more, and depending on how much pain I suffer, will add them over time.
// My aim is to make it easy to add commands, so if you want to put something in, please send me a PR.
// The buffer/display search and find support makes it easy to gather the info you need, and the basic insert/delete undo redo commands
// make it easy to find the locations in the buffer
// Important to note: I'm not trying to beat/better Vim here.  Just make an editor I can use in a viewport without feeling pain!
// See further down for what is implemented, and what's on my todo list

// IMPLEMENTED VIM:
// Command counts
// hjkl Motions
// . dot command
// TAB
// w,W,e,E,ge,gE,b,B WORD motions
// u,CTRL+r  Undo, Redo
// i,I,a,A Insert mode (pending undo/redo fix)
// DELETE/BACKSPACE in insert and normal mode; match vim
// Command status bar
// Arrow keys
// '$'
// 'jk' to insert mode
// gg Jump to end
// G Jump to beginning
// CTRL+F/B/D/U page and have page moves
// 'J' join
// D
// dd,d$,x  Delete line, to end of line, chars
// 'v' + 'x'/'d'
// 'y'
// 'p'/'P'
// a-z&a-Z, 0->9, _ " registers
// r Replace with char
// '$'
// 'yy'
// cc
// c$  Change to end of line
// C
// S, s, with visual modes
// ^
// 'O', 'o'
// 'V' (linewise v)
// Y, D, linewise yank/paste
// d[a]<count>w/e  Delete words
// di[({})]"'
// c[a]<count>w/e  Change word
// ci[({})]"'
// ct[char]/dt[char] Change to and delete to
// vi[Ww], va[Ww] Visual inner and word selections
// f[char] find on line
// /[string] find in file, 'n' find next

namespace Zep
{

ZepMode_Vim::ZepMode_Vim(ZepEditor& editor)
    : ZepMode(editor)
{
    Init();
}

ZepMode_Vim::~ZepMode_Vim()
{
}

void ZepMode_Vim::Init()
{
    timer_restart(m_insertEscapeTimer);
    for (int i = 0; i <= 9; i++)
    {
        GetEditor().SetRegister('0' + (const char)i, "");
    }
    GetEditor().SetRegister('"', "");

    // Setup keymaps
    // Later, we will be able to read these from a file.
    // But keymaps are useful for overriding behavior in modes too

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
    keymap_add({ &m_normalMap, &m_visualMap, &m_exMap, &m_insertMap }, { "<Escape>" }, id_NormalMode);

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
    keymap_add({ &m_normalMap }, { "<C-r>" }, id_Redo);
    keymap_add({ &m_normalMap }, { "<C-z>", "u" }, id_Undo);

    keymap_add({ &m_normalMap, &m_visualMap }, { ":", "/", "?" }, id_ExMode);

    /* Standard mode
    keymap_add({ &m_normalMap }, { "<C-y>"}, id_Redo);
    */

    // Insert Mode
    keymap_add({ &m_insertMap }, { "<Backspace>" }, id_Backspace);
    keymap_add({ &m_insertMap }, { "jk" }, id_NormalMode);

    // Ex Mode
    keymap_add({ &m_exMap }, { "<Backspace>" }, id_ExBackspace);
}

void ZepMode_Vim::Begin()
{
    if (GetCurrentWindow())
    {
        GetCurrentWindow()->SetCursorType(CursorType::Normal);
        GetEditor().SetCommandText(m_currentCommand);
    }
    m_currentMode = EditorMode::Normal;
    m_currentCommand.clear();
    m_lastCommand.clear();
    m_lastCount = 0;
    m_pendingEscape = false;
}

std::shared_ptr<CommandContext> ZepMode_Vim::AddKeyPress(uint32_t key, uint32_t modifierKeys)
{
    auto spContext = ZepMode::AddKeyPress(key, modifierKeys);
    if (!spContext)
    {
        return nullptr;
    }

    // Did we find something to do?
    if (spContext->foundCommand)
    {
        // It's an undoable command  - add it
        // Note: a command here is something that modifies the text, so it is kind of like an insert
        if (spContext->commandResult.spCommand)
        {
            if (m_currentMode != EditorMode::Insert)
            {
                AddCommand(std::make_shared<ZepCommand_BeginGroup>(spContext->buffer));
            }

            // Do the command
            AddCommand(spContext->commandResult.spCommand);
        }

        // If the command can't manage the count, we do it
        // Maybe all commands should handle the count?  What are the implications of that?  This bit is a bit messy
        if (!(spContext->commandResult.flags & CommandResultFlags::HandledCount))
        {
            for (int i = 1; i < spContext->count; i++)
            {
                // May immediate execute and not return a command...
                // Create a new 'inner' spContext-> for the next command, because we need to re-initialize the command
                // spContext-> for 'after' what just happened!
                CommandContext contextInner(m_currentCommand, *this, key, modifierKeys, m_currentMode);
                if (GetCommand(contextInner) && contextInner.commandResult.spCommand)
                {
                    // Actually queue/do command
                    AddCommand(contextInner.commandResult.spCommand);
                }
            }
        }
      
        if (spContext->commandResult.spCommand)
        {
            // Back from insert will mean we end the undo group
            if (spContext->commandResult.modeSwitch != EditorMode::Insert &&
                spContext->commandResult.modeSwitch != EditorMode::None)
            {
                AddCommand(std::make_shared<ZepCommand_EndGroup>(spContext->buffer));
            }
        }

        // A mode to switch to after the command is done
        SwitchMode(spContext->commandResult.modeSwitch);

        // If not in ex mode, wait for a new command
        // Can this be cleaner?
        if (m_currentMode != EditorMode::Ex)
        {
            ResetCommand();
        }

        // Motions can update the visual selection
        UpdateVisualSelection();
    }

    ClampCursorForMode();

    return spContext;
}

void ZepMode_Vim::HandleInsert(uint32_t key)
{
    key = 5;
    /*
    auto bufferCursor = GetCurrentWindow()->GetBufferCursor();

    // Operations outside of inserts will pack up the insert operation
    // and start a new one
    bool packCommand = false;
    switch (key)
    {
    case ExtKeys::ESCAPE:
    case ExtKeys::BACKSPACE:
    case ExtKeys::DEL:
    case ExtKeys::RIGHT:
    case ExtKeys::LEFT:
    case ExtKeys::UP:
    case ExtKeys::DOWN:
    case ExtKeys::PAGEUP:
    case ExtKeys::PAGEDOWN:
        packCommand = true;
        break;
    default:
        break;
    }

    if (m_pendingEscape)
    {
        // My custom 'jk' escape option
        auto canEscape = timer_get_elapsed_seconds(m_insertEscapeTimer) < .25f;
        if (canEscape && key == 'k')
        {
            packCommand = true;
            key = ExtKeys::ESCAPE;
        }
        m_pendingEscape = false;
    }

    auto& buffer = GetCurrentWindow()->GetBuffer();

    // Escape back to normal mode
    if (packCommand)
    {
        // End location is where we just finished typing
        auto insertEnd = bufferCursor;
        if (insertEnd > m_insertBegin)
        {
            // Get the string we inserted
            auto strInserted = std::string(buffer.GetText().begin() + m_insertBegin, buffer.GetText().begin() + insertEnd);

            // Remember the inserted string for repeating the command
            m_lastInsertString = strInserted;

            auto lineBegin = buffer.GetLinePos(bufferCursor, LineLocation::LineBegin);

            // Temporarily remove it
            buffer.Delete(m_insertBegin, insertEnd);

            // Generate a command to put it in with undoable state
            // Leave cusor at the end of the insert, on the last inserted char.
            // ...but clamp to the line begin, so that a RETURN + ESCAPE lands you at the beginning of the new line
            auto cursorAfterEscape = std::max(lineBegin, m_insertBegin + BufferLocation(strInserted.length() - 1));

            auto cmd = std::make_shared<ZepCommand_Insert>(buffer, m_insertBegin, strInserted, m_insertBegin, cursorAfterEscape);
            AddCommand(std::static_pointer_cast<ZepCommand>(cmd));
        }

        // Finished escaping
        if (key == ExtKeys::ESCAPE)
        {
            // Back to normal mode
            SwitchMode(EditorMode::Normal);
        }
        else
        {
            // Any other key here is a command while in insert mode
            // For example, hitting Backspace
            // There is more work to do here to support keyboard combos in insert mode
            // (not that I can think of ones that I use!)
            CommandContext context(std::string(1, char(key)), *this, key, 0, EditorMode::Insert);
            context.bufferCursor = bufferCursor;
            if (GetCommand(context) && context.commandResult.spCommand)
            {
                AddCommand(context.commandResult.spCommand);
            }
            SwitchMode(EditorMode::Insert);
        }
        return;
    }

    std::string ch(1, (char)key);
    if (key == ExtKeys::RETURN)
    {
        ch = "\n";
    }
    else if (key == ExtKeys::TAB)
    {
        // 4 Spaces, obviously :)
        ch = "    ";
    }

    if (key == 'j' && !m_pendingEscape)
    {
        timer_restart(m_insertEscapeTimer);
        m_pendingEscape = true;
    }
    else
    {
        // If we thought it was an escape but it wasn't, put the 'j' back in!
        if (m_pendingEscape)
        {
            ch = "j" + ch;
        }
        m_pendingEscape = false;

        buffer.Insert(bufferCursor, ch);

        // Insert back to normal mode should put the m_bufferCursor on top of the last character typed.
        GetCurrentWindow()->SetBufferCursor(bufferCursor + long(ch.size()));
    }
    */
}

void ZepMode_Vim::PreDisplay()
{

    // If we thought it was an escape but it wasn't, put the 'j' back in!
    // TODO: Move to a more sensible place where we can check the time
    if (m_pendingEscape && timer_get_elapsed_seconds(m_insertEscapeTimer) > .25f)
    {
        m_pendingEscape = false;
        GetCurrentWindow()->GetBuffer().Insert(GetCurrentWindow()->GetBufferCursor(), "j");
        GetCurrentWindow()->SetBufferCursor(GetCurrentWindow()->GetBufferCursor() + 1);
    }
}
} // namespace Zep

/*
            // Remember a new modification command and clear the last dot command string
            if (spContext->commandResult.spCommand && key != '.')
            {
                m_lastCommand = spContext->command;
                m_lastCount = spContext->count;
                m_lastInsertString.clear();
            }

            // Dot group means we have an extra command to append
            // This is to make a command and insert into a single undo operation
            bool appendDotInsert = false;

            // Label group beginning
            if (spContext->commandResult.spCommand)
            {
                if (key == '.' && !m_lastInsertString.empty() && spContext->commandResult.modeSwitch == EditorMode::Insert)
                {
                    appendDotInsert = true;
                }

                if (appendDotInsert || (spContext->count > 1 && !(spContext->commandResult.flags & CommandResultFlags::HandledCount)))
                {
                    spContext->commandResult.spCommand->SetFlags(CommandFlags::GroupBoundary);
                }
                AddCommand(spContext->commandResult.spCommand);
            }

            // Next commands (for counts)
            // Many command handlers do the right thing for counts; if they don't we basically interpret the command
            // multiple times to implement it.
            if (!(spContext->commandResult.flags & CommandResultFlags::HandledCount))
            {
                for (int i = 1; i < spContext->count; i++)
                {
                    // May immediate execute and not return a command...
                    // Create a new 'inner' spContext-> for the next command, because we need to re-initialize the command
                    // spContext-> for 'after' what just happened!
                    CommandContext contextInner(m_currentCommand, *this, key, modifierKeys, m_currentMode);
                    if (GetCommand(contextInner) && contextInner.commandResult.spCommand)
                    {
                        // Group counted
                        if (i == (spContext->count - 1) && !appendDotInsert)
                        {
                            contextInner.commandResult.spCommand->SetFlags(CommandFlags::GroupBoundary);
                        }

                        // Actually queue/do command
                        AddCommand(contextInner.commandResult.spCommand);
                    }
                }
            }

            ResetCommand();

            // A mode to switch to after the command is done
            SwitchMode(spContext->commandResult.modeSwitch);

            // If used dot command, append the inserted text.  This is a little confusing.
            // TODO: Think of a cleaner way to express it
            if (appendDotInsert)
            {
                if (!m_lastInsertString.empty())
                {
                    auto cmd = std::make_shared<ZepCommand_Insert>(
                        GetCurrentWindow()->GetBuffer(),
                        GetCurrentWindow()->GetBufferCursor(),
                        m_lastInsertString,
                        spContext->bufferCursor);
                    cmd->SetFlags(CommandFlags::GroupBoundary);
                    AddCommand(std::static_pointer_cast<ZepCommand>(cmd));
                }
                SwitchMode(EditorMode::Normal);
            }

            // Any motions while in Vim mode will update the selection
            UpdateVisualSelection();
        }
        else
        {
            if (m_currentMode == EditorMode::Insert)
            {
                HandleInsert(key);
                ResetCommand();
            }
            else
            {
                // If the user has so far typed numbers, then wait for more input
                if (!m_currentCommand.empty() && m_currentCommand.find_first_not_of("0123456789") == std::string::npos)
                {
                    spContext->commandResult.flags |= CommandResultFlags::NeedMoreChars;
                }
                // Handled, but no new command

                // A better mechanism is required for clearing pending commands!
                if (m_currentCommand[0] != ':' && m_currentCommand[0] != '"' && !(spContext->commandResult.flags & CommandResultFlags::NeedMoreChars))
                {
                    ResetCommand();
                }
            }
        }
    }
    */
