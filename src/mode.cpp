#include "zep/mode.h"
#include "zep/editor.h"
#include "zep/filesystem.h"
#include "zep/mode_search.h"
#include "zep/tab_window.h"

#include "zep/mcommon/logger.h"

namespace Zep
{
CommandContext::CommandContext(const std::string& commandIn, ZepMode& md, EditorMode editorMode)
    : owner(md)
    , fullCommand(commandIn)
    , buffer(md.GetCurrentWindow()->GetBuffer())
    , bufferCursor(md.GetCurrentWindow()->GetBufferCursor())
    , tempReg("", false)
    , currentMode(editorMode)
{
    registers.push('"');
    pRegister = &tempReg;

    GetCommandAndCount();
    GetCommandRegisters();
}

void CommandContext::UpdateRegisters()
{
    // Store in a register
    if (registers.empty())
        return;

    if (op == CommandOperation::Delete || op == CommandOperation::DeleteLines)
    {
        beginRange = std::max(beginRange, BufferLocation{ 0 });
        endRange = std::max(endRange, BufferLocation{ 0 });
        if (beginRange > endRange)
        {
            std::swap(beginRange, endRange);
        }

        std::string str = std::string(buffer.GetText().begin() + beginRange, buffer.GetText().begin() + endRange);

        // Delete commands fill up 1-9 registers
        if (command[0] == 'd' || command[0] == 'D')
        {
            for (int i = 9; i > 1; i--)
            {
                owner.GetEditor().SetRegister('0' + (char)i, owner.GetEditor().GetRegister('0' + (char)i - 1));
            }
            owner.GetEditor().SetRegister('1', Register(str, (op == CommandOperation::DeleteLines)));
        }

        // Fill up any other required registers
        while (!registers.empty())
        {
            owner.GetEditor().SetRegister(registers.top(), Register(str, (op == CommandOperation::DeleteLines)));
            registers.pop();
        }
    }
    else if (op == CommandOperation::Copy || op == CommandOperation::CopyLines)
    {
        beginRange = std::max(beginRange, BufferLocation{ 0 });
        endRange = std::max(endRange, BufferLocation{ 0 });
        if (beginRange > endRange)
        {
            std::swap(beginRange, endRange);
        }

        std::string str = std::string(buffer.GetText().begin() + beginRange, buffer.GetText().begin() + endRange);
        while (!registers.empty())
        {
            auto& ed = owner.GetEditor();

            // Capital letters append to registers instead of replacing them
            if (registers.top() >= 'A' && registers.top() <= 'Z')
            {
                auto chlow = (char)std::tolower(registers.top());
                ed.SetRegister(chlow, Register(ed.GetRegister(chlow).text + str, (op == CommandOperation::CopyLines)));
            }
            else
            {
                ed.SetRegister(registers.top(), Register(str, op == CommandOperation::CopyLines));
            }
            registers.pop();
        }
    }
}

// Parse the command into:
// [count1] opA [count2] opB
// And generate (count1 * count2), opAopB
void CommandContext::GetCommandAndCount()
{
    std::string count1;
    std::string command1;
    std::string count2;
    std::string command2;

    auto itr = fullCommand.begin();
    while (itr != fullCommand.end() && std::isdigit((unsigned char)*itr))
    {
        count1 += *itr;
        itr++;
    }

    // Special case; 'f3' is a find for the character '3', not a count of 3!
    // But 2f3 is 'find the second 3'....
    // Same thing for 'r'
    // I need a key mapper with input patterns to sort this out properly
    if (itr != fullCommand.end())
    {
        if (*itr == 'f' || *itr == 'F' || *itr == 'c' || *itr == 'r')
        {
            while (itr != fullCommand.end()
                && std::isgraph(ToASCII(*itr)))
            {
                command1 += *itr;
                itr++;
            }
        }
        else
        {
            while (itr != fullCommand.end() && !std::isdigit(ToASCII(*itr)))
            {
                command1 += *itr;
                itr++;
            }
        }
    }

    // If not a register target, then another count
    if (command1[0] != '\"' && command1[0] != ':' && command1[0] != '/' && command1[0] != '?')
    {
        while (itr != fullCommand.end()
            && std::isdigit(ToASCII(*itr)))
        {
            count2 += *itr;
            itr++;
        }
    }

    // No more counts, pull out the rest of the command
    while (itr != fullCommand.end())
    {
        command2 += *itr;
        itr++;
    }

    foundCount = false;
    count = 1;

    try
    {
        if (!count1.empty())
        {
            count = std::stoi(count1);
            foundCount = true;
        }
        if (!count2.empty())
        {
            // When 2 counts are specified, they multiply!
            // Such as 2d2d, which deletes 4 lines
            count *= std::stoi(count2);
            foundCount = true;
        }
    }
    catch (std::out_of_range&)
    {
        // Ignore bad count
    }

    // Concatentate the parts of the command into a single command string
    commandWithoutCount = command1 + command2;

    // Short circuit - 0 is special, first on line.  Since we don't want to confuse it
    // with a command count
    if (count == 0)
    {
        count = 1;
        commandWithoutCount = "0";
    }
}

void CommandContext::GetCommandRegisters()
{
    // Store the register source
    if (commandWithoutCount[0] == '"' && commandWithoutCount.size() > 2)
    {
        // Null register
        if (commandWithoutCount[1] == '_')
        {
            std::stack<char> temp;
            registers.swap(temp);
        }
        else
        {
            registers.push(commandWithoutCount[1]);
            char reg = commandWithoutCount[1];

            // Demote capitals to lower registers when pasting (all both)
            if (reg >= 'A' && reg <= 'Z')
            {
                reg = (char)std::tolower((char)reg);
            }

            if (owner.GetEditor().GetRegisters().find(std::string({ reg })) != owner.GetEditor().GetRegisters().end())
            {
                pRegister = &owner.GetEditor().GetRegister(reg);
            }
        }
        command = commandWithoutCount.substr(2);
    }
    else
    {
        // Default register
        if (pRegister->text.empty())
        {
            pRegister = &owner.GetEditor().GetRegister('"');
        }
        command = commandWithoutCount;
    }
}

ZepMode::ZepMode(ZepEditor& editor)
    : ZepComponent(editor)
{
}

ZepMode::~ZepMode()
{
}

ZepWindow* ZepMode::GetCurrentWindow() const
{
    if (GetEditor().GetActiveTabWindow())
    {
        return GetEditor().GetActiveTabWindow()->GetActiveWindow();
    }
    return nullptr;
}

EditorMode ZepMode::GetEditorMode() const
{
    return m_currentMode;
}

void ZepMode::SetEditorMode(EditorMode currentMode)
{
    m_currentMode = currentMode;
}

void ZepMode::AddCommandText(std::string strText)
{
    for (auto& ch : strText)
    {
        AddKeyPress(ch);
    }
}

void ZepMode::ClampCursorForMode()
{
    // Normal mode cursor is never on a CR/0
    if (m_currentMode == EditorMode::Normal)
    {
        GetCurrentWindow()->SetBufferCursor(GetCurrentWindow()->GetBuffer().ClampToVisibleLine(GetCurrentWindow()->GetBufferCursor()));
    }
}

// TODO: This happens every time and doesn't guard against repeats. And some logic requires that.
// Should really just switch if not the same.  The new command setup should make this easier to fix
void ZepMode::SwitchMode(EditorMode currentMode)
{
    // Don't switch to invalid mode
    if (currentMode == EditorMode::None)
        return;

    if (currentMode == m_currentMode)
        return;

    auto pWindow = GetCurrentWindow();
    auto& buffer = pWindow->GetBuffer();
    auto cursor = pWindow->GetBufferCursor();

    if (currentMode == EditorMode::Insert && buffer.TestFlags(FileFlags::ReadOnly))
    {
        currentMode = EditorMode::Normal;
    }

    // When leaving Ex mode, reset search markers
    if (m_currentMode == EditorMode::Ex)
    {
        buffer.HideMarkers(RangeMarkerType::Search);

        // Bailed out of ex mode; reset the start location
        /*if (mode != EditorMode::Ex)
        {
            pWindow->SetBufferCursor(m_exCommandStartLocation);
        }
        */
    }
    else if (m_currentMode == EditorMode::Insert)
    {
        // When switching back to normal mode, put the cursor on the last character typed
        if (currentMode == EditorMode::Normal)
        {
            // Move back, but not to the previous line
            auto lineStart = buffer.GetLinePos(cursor, LineLocation::LineBegin);
            GetCurrentWindow()->SetBufferCursor(std::max(cursor - 1, lineStart));
        }
    }

    m_currentMode = currentMode;

    switch (currentMode)
    {
    case EditorMode::Normal:
    {
        pWindow->SetCursorType(CursorType::Normal);
        buffer.ClearSelection();
        ClampCursorForMode();
        ResetCommand();
    }
    break;
    case EditorMode::Insert:
        pWindow->SetCursorType(CursorType::Insert);
        buffer.ClearSelection();
        ResetCommand();
        m_pendingEscape = false;
        break;
    case EditorMode::Visual:
    {
        pWindow->SetCursorType(m_visualCursorType);
        ResetCommand();
        m_pendingEscape = false;
    }
    break;
    case EditorMode::Ex:
    {
        m_exCommandStartLocation = cursor;
        pWindow->SetCursorType(CursorType::Hidden);
        m_pendingEscape = false;
    }
    break;
    default:
    case EditorMode::None:
        break;
    }
}

std::string ZepMode::ConvertInputToMapString(uint32_t key, uint32_t modifierKeys)
{
    std::ostringstream str;
    bool closeBracket = false;
    if (modifierKeys & ModifierKey::Ctrl)
    {
        str << "<C-";
        if (modifierKeys & ModifierKey::Shift)
        {
            // Add the S- modifier for shift enabled special keys
            // We want to avoid adding S- to capitalized (and already shifted)
            // keys
            if (key < ' ')
            {
                str << "S-";
            }
        }
        closeBracket = true;
    }
    else if (modifierKeys & ModifierKey::Shift)
    {
        if (key < ' ')
        {
            str << "<S-";
            closeBracket = true;
        }
    }

    str << char(key);
    if (closeBracket)
    {
        str << ">";
    }
    return str.str();
}

// Handle a key press, convert it to an input command and context, and return it.
void ZepMode::AddKeyPress(uint32_t key, uint32_t modifierKeys)
{
    if (!GetCurrentWindow())
    {
        return;
    }

    // Get the new command by parsing out the keys
    // We convert CTRL + f to a string: "<C-f>"
    HandleMappedInput(ConvertInputToMapString(key, modifierKeys));
}

void ZepMode::HandleMappedInput(const std::string& input)
{
    if (input.empty())
    {
        return;
    }

    // Special case, dot command (do last edit again)
    // Dot command is complicated, this is my second attempt at implementing it and is less
    // complex.  The approach is to store relevent key strokes for the last edit operation,
    // and replay them when the user uses the dot.
    if (m_currentMode == EditorMode::Normal && input[input.size() - 1] == '.')
    {
        // Save and restore the last command while doing it.
        auto lastCommand = m_dotCommand;
        for (auto& last : lastCommand)
        {
            HandleMappedInput(std::string(1, last));
        }
        m_dotCommand = lastCommand;

        SwitchMode(EditorMode::Normal);

        return;
    }

    // The current command is our currently typed multi-key operation
    m_currentCommand += input;

    // Reset the timer for the last edit, for time-sensitive key strokes
    GetEditor().ResetLastEditTimer();

    // Reset the cursor to keep it visible during typing, and not flashing
    GetEditor().ResetCursorTimer();

    // Reset command text - it may get updated later.
    GetEditor().SetCommandText("");

    // Ex mode, or show chars mode, we display the text typed
    if (m_currentMode == EditorMode::Ex || m_settings.ShowNormalModeKeyStrokes)
    {
        GetEditor().SetCommandText(m_currentCommand);
    }

    // Figure out the command we have typed. foundCommand means that the command was interpreted and understood.
    // If spCommand is returned, then there is an atomic command operation that needs to be done.
    auto spContext = std::make_shared<CommandContext>(m_currentCommand, *this, m_currentMode);
    spContext->foundCommand = GetCommand(*spContext);

    // A lambda to check for a pending mode switch after the command
    auto enteringMode = [=](auto mode) {
        if (m_currentMode != spContext->commandResult.modeSwitch && spContext->commandResult.modeSwitch == mode)
        {
            return true;
        }
        return false;
    };

    // Escape Nukes the current command - we handle it in the keyboard mappings after that
    // TODO: This feels awkward
    if (input[input.size() - 1] == ExtKeys::ESCAPE)
    {
        m_currentCommand.clear();
    }

    // Did we find something to do?
    if (spContext->foundCommand)
    {
        // It's an undoable command  - add it
        // Note: a command here is something that modifies the text.  It can be something like a delete
        // or a simple insert
        if (spContext->commandResult.spCommand)
        {
            // If not in insert mode, begin the group, because we have started a new operation
            if (m_currentMode != EditorMode::Insert || spContext->commandResult.beginUndoGroup)
            {
                AddCommand(std::make_shared<ZepCommand_GroupMarker>(spContext->buffer));

                // Record for the dot command
                m_dotCommand = m_currentCommand;
            }
            else
            {
                // In insert mode keep the text for the dot command.  An insert adds a command too!
                m_dotCommand += input;
            }

            // Do the command
            AddCommand(spContext->commandResult.spCommand);
        }
        else
        {
            // This command didn't change anything, but switched into insert mode, so
            // remember the dot command that did it
            if (enteringMode(EditorMode::Insert))
            {
                AddCommand(std::make_shared<ZepCommand_GroupMarker>(spContext->buffer));
                m_dotCommand = m_currentCommand;
            }
        }

        // If the command can't manage the count, we do it
        // Maybe all commands should handle the count?  What are the implications of that?  This bit is a bit messy
        if (!(spContext->commandResult.flags & CommandResultFlags::HandledCount))
        {
            // Ignore count == 1, we already did it
            for (int i = 1; i < spContext->count; i++)
            {
                // May immediate execute and not return a command...
                // Create a new 'inner' spContext-> for the next command, because we need to re-initialize the command
                // spContext-> for 'after' what just happened!
                CommandContext contextInner(m_currentCommand, *this, m_currentMode);
                if (GetCommand(contextInner) && contextInner.commandResult.spCommand)
                {
                    // Actually queue/do command
                    AddCommand(contextInner.commandResult.spCommand);
                }
            }
        }

        // Back from insert will mean we end the undo group
        /*
        if (enteringMode(EditorMode::Normal))
        {
            AddCommand(std::make_shared<ZepCommand_EndGroup>(spContext->buffer));
        }
        */

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
    else
    {
        // If not found, and there was no request for more characters, and we aren't in Ex mode
        // Reset the current command
        // TODO: Clean up these 'is command finished or not' complications.
        // Perhaps by having a regex for every command: <?count>d<?count>d.  Otherwise there is no easy way to ensure
        // a command is not 'finished'.  The user can always escape/return of course, but it's neater if invalid
        // commands are ignored, as we mostly do.  To add a regex, the keymapper would have to build a tree and 'capture'
        // relevent information, for example for 'f<?graph>' you would capture the char to find
        // In this way, arriving at a tree leaf without a command would easily tell us that we had failed to match a command
        if ((spContext->commandResult.flags & CommandResultFlags::NeedMoreChars) == 0)
        {
            if (m_currentMode != EditorMode::Ex && spContext->command[0] != '"')
            {
                ResetCommand();
            }
        }
    }
    ClampCursorForMode();
}

void ZepMode::AddCommand(std::shared_ptr<ZepCommand> spCmd)
{
    if (GetCurrentWindow() && GetCurrentWindow()->GetBuffer().TestFlags(FileFlags::Locked))
    {
        // Ignore commands on buffers because we are view only,
        // and all commands currently modify the buffer!
        return;
    }

    spCmd->Redo();
    m_undoStack.push(spCmd);

    // Can't redo anything beyond this point
    std::stack<std::shared_ptr<ZepCommand>> empty;
    m_redoStack.swap(empty);

    if (spCmd->GetCursorAfter() != -1)
    {
        GetCurrentWindow()->SetBufferCursor(spCmd->GetCursorAfter());
    }
}

void ZepMode::Redo()
{
    if (m_redoStack.empty())
        return;

    if (std::dynamic_pointer_cast<ZepCommand_GroupMarker>(m_redoStack.top()) != nullptr)
    {
        m_undoStack.push(m_redoStack.top());
        m_redoStack.pop();
    }

    while (!m_redoStack.empty())
    {
        auto& spCommand = m_redoStack.top();
        spCommand->Redo();

        if (spCommand->GetCursorAfter() != -1)
        {
            GetCurrentWindow()->SetBufferCursor(spCommand->GetCursorAfter());
        }

        m_undoStack.push(spCommand);
        m_redoStack.pop();

        if (std::dynamic_pointer_cast<ZepCommand_GroupMarker>(spCommand) != nullptr)
        {
            break;
        }
    };
}

void ZepMode::Undo()
{
    if (m_undoStack.empty())
        return;

    if (std::dynamic_pointer_cast<ZepCommand_GroupMarker>(m_undoStack.top()) != nullptr)
    {
        m_redoStack.push(m_undoStack.top());
        m_undoStack.pop();
    }

    while (!m_undoStack.empty())
    {
        auto& spCommand = m_undoStack.top();
        spCommand->Undo();

        if (spCommand->GetCursorBefore() != -1)
        {
            GetCurrentWindow()->SetBufferCursor(spCommand->GetCursorBefore());
        }

        m_redoStack.push(spCommand);
        m_undoStack.pop();

        if (std::dynamic_pointer_cast<ZepCommand_GroupMarker>(spCommand) != nullptr)
        {
            break;
        }
    };
}

NVec2i ZepMode::GetNormalizedVisualRange() const
{
    auto& buffer = GetCurrentWindow()->GetBuffer();
    // Clamp and orient the correct way around
    auto startOffset = buffer.Clamp(m_visualBegin);
    auto endOffset = buffer.Clamp(m_visualEnd);
    if (startOffset > endOffset)
    {
        std::swap(startOffset, endOffset);
    }
    return NVec2i(startOffset, endOffset);
}

const std::string& ZepMode::GetLastCommand() const
{
    return m_dotCommand;
}

bool ZepMode::GetCommand(CommandContext& context)
{
    auto bufferCursor = GetCurrentWindow()->GetBufferCursor();
    auto& buffer = GetCurrentWindow()->GetBuffer();

    bool needMoreChars = false;

    uint32_t mappedCommand = 0;
    if (m_currentMode == EditorMode::Visual)
    {
        mappedCommand = keymap_find(m_visualMap, context.command, needMoreChars);
    }
    else if (m_currentMode == EditorMode::Normal)
    {
        mappedCommand = keymap_find(m_normalMap, context.command, needMoreChars);
    }
    else if (m_currentMode == EditorMode::Insert)
    {
        mappedCommand = keymap_find(m_insertMap, context.command, needMoreChars);
    }
    else if (m_currentMode == EditorMode::Ex)
    {
        // TODO: Is it possible extend our key mapping to better process ex commands?  Or are these
        // too specialized?
        if (HandleExCommand(context.command))
        {
            SwitchMode(EditorMode::Normal);
            ResetCommand();
            return true;
        }
        return false;
    }

    // Didn't find an immediate match, found a count and there is no other part to the command
    if (context.currentMode == EditorMode::Normal && mappedCommand == 0 && context.foundCount && context.command.empty())
    {
        needMoreChars = true;
    }

    // Found a valid command, but there are more options to come
    if (needMoreChars)
    {
        context.commandResult.flags |= CommandResultFlags::NeedMoreChars;
        return false;
    }

    if (mappedCommand == id_NormalMode)
    {
        // TODO: I think this should be a 'command' which would get replayed with dot;
        // instead of special casing it later, we could just insert it into the stream of commands
        context.commandResult.modeSwitch = EditorMode::Normal;
        return true;
    }
    else if (mappedCommand == id_ExMode)
    {
        context.commandResult.modeSwitch = EditorMode::Ex;
        return true;
    }
    // Control
    else if (mappedCommand == id_MotionNextMarker)
    {
        auto pFound = buffer.FindNextMarker(GetCurrentWindow()->GetBufferCursor(), SearchDirection::Forward, RangeMarkerType::Message);
        if (pFound)
        {
            GetCurrentWindow()->SetBufferCursor(pFound->range.first);
        }
        return true;
    }
    else if (mappedCommand == id_MotionPreviousMarker)
    {
        auto pFound = buffer.FindNextMarker(GetCurrentWindow()->GetBufferCursor(), SearchDirection::Backward, RangeMarkerType::Message);
        if (pFound)
        {
            GetCurrentWindow()->SetBufferCursor(pFound->range.first);
        }
        return true;
    }
    else if (mappedCommand == id_SwitchToAlternateFile)
    {
        // This is a quick and easy 'alternate file swap'.
        // It searches a preset list of useful folder targets around the current file.
        // A better alternative might be a wildcard list of relations, but this works well enough
        // It also only looks for a file with the same name and different extension!
        // it is good enough for my current needs...
        auto path = buffer.GetFilePath();
        if (!path.empty() && GetEditor().GetFileSystem().Exists(path))
        {
            auto ext = path.extension();
            auto searchPaths = std::vector<ZepPath>{
                path.parent_path(),
                path.parent_path().parent_path(),
                path.parent_path().parent_path().parent_path()
            };

            auto ignoreFolders = std::vector<std::string>{ "build", ".git", "obj", "debug", "release" };

            auto priorityFolders = std::vector<std::string>{ "source", "include", "src", "inc", "lib" };

            // Search the paths, starting near and widening
            for (auto& p : searchPaths)
            {
                if (p.empty())
                    continue;

                bool found = false;

                // Look for the priority folder locations
                std::vector<ZepPath> searchFolders{ path.parent_path() };
                for (auto& priorityFolder : priorityFolders)
                {
                    GetEditor().GetFileSystem().ScanDirectory(p, [&](const ZepPath& currentPath, bool& recurse) {
                        recurse = false;
                        if (GetEditor().GetFileSystem().IsDirectory(currentPath))
                        {
                            auto lower = string_tolower(currentPath.filename().string());
                            if (std::find(ignoreFolders.begin(), ignoreFolders.end(), lower) != ignoreFolders.end())
                            {
                                return true;
                            }

                            if (priorityFolder == lower)
                            {
                                searchFolders.push_back(currentPath);
                            }
                        }
                        return true;
                    });
                }

                for (auto& folder : searchFolders)
                {
                    LOG(INFO) << "Searching: " << folder.string();

                    GetEditor().GetFileSystem().ScanDirectory(folder, [&](const ZepPath& currentPath, bool& recurse) {
                        recurse = true;
                        if (path.stem() == currentPath.stem() && !(currentPath.extension() == path.extension()))
                        {
                            auto load = GetEditor().GetFileBuffer(currentPath, 0, true);
                            if (load != nullptr)
                            {
                                GetCurrentWindow()->SetBuffer(load);
                                found = true;
                                return false;
                            }
                        }
                        return true;
                    });
                    if (found)
                        return true;
                }
            }
        }
    }
    else if (mappedCommand == id_FontBigger)
    {
        GetEditor().GetDisplay().SetFontPointSize(std::min(GetEditor().GetDisplay().GetFontPointSize() + 1.0f, 20.0f));
        return true;
    }
    else if (mappedCommand == id_FontSmaller)
    {
        GetEditor().GetDisplay().SetFontPointSize(std::max(10.0f, GetEditor().GetDisplay().GetFontPointSize() - 1.0f));
        return true;
    }
    // Moving between splits
    else if (mappedCommand == id_MotionDownSplit)
    {
        GetCurrentWindow()->GetTabWindow().DoMotion(WindowMotion::Down);
        return true;
    }
    else if (mappedCommand == id_MotionUpSplit)
    {
        GetCurrentWindow()->GetTabWindow().DoMotion(WindowMotion::Up);
        return true;
    }
    else if (mappedCommand == id_MotionLeftSplit)
    {
        GetCurrentWindow()->GetTabWindow().DoMotion(WindowMotion::Left);
        return true;
    }
    else if (mappedCommand == id_MotionRightSplit)
    {
        GetCurrentWindow()->GetTabWindow().DoMotion(WindowMotion::Right);
        return true;
    }
    // global search
    else if (mappedCommand == id_QuickSearch)
    {
        GetEditor().AddSearch();
        return true;
    }
    else if (mappedCommand == id_Redo)
    {
        Redo();
        return true;
    }
    else if (mappedCommand == id_Undo)
    {
        Undo();
        return true;
    }
    else if (mappedCommand == id_MotionLineEnd)
    {
        GetCurrentWindow()->SetBufferCursor(context.buffer.GetLinePos(bufferCursor, LineLocation::LineLastNonCR));
        return true;
    }
    else if (mappedCommand == id_MotionLineBegin)
    {
        GetCurrentWindow()->SetBufferCursor(context.buffer.GetLinePos(bufferCursor, LineLocation::LineBegin));
        return true;
    }
    else if (mappedCommand == id_MotionLineFirstChar)
    {
        GetCurrentWindow()->SetBufferCursor(context.buffer.GetLinePos(bufferCursor, LineLocation::LineFirstGraphChar));
        return true;
    }
    // Moving between tabs
    else if (mappedCommand == id_PreviousTabWindow)
    {
        GetEditor().PreviousTabWindow();
        return true;
    }
    else if (mappedCommand == id_NextTabWindow)
    {
        GetEditor().NextTabWindow();
        return true;
    }
    else if (mappedCommand == id_MotionDown)
    {
        GetCurrentWindow()->MoveCursorY(context.count);
        context.commandResult.flags |= CommandResultFlags::HandledCount;
        return true;
    }
    else if (mappedCommand == id_MotionUp)
    {
        GetCurrentWindow()->MoveCursorY(-context.count);
        context.commandResult.flags |= CommandResultFlags::HandledCount;
        return true;
    }
    else if (mappedCommand == id_MotionRight)
    {
        auto lineEnd = context.buffer.GetLinePos(context.bufferCursor, LineLocation::LineLastNonCR);
        GetCurrentWindow()->SetBufferCursor(std::min(context.bufferCursor + context.count, lineEnd));
        context.commandResult.flags |= CommandResultFlags::HandledCount;
        return true;
    }
    else if (mappedCommand == id_MotionLeft)
    {
        auto lineStart = context.buffer.GetLinePos(context.bufferCursor, LineLocation::LineBegin);
        GetCurrentWindow()->SetBufferCursor(std::max(context.bufferCursor - context.count, lineStart));
        context.commandResult.flags |= CommandResultFlags::HandledCount;
        return true;
    }
    else if (mappedCommand == id_MotionStandardRight)
    {
        GetCurrentWindow()->SetBufferCursor(bufferCursor + 1);
        return true;
    }
    else if (mappedCommand == id_MotionStandardLeft)
    {
        GetCurrentWindow()->SetBufferCursor(bufferCursor - 1);
        return true;
    }
    else if (mappedCommand == id_MotionStandardUp)
    {
        GetCurrentWindow()->MoveCursorY(-1, LineLocation::LineCRBegin);
        return true;
    }
    else if (mappedCommand == id_MotionStandardDown)
    {
        GetCurrentWindow()->MoveCursorY(1, LineLocation::LineCRBegin);
        return true;
    }
    else if (mappedCommand == id_MotionStandardRightSelect)
    {
        context.commandResult.modeSwitch = EditorMode::Visual;
        if (m_currentMode != EditorMode::Visual)
        {
            m_visualBegin = GetCurrentWindow()->GetBufferCursor();
        }
        GetCurrentWindow()->SetBufferCursor(bufferCursor + 1);
        UpdateVisualSelection();
        return true;
    }
    else if (mappedCommand == id_MotionStandardLeftSelect)
    {
        context.commandResult.modeSwitch = EditorMode::Visual;
        if (m_currentMode != EditorMode::Visual)
        {
            m_visualBegin = GetCurrentWindow()->GetBufferCursor();
        }
        GetCurrentWindow()->SetBufferCursor(bufferCursor - 1);
        UpdateVisualSelection();
        return true;
    }
    else if (mappedCommand == id_MotionStandardUpSelect)
    {
        context.commandResult.modeSwitch = EditorMode::Visual;
        if (m_currentMode != EditorMode::Visual)
        {
            m_visualBegin = GetCurrentWindow()->GetBufferCursor();
        }
        GetCurrentWindow()->MoveCursorY(-1, LineLocation::LineCRBegin);
        UpdateVisualSelection();
        return true;
    }
    else if (mappedCommand == id_MotionStandardDownSelect)
    {
        context.commandResult.modeSwitch = EditorMode::Visual;
        if (m_currentMode != EditorMode::Visual)
        {
            m_visualBegin = GetCurrentWindow()->GetBufferCursor();
        }
        GetCurrentWindow()->MoveCursorY(1, LineLocation::LineCRBegin);
        UpdateVisualSelection();
        return true;
    }
    else if (mappedCommand == id_MotionStandardRightWord)
    {
        auto target = buffer.StandardCtrlMotion(bufferCursor, SearchDirection::Forward);
        GetCurrentWindow()->SetBufferCursor(target.second);
        return true;
    }
    else if (mappedCommand == id_MotionStandardLeftWord)
    {
        auto target = buffer.StandardCtrlMotion(bufferCursor, SearchDirection::Backward);
        GetCurrentWindow()->SetBufferCursor(target.second);
        return true;
    }
    else if (mappedCommand == id_MotionStandardRightWordSelect)
    {
        context.commandResult.modeSwitch = EditorMode::Visual;
        if (m_currentMode != EditorMode::Visual)
        {
            m_visualBegin = GetCurrentWindow()->GetBufferCursor();
        }
        auto target = buffer.StandardCtrlMotion(bufferCursor, SearchDirection::Forward);
        GetCurrentWindow()->SetBufferCursor(target.second);
        UpdateVisualSelection();
        return true;
    }
    else if (mappedCommand == id_MotionStandardLeftWordSelect)
    {
        context.commandResult.modeSwitch = EditorMode::Visual;
        if (m_currentMode != EditorMode::Visual)
        {
            m_visualBegin = GetCurrentWindow()->GetBufferCursor();
        }
        auto target = buffer.StandardCtrlMotion(bufferCursor, SearchDirection::Backward);
        GetCurrentWindow()->SetBufferCursor(target.second);
        UpdateVisualSelection();
        return true;
    }
    else if (mappedCommand == id_MotionPageForward)
    {
        // Note: the vim spec says 'visible lines - 2' for a 'page'.
        // We jump the max possible lines, which might hit the end of the text; this matches observed vim behavior
        GetCurrentWindow()->MoveCursorY((GetCurrentWindow()->GetMaxDisplayLines() - 2) * context.count);
        context.commandResult.flags |= CommandResultFlags::HandledCount;
        return true;
    }
    else if (mappedCommand == id_MotionHalfPageForward)
    {
        // Note: the vim spec says 'half visible lines' for up/down
        GetCurrentWindow()->MoveCursorY((GetCurrentWindow()->GetNumDisplayedLines() / 2) * context.count);
        context.commandResult.flags |= CommandResultFlags::HandledCount;
        return true;
    }
    else if (mappedCommand == id_MotionPageBackward)
    {
        // Note: the vim spec says 'visible lines - 2' for a 'page'
        GetCurrentWindow()->MoveCursorY(-(GetCurrentWindow()->GetMaxDisplayLines() - 2) * context.count);
        context.commandResult.flags |= CommandResultFlags::HandledCount;
        return true;
    }
    else if (mappedCommand == id_MotionHalfPageBackward)
    {
        GetCurrentWindow()->MoveCursorY(-(GetCurrentWindow()->GetNumDisplayedLines() / 2) * context.count);
        context.commandResult.flags |= CommandResultFlags::HandledCount;
        return true;
    }
    else if (mappedCommand == id_MotionGotoLine)
    {
        if (context.foundCount)
        {
            // In Vim, 0G means go to end!  1G is the first line...
            long count = context.count - 1;
            count = std::min(context.buffer.GetLineCount() - 1, count);
            if (count < 0)
                count = context.buffer.GetLineCount() - 1;

            long start, end;
            if (context.buffer.GetLineOffsets(count, start, end))
            {
                GetCurrentWindow()->SetBufferCursor(start);
            }
        }
        else
        {
            // Move right to the end
            auto lastLine = context.buffer.GetLinePos(MaxCursorMove, LineLocation::LineBegin);
            GetCurrentWindow()->SetBufferCursor(lastLine);
            context.commandResult.flags |= CommandResultFlags::HandledCount;
        }
        return true;
    }
    else if (mappedCommand == id_Backspace)
    {
        auto loc = context.bufferCursor;

        // In insert mode, we are 'on' the character after the one we want to delete
        context.beginRange = context.buffer.LocationFromOffsetByChars(loc, -1);
        context.endRange = context.buffer.LocationFromOffsetByChars(loc, 0);
        context.op = CommandOperation::Delete;
    }
    else if (mappedCommand == id_MotionWord)
    {
        auto target = context.buffer.WordMotion(context.bufferCursor, SearchType::Word, SearchDirection::Forward);
        GetCurrentWindow()->SetBufferCursor(target);
        return true;
    }
    else if (mappedCommand == id_MotionWORD)
    {
        auto target = context.buffer.WordMotion(context.bufferCursor, SearchType::WORD, SearchDirection::Forward);
        GetCurrentWindow()->SetBufferCursor(target);
        return true;
    }
    else if (mappedCommand == id_MotionBackWord)
    {
        auto target = context.buffer.WordMotion(context.bufferCursor, SearchType::Word, SearchDirection::Backward);
        GetCurrentWindow()->SetBufferCursor(target);
        return true;
    }
    else if (mappedCommand == id_MotionBackWORD)
    {
        auto target = context.buffer.WordMotion(context.bufferCursor, SearchType::WORD, SearchDirection::Backward);
        GetCurrentWindow()->SetBufferCursor(target);
        return true;
    }
    else if (mappedCommand == id_MotionEndWord)
    {
        auto target = context.buffer.EndWordMotion(context.bufferCursor, SearchType::Word, SearchDirection::Forward);
        GetCurrentWindow()->SetBufferCursor(target);
        return true;
    }
    else if (mappedCommand == id_MotionEndWORD)
    {
        auto target = context.buffer.EndWordMotion(context.bufferCursor, SearchType::WORD, SearchDirection::Forward);
        GetCurrentWindow()->SetBufferCursor(target);
        return true;
    }
    else if (mappedCommand == id_MotionBackEndWord)
    {
        auto target = context.buffer.EndWordMotion(context.bufferCursor, SearchType::Word, SearchDirection::Backward);
        GetCurrentWindow()->SetBufferCursor(target);
        return true;
    }
    else if (mappedCommand == id_MotionBackEndWORD)
    {
        auto target = context.buffer.EndWordMotion(context.bufferCursor, SearchType::WORD, SearchDirection::Backward);
        GetCurrentWindow()->SetBufferCursor(target);
        return true;
    }
    else if (mappedCommand == id_MotionGotoBeginning)
    {
        GetCurrentWindow()->SetBufferCursor(BufferLocation{ 0 });
        return true;
    }
    else if (mappedCommand == id_JoinLines)
    {
        // Delete the CR (and thus join lines)
        context.beginRange = context.buffer.GetLinePos(context.bufferCursor, LineLocation::LineCRBegin);
        context.endRange = context.buffer.GetLinePos(context.bufferCursor, LineLocation::BeyondLineEnd);

        // Skip white space (as the J append command does)
        context.tempReg.text = " ";
        context.pRegister = &context.tempReg;
        context.endRange = buffer.GetLinePos(context.endRange, LineLocation::LineFirstGraphChar);
        context.replaceRangeMode = ReplaceRangeMode::Replace;

        context.op = CommandOperation::Replace;
    }
    else if (mappedCommand == id_VisualMode || mappedCommand == id_VisualLineMode)
    {
        if (m_currentMode == EditorMode::Visual)
        {
            context.commandResult.modeSwitch = EditorMode::Normal;
        }
        else
        {
            if (mappedCommand == id_VisualLineMode)
            {
                m_visualBegin = context.buffer.GetLinePos(context.bufferCursor, LineLocation::LineBegin);
                m_visualEnd = context.buffer.GetLinePos(context.bufferCursor, LineLocation::BeyondLineEnd);
            }
            else
            {
                m_visualBegin = context.bufferCursor;
                m_visualEnd = m_visualBegin;
            }
            context.commandResult.modeSwitch = EditorMode::Visual;
        }
        m_lineWise = (mappedCommand == id_VisualLineMode ? true : false);
        return true;
    }
    else if (mappedCommand == id_Delete)
    {
        auto loc = context.bufferCursor;

        if (m_currentMode == EditorMode::Visual)
        {
            auto range = GetNormalizedVisualRange();
            context.beginRange = range.x;
            context.endRange = range.y;
            context.op = CommandOperation::Delete;
            context.commandResult.modeSwitch = EditorMode::Normal;
        }
        else
        {
            // Don't allow x to delete beyond the end of the line
            // Not sure what/where this is from!
            if (context.command != "x" || std::isgraph(ToASCII(context.buffer.GetText()[loc])) || std::isblank(ToASCII(context.buffer.GetText()[loc])))
            {
                context.beginRange = loc;
                context.endRange = std::min(context.buffer.GetLinePos(loc, LineLocation::LineCRBegin),
                    context.buffer.LocationFromOffsetByChars(loc, context.count));
                context.op = CommandOperation::Delete;
                context.commandResult.flags |= CommandResultFlags::HandledCount;
            }
            else
            {
                ResetCommand();
            }
        }
    }
    else if (mappedCommand == id_OpenLineBelow)
    {
        context.beginRange = context.buffer.GetLinePos(context.bufferCursor, LineLocation::LineCRBegin);
        context.tempReg.text = "\n";
        context.pRegister = &context.tempReg;
        context.op = CommandOperation::Insert;
        context.commandResult.modeSwitch = EditorMode::Insert;
    }
    else if (mappedCommand == id_OpenLineAbove)
    {
        context.beginRange = context.buffer.GetLinePos(context.bufferCursor, LineLocation::LineBegin);
        context.tempReg.text = "\n";
        context.pRegister = &context.tempReg;
        context.op = CommandOperation::Insert;
        context.commandResult.modeSwitch = EditorMode::Insert;
        context.cursorAfterOverride = context.bufferCursor;
    }
    else if (mappedCommand == id_YankLine)
    {
        // Copy the whole line, including the CR
        context.registers.push('0');
        context.beginRange = context.buffer.GetLinePos(context.bufferCursor, LineLocation::LineBegin);
        context.endRange = context.buffer.GetLinePos(context.bufferCursor, LineLocation::BeyondLineEnd);
        context.op = CommandOperation::CopyLines;
        context.commandResult.modeSwitch = EditorMode::Normal;
        context.cursorAfterOverride = context.beginRange;
    }
    else if (mappedCommand == id_Yank)
    {
        context.registers.push('0');
        auto range = GetNormalizedVisualRange();
        context.beginRange = range.x;
        context.endRange = range.y;
        // Note: select line wise yank if we started in linewise copy mode
        context.op = m_lineWise ? CommandOperation::CopyLines : CommandOperation::Copy;
        context.commandResult.modeSwitch = EditorMode::Normal;
        context.cursorAfterOverride = context.beginRange;
    }
    else if (mappedCommand == id_StandardCopy)
    {
        context.registers.push('0');
        auto range = GetNormalizedVisualRange();
        context.beginRange = range.x;
        context.endRange = range.y;
        context.cursorAfterOverride = context.bufferCursor;

        // Ignore empty copy
        if (context.beginRange == context.endRange)
        {
            return true;
        }

        // Note: select line wise yank if we started in linewise copy mode
        context.op = CommandOperation::Copy;
    }
    else if (mappedCommand == id_StandardPaste)
    {
        if (context.currentMode == EditorMode::Visual)
        {
            context.replaceRangeMode = ReplaceRangeMode::Replace;
            context.op = CommandOperation::Replace;
            context.pRegister = &GetEditor().GetRegister('"');
            auto range = GetNormalizedVisualRange();
            context.beginRange = range.x;
            context.endRange = range.y;
            context.cursorAfterOverride = context.beginRange + BufferLocation(context.pRegister->text.length());
            context.commandResult.modeSwitch = EditorMode::Insert;
        }
        else
        {
            context.beginRange = context.bufferCursor;
            context.op = CommandOperation::Insert;
        }
        context.commandResult.beginUndoGroup = true;
    }
    else if (mappedCommand == id_PasteAfter)
    {
        if (!context.pRegister->text.empty())
        {
            if (context.pRegister->lineWise)
            {
                context.beginRange = context.buffer.GetLinePos(context.bufferCursor, LineLocation::BeyondLineEnd);
                context.cursorAfterOverride = context.beginRange;
            }
            else
            {
                context.beginRange = context.buffer.LocationFromOffsetByChars(context.bufferCursor, 1, LineLocation::LineCRBegin);
            }
            context.op = CommandOperation::Insert;
        }
        context.commandResult.beginUndoGroup = true;
    }
    else if (mappedCommand == id_PasteBefore)
    {
        if (!context.pRegister->text.empty())
        {
            if (context.pRegister->lineWise)
            {
                context.beginRange = context.buffer.GetLinePos(context.bufferCursor, LineLocation::LineBegin);
            }
            else
            {
                context.beginRange = context.bufferCursor;
            }
            context.op = CommandOperation::Insert;
        }
        context.commandResult.beginUndoGroup = true;
    }
    else if (mappedCommand == id_InsertMode)
    {
        // TODO: I think this should be a 'command' which would get replayed with dot;
        // instead of special casing it later, we could just insert it into the stream of commands
        // Then undo/redo would replay the insert operation neatly
        context.commandResult.modeSwitch = EditorMode::Insert;
        return true;
    }
    else if (mappedCommand == id_VisualSelectInnerWORD)
    {
        if (GetOperationRange("iW", context.currentMode, context.beginRange, context.endRange))
        {
            m_visualBegin = context.beginRange;
            GetCurrentWindow()->SetBufferCursor(context.endRange - 1);
            UpdateVisualSelection();
            return true;
        }
        return true;
    }
    else if (mappedCommand == id_VisualSelectInnerWord)
    {
        if (GetOperationRange("iw", context.currentMode, context.beginRange, context.endRange))
        {
            m_visualBegin = context.beginRange;
            GetCurrentWindow()->SetBufferCursor(context.endRange - 1);
            UpdateVisualSelection();
            return true;
        }
    }
    else if (m_currentMode == EditorMode::Normal || m_currentMode == EditorMode::Visual)
    {
        if (context.command[0] == 'd' || context.command == "D")
        {
            if (context.command == "d")
            {
                // Only in visual mode; delete selected block
                if (GetOperationRange("visual", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::Delete;
                    context.commandResult.modeSwitch = EditorMode::Normal;
                }
                else
                {
                    context.commandResult.flags |= CommandResultFlags::NeedMoreChars;
                }
            }
            else if (context.command == "dd")
            {
                if (GetOperationRange("line", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::DeleteLines;
                    context.commandResult.modeSwitch = EditorMode::Normal;
                }
            }
            else if (context.command == "d$" || context.command == "D")
            {
                if (GetOperationRange("$", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::Delete;
                }
            }
            else if (context.command == "dw")
            {
                if (GetOperationRange("w", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::Delete;
                }
            }
            else if (context.command == "dW")
            {
                if (GetOperationRange("W", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::Delete;
                }
            }
            else if (context.command == "da")
            {
                context.commandResult.flags |= CommandResultFlags::NeedMoreChars;
            }
            else if (context.command == "daw")
            {
                if (GetOperationRange("aw", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::Delete;
                }
            }
            else if (context.command == "daW")
            {
                if (GetOperationRange("aW", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::Delete;
                }
            }
            else if (context.command == "di")
            {
                context.commandResult.flags |= CommandResultFlags::NeedMoreChars;
            }
            else if (context.command == "diw")
            {
                if (GetOperationRange("iw", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::Delete;
                }
            }
            else if (context.command == "diW")
            {
                if (GetOperationRange("iW", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::Delete;
                }
            }
            else if (context.command.find("dt") == 0)
            {
                if (context.command.length() == 3)
                {
                    context.beginRange = bufferCursor;
                    context.endRange = buffer.FindOnLineMotion(bufferCursor, (const utf8*)&context.command[2], SearchDirection::Forward);
                    context.op = CommandOperation::Delete;
                }
                else
                {
                    context.commandResult.flags |= CommandResultFlags::NeedMoreChars;
                }
            }
        }
        else if (context.command[0] == 'r')
        {
            if (context.command.size() == 1)
            {
                context.commandResult.flags |= CommandResultFlags::NeedMoreChars;
            }
            else
            {
                context.commandResult.flags |= CommandResultFlags::HandledCount;

                if (!buffer.InsideBuffer(bufferCursor + context.count))
                {
                    // Outside the valid buffer; an invalid replace with count!
                    return true;
                }

                context.replaceRangeMode = ReplaceRangeMode::Fill;
                context.op = CommandOperation::Replace;
                context.tempReg.text = context.command[1];
                context.pRegister = &context.tempReg;

                // Get the range from visual, or use the cursor location
                if (!GetOperationRange("visual", context.currentMode, context.beginRange, context.endRange))
                {
                    context.beginRange = bufferCursor;
                    context.endRange = buffer.LocationFromOffsetByChars(bufferCursor, context.count);
                }
                context.commandResult.modeSwitch = EditorMode::Normal;
            }
        }
        // Substitute
        else if ((context.command[0] == 's') || (context.command[0] == 'S'))
        {
            if (context.command == "S")
            {
                // Delete whole line and go to insert mode
                context.beginRange = context.buffer.GetLinePos(context.bufferCursor, LineLocation::LineBegin);
                context.endRange = context.buffer.GetLinePos(context.bufferCursor, LineLocation::LineCRBegin);
                context.op = CommandOperation::Delete;
            }
            else if (context.command == "s")
            {
                // Only in visual mode; delete selected block and go to insert mode
                if (GetOperationRange("visual", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::Delete;
                }
                // Just delete under m_bufferCursor and insert
                else if (GetOperationRange("cursor", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::Delete;
                }
            }
            else
            {
                return false;
            }
            context.commandResult.modeSwitch = EditorMode::Insert;
        }
        else if (context.command[0] == 'C' || context.command[0] == 'c')
        {
            if (context.command == "c")
            {
                // Only in visual mode; delete selected block
                if (GetOperationRange("visual", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::Delete;
                }
                else
                {
                    context.commandResult.flags |= CommandResultFlags::NeedMoreChars;
                }
            }
            else if (context.command == "cc")
            {
                if (GetOperationRange("line", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::DeleteLines;
                }
            }
            else if (context.command == "c$" || context.command == "C")
            {
                if (GetOperationRange("$", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::Delete;
                }
            }
            else if (context.command == "cw")
            {
                if (GetOperationRange("cw", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::Delete;
                }
            }
            else if (context.command == "cW")
            {
                if (GetOperationRange("cW", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::Delete;
                }
            }
            else if (context.command == "ca")
            {
                context.commandResult.flags |= CommandResultFlags::NeedMoreChars;
            }
            else if (context.command == "caw")
            {
                if (GetOperationRange("aw", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::Delete;
                }
            }
            else if (context.command == "caW")
            {
                if (GetOperationRange("aW", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::Delete;
                }
            }
            else if (context.command == "ci")
            {
                context.commandResult.flags |= CommandResultFlags::NeedMoreChars;
            }
            else if (context.command == "ciw")
            {
                if (GetOperationRange("iw", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::Delete;
                }
            }
            else if (context.command == "ciW")
            {
                if (GetOperationRange("iW", context.currentMode, context.beginRange, context.endRange))
                {
                    context.op = CommandOperation::Delete;
                }
            }
            else if (context.command.find("ct") == 0)
            {
                if (context.command.length() == 3)
                {
                    context.beginRange = bufferCursor;
                    context.endRange = buffer.FindOnLineMotion(bufferCursor, (const utf8*)&context.command[2], SearchDirection::Forward);
                    context.op = CommandOperation::Delete;
                }
                else
                {
                    context.commandResult.flags |= CommandResultFlags::NeedMoreChars;
                }
            }

            if (context.op != CommandOperation::None)
            {
                context.commandResult.modeSwitch = EditorMode::Insert;
            }
        }
        else if (context.command[0] == 'a')
        {
            if (m_currentMode == EditorMode::Visual)
            {
                if (context.command.size() < 2)
                {
                    context.commandResult.flags |= CommandResultFlags::NeedMoreChars;
                }
                else
                {
                    if (context.command[1] == 'W')
                    {
                        if (GetOperationRange("aW", context.currentMode, context.beginRange, context.endRange))
                        {
                            m_visualBegin = context.beginRange;
                            GetCurrentWindow()->SetBufferCursor(context.endRange - 1);
                            UpdateVisualSelection();
                            return true;
                        }
                        return true;
                    }
                    else if (context.command[1] == 'w')
                    {
                        if (GetOperationRange("aw", context.currentMode, context.beginRange, context.endRange))
                        {
                            m_visualBegin = context.beginRange;
                            GetCurrentWindow()->SetBufferCursor(context.endRange - 1);
                            UpdateVisualSelection();
                            return true;
                        }
                    }
                }
            }
            else
            {
                // Cursor append
                GetCurrentWindow()->SetBufferCursor(context.bufferCursor + 1);
                context.commandResult.modeSwitch = EditorMode::Insert;
                return true;
            }
        }
        else if (context.command == "A")
        {
            // Cursor append to end of line
            GetCurrentWindow()->SetBufferCursor(context.buffer.GetLinePos(bufferCursor, LineLocation::LineCRBegin));
            context.commandResult.modeSwitch = EditorMode::Insert;
            return true;
        }
        else if (context.command == "I")
        {
            // Cursor Insert beginning char of line
            GetCurrentWindow()->SetBufferCursor(context.buffer.GetLinePos(bufferCursor, LineLocation::LineFirstGraphChar));
            context.commandResult.modeSwitch = EditorMode::Insert;
            return true;
        }
        else if (context.command == ";")
        {
            if (!m_lastFind.empty())
            {
                GetCurrentWindow()->SetBufferCursor(context.buffer.FindOnLineMotion(bufferCursor, (const utf8*)m_lastFind.c_str(), m_lastFindDirection));
                return true;
            }
        }
        else if (context.command[0] == 'f')
        {
            if (context.command.length() > 1)
            {
                GetCurrentWindow()->SetBufferCursor(context.buffer.FindOnLineMotion(bufferCursor, (const utf8*)&context.command[1], SearchDirection::Forward));
                m_lastFind = context.command[1];
                m_lastFindDirection = SearchDirection::Forward;
                return true;
            }
            context.commandResult.flags |= CommandResultFlags::NeedMoreChars;
        }
        else if (context.command[0] == 'F')
        {
            if (context.command.length() > 1)
            {
                GetCurrentWindow()->SetBufferCursor(context.buffer.FindOnLineMotion(bufferCursor, (const utf8*)&context.command[1], SearchDirection::Backward));
                m_lastFind = context.command[1];
                m_lastFindDirection = SearchDirection::Backward;
                return true;
            }
            context.commandResult.flags |= CommandResultFlags::NeedMoreChars;
        }
        else if (context.command[0] == 'n')
        {
            auto pMark = buffer.FindNextMarker(context.bufferCursor, m_lastSearchDirection, RangeMarkerType::Search);
            if (pMark)
            {
                GetCurrentWindow()->SetBufferCursor(pMark->range.first);
            }
            return true;
        }
        else if (context.command[0] == 'N')
        {
            auto pMark = buffer.FindNextMarker(context.bufferCursor, m_lastSearchDirection == SearchDirection::Forward ? SearchDirection::Backward : SearchDirection::Forward, RangeMarkerType::Search);
            if (pMark)
            {
                GetCurrentWindow()->SetBufferCursor(pMark->range.first);
            }
            return true;
        }
        else if (context.command[0] == ExtKeys::RETURN)
        {
            if (context.currentMode == EditorMode::Normal)
            {
                // Normal mode - RETURN moves cursor down a line!
                GetCurrentWindow()->MoveCursorY(1);
                GetCurrentWindow()->SetBufferCursor(context.buffer.GetLinePos(GetCurrentWindow()->GetBufferCursor(), LineLocation::LineBegin));
                return true;
            }
            return false;
        }
        else
        {
            return false;
        }
    }
    else if (m_currentMode == EditorMode::Insert)
    {
        std::string strText = context.fullCommand;
        if (strText[0] == ExtKeys::RETURN)
        {
            strText = "\n";
        }
        else if (strText[0] == ExtKeys::TAB)
        {
            // 4 spaces, obviously
            strText = "    ";
        }
        // We are insert mode and didn't find a mapped operation
        auto cmd = std::make_shared<ZepCommand_Insert>(
            context.buffer,
            context.bufferCursor, // Start Offset
            strText, // String
            context.bufferCursor); // Where the cursor is before
        context.commandResult.flags |= CommandResultFlags::HandledCount;
        context.commandResult.spCommand = std::static_pointer_cast<ZepCommand>(cmd);
        return true;
    }

    // Update the registers based on context state
    context.UpdateRegisters();

    // Setup command, if any
    if (context.op == CommandOperation::Delete || context.op == CommandOperation::DeleteLines)
    {
        auto cmd = std::make_shared<ZepCommand_DeleteRange>(
            context.buffer,
            context.beginRange,
            context.endRange,
            context.bufferCursor);
        context.commandResult.spCommand = std::static_pointer_cast<ZepCommand>(cmd);
        return true;
    }
    else if (context.op == CommandOperation::Insert && !context.pRegister->text.empty())
    {
        auto cmd = std::make_shared<ZepCommand_Insert>(
            context.buffer,
            context.beginRange,
            context.pRegister->text,
            context.bufferCursor,
            context.cursorAfterOverride);
        context.commandResult.spCommand = std::static_pointer_cast<ZepCommand>(cmd);
        return true;
    }
    else if (context.op == CommandOperation::Replace && !context.pRegister->text.empty())
    {
        auto cmd = std::make_shared<ZepCommand_ReplaceRange>(
            context.buffer,
            context.replaceRangeMode,
            context.beginRange,
            context.endRange,
            context.pRegister->text,
            context.bufferCursor,
            context.cursorAfterOverride);
        context.commandResult.spCommand = std::static_pointer_cast<ZepCommand>(cmd);
        return true;
    }
    else if (context.op == CommandOperation::Copy || context.op == CommandOperation::CopyLines)
    {
        // Put the cursor where the command says it should be
        GetCurrentWindow()->SetBufferCursor(context.cursorAfterOverride);
        return true;
    }

    return false;
}

void ZepMode::ResetCommand()
{
    m_currentCommand.clear();
}

bool ZepMode::GetOperationRange(const std::string& op, EditorMode currentMode, BufferLocation& beginRange, BufferLocation& endRange) const
{
    auto& buffer = GetCurrentWindow()->GetBuffer();
    const auto bufferCursor = GetCurrentWindow()->GetBufferCursor();

    beginRange = BufferLocation{ -1 };
    if (op == "visual")
    {
        if (currentMode == EditorMode::Visual)
        {
            auto range = GetNormalizedVisualRange();
            beginRange = range.x;
            endRange = range.y;
        }
    }
    else if (op == "line")
    {
        // Whole line
        beginRange = buffer.GetLinePos(bufferCursor, LineLocation::LineBegin);
        endRange = buffer.GetLinePos(bufferCursor, LineLocation::BeyondLineEnd);
    }
    else if (op == "$")
    {
        beginRange = bufferCursor;
        endRange = buffer.GetLinePos(bufferCursor, LineLocation::LineCRBegin);
    }
    else if (op == "w")
    {
        beginRange = bufferCursor;
        endRange = buffer.WordMotion(bufferCursor, SearchType::Word, SearchDirection::Forward);
    }
    else if (op == "cw")
    {
        // Change word doesn't extend over the next space
        beginRange = bufferCursor;
        endRange = buffer.ChangeWordMotion(bufferCursor, SearchType::Word, SearchDirection::Forward);
    }
    else if (op == "cW")
    {
        beginRange = bufferCursor;
        endRange = buffer.ChangeWordMotion(bufferCursor, SearchType::WORD, SearchDirection::Forward);
    }
    else if (op == "W")
    {
        beginRange = bufferCursor;
        endRange = buffer.WordMotion(bufferCursor, SearchType::WORD, SearchDirection::Forward);
    }
    else if (op == "aw")
    {
        auto range = buffer.AWordMotion(bufferCursor, SearchType::Word);
        beginRange = range.first;
        endRange = range.second;
    }
    else if (op == "aW")
    {
        auto range = buffer.AWordMotion(bufferCursor, SearchType::WORD);
        beginRange = range.first;
        endRange = range.second;
    }
    else if (op == "iw")
    {
        auto range = buffer.InnerWordMotion(bufferCursor, SearchType::Word);
        beginRange = range.first;
        endRange = range.second;
    }
    else if (op == "iW")
    {
        auto range = buffer.InnerWordMotion(bufferCursor, SearchType::WORD);
        beginRange = range.first;
        endRange = range.second;
    }
    else if (op == "cursor")
    {
        beginRange = bufferCursor;
        endRange = buffer.LocationFromOffsetByChars(bufferCursor, 1);
    }
    return beginRange != -1;
}

void ZepMode::UpdateVisualSelection()
{
    // Visual mode update - after a command
    if (m_currentMode == EditorMode::Visual)
    {
        // Update the visual range
        if (m_lineWise)
        {
            m_visualEnd = GetCurrentWindow()->GetBuffer().GetLinePos(GetCurrentWindow()->GetBufferCursor(), LineLocation::BeyondLineEnd);
        }
        else
        {
            // In standard mode, when using an insert cursor, the visual selection stops at the character before the cursor insert point
            if (m_visualCursorType == CursorType::Insert)
            {
                m_visualEnd = GetCurrentWindow()->GetBufferCursor();
            }
            else
            {
                m_visualEnd = GetCurrentWindow()->GetBuffer().LocationFromOffsetByChars(GetCurrentWindow()->GetBufferCursor(), 1);
            }
        }

        auto range = GetNormalizedVisualRange();
        GetCurrentWindow()->GetBuffer().SetSelection(BufferRange{ range.x, range.y });
    }
}

bool ZepMode::HandleExCommand(std::string strCommand)
{
    if (strCommand.empty())
        return false;

    if (strCommand[strCommand.size() - 1] == ExtKeys::BACKSPACE)
    {
        // Remove the backspace
        m_currentCommand.pop_back();

        // Remove the previous character
        if (!m_currentCommand.empty())
            m_currentCommand.pop_back();

        if (m_currentCommand.empty())
        {
            GetEditor().GetActiveTabWindow()->GetActiveWindow()->SetBufferCursor(m_exCommandStartLocation);
            return true;
        }
        return false;
    }
    if (strCommand[strCommand.size() - 1] == ExtKeys::RETURN)
    {
        auto pWindow = GetEditor().GetActiveTabWindow()->GetActiveWindow();
        auto& buffer = pWindow->GetBuffer();
        auto bufferCursor = pWindow->GetBufferCursor();

        if (strCommand[0] == '/' || strCommand[0] == '?')
        {
            // Just exit Ex mode when finished the search
            return true;
        }

        // Remove the return
        strCommand = strCommand.substr(0, strCommand.length() - 1);
        if (strCommand == "")
        {
            return false;
        }

        if (GetEditor().Broadcast(std::make_shared<ZepMessage>(Msg::HandleCommand, strCommand)))
        {
            return true;
        }

        if (strCommand == ":reg")
        {
            std::ostringstream str;
            str << "--- Registers ---" << '\n';
            for (auto& reg : GetEditor().GetRegisters())
            {
                if (!reg.second.text.empty())
                {
                    std::string displayText = reg.second.text;
                    displayText = string_replace(displayText, "\n", "^J");
                    str << "\"" << reg.first << "   " << displayText << '\n';
                }
            }
            GetEditor().SetCommandText(str.str());
        }
        else if (strCommand.find(":tabedit") == 0)
        {
            auto pTab = GetEditor().AddTabWindow();
            auto strTok = string_split(strCommand, " ");
            if (strTok.size() > 1)
            {
                if (strTok[1] == "%")
                {
                    pTab->AddWindow(&buffer, nullptr, true);
                }
                else
                {
                    auto fname = strTok[1];
                    auto pBuffer = GetEditor().GetFileBuffer(fname);
                    pTab->AddWindow(pBuffer, nullptr, true);
                }
            }
            GetEditor().SetCurrentTabWindow(pTab);
        }
        else if (strCommand.find(":repl") == 0)
        {
            GetEditor().AddRepl();
        }
        else if (strCommand.find(":orca") == 0)
        {
            GetEditor().AddOrca();
        }
        else if (strCommand.find(":tree") == 0)
        {
            GetEditor().AddTree();
        }
        else if (strCommand.find(":vsplit") == 0)
        {
            auto pTab = GetEditor().GetActiveTabWindow();
            auto strTok = string_split(strCommand, " ");
            if (strTok.size() > 1)
            {
                if (strTok[1] == "%")
                {
                    pTab->AddWindow(&GetEditor().GetActiveTabWindow()->GetActiveWindow()->GetBuffer(), pWindow, true);
                }
                else
                {
                    auto fname = strTok[1];
                    auto pBuffer = GetEditor().GetFileBuffer(fname);
                    pTab->AddWindow(pBuffer, pWindow, true);
                }
            }
            else
            {
                pTab->AddWindow(&GetEditor().GetActiveTabWindow()->GetActiveWindow()->GetBuffer(), pWindow, true);
            }
        }
        else if (strCommand.find(":hsplit") == 0 || strCommand.find(":split") == 0)
        {
            auto pTab = GetEditor().GetActiveTabWindow();
            auto strTok = string_split(strCommand, " ");
            if (strTok.size() > 1)
            {
                if (strTok[1] == "%")
                {
                    pTab->AddWindow(&GetEditor().GetActiveTabWindow()->GetActiveWindow()->GetBuffer(), pWindow, false);
                }
                else
                {
                    auto fname = strTok[1];
                    auto pBuffer = GetEditor().GetFileBuffer(fname);
                    pTab->AddWindow(pBuffer, pWindow, false);
                }
            }
            else
            {
                pTab->AddWindow(&GetEditor().GetActiveTabWindow()->GetActiveWindow()->GetBuffer(), pWindow, false);
            }
        }
        else if (strCommand.find(":e") == 0)
        {
            auto strTok = string_split(strCommand, " ");
            if (strTok.size() > 1)
            {
                auto fname = strTok[1];
                auto pBuffer = GetEditor().GetFileBuffer(fname);
                pWindow->SetBuffer(pBuffer);
            }
        }
        else if (strCommand.find(":w") == 0)
        {
            auto strTok = string_split(strCommand, " ");
            if (strTok.size() > 1)
            {
                auto fname = strTok[1];
                GetCurrentWindow()->GetBuffer().SetFilePath(fname);
            }
            GetEditor().SaveBuffer(pWindow->GetBuffer());
        }
        else if (strCommand == ":close" || strCommand == ":clo")
        {
            GetEditor().GetActiveTabWindow()->CloseActiveWindow();
        }
        else if (strCommand[1] == 'q')
        {
            if (strCommand == ":q")
            {
                GetEditor().GetActiveTabWindow()->CloseActiveWindow();
            }
        }
        else if (strCommand.find(":ZTestFloatSlider") == 0)
        {
            auto line = buffer.GetBufferLine(bufferCursor);
            auto pSlider = std::make_shared<FloatSlider>(GetEditor(), 4);
            buffer.AddLineWidget(line, pSlider);
        }
        else if (strCommand.find(":ZTestMarkers") == 0)
        {
            int markerType = 0;
            auto strTok = string_split(strCommand, " ");
            if (strTok.size() > 1)
            {
                markerType = std::stoi(strTok[1]);
            }
            auto spMarker = std::make_shared<RangeMarker>();
            long start, end;

            if (GetCurrentWindow()->GetBuffer().HasSelection())
            {
                auto range = GetCurrentWindow()->GetBuffer().GetSelection();
                start = range.first;
                end = range.second;
            }
            else
            {
                start = buffer.GetLinePos(bufferCursor, LineLocation::LineFirstGraphChar);
                end = buffer.GetLinePos(bufferCursor, LineLocation::LineLastGraphChar) + 1;
            }
            spMarker->range = BufferRange{ start, end };
            switch (markerType)
            {
            case 5:
                spMarker->highlightColor = ThemeColor::Error;
                spMarker->backgroundColor = ThemeColor::Error;
                spMarker->textColor = ThemeColor::Text;
                spMarker->name = "All Marker";
                spMarker->description = "This is an example tooltip\nThey can be added to any range of characters";
                spMarker->displayType = RangeMarkerDisplayType::All;
                break;
            case 4:
                spMarker->highlightColor = ThemeColor::Error;
                spMarker->backgroundColor = ThemeColor::Error;
                spMarker->textColor = ThemeColor::Text;
                spMarker->name = "Filled Marker";
                spMarker->description = "This is an example tooltip\nThey can be added to any range of characters";
                spMarker->displayType = RangeMarkerDisplayType::Tooltip | RangeMarkerDisplayType::Underline | RangeMarkerDisplayType::Indicator | RangeMarkerDisplayType::Background;
                break;
            case 3:
                spMarker->highlightColor = ThemeColor::TabActive;
                spMarker->textColor = ThemeColor::Text;
                spMarker->name = "Underline Marker";
                spMarker->description = "This is an example tooltip\nThey can be added to any range of characters";
                spMarker->displayType = RangeMarkerDisplayType::Tooltip | RangeMarkerDisplayType::Underline | RangeMarkerDisplayType::Indicator;
                break;
            case 2:
                spMarker->highlightColor = ThemeColor::Warning;
                spMarker->textColor = ThemeColor::Text;
                spMarker->name = "Tooltip";
                spMarker->description = "This is an example tooltip\nThey can be added to any range of characters";
                spMarker->displayType = RangeMarkerDisplayType::Tooltip;
                break;
            case 1:
                spMarker->highlightColor = ThemeColor::Warning;
                spMarker->textColor = ThemeColor::Text;
                spMarker->name = "Warning";
                spMarker->description = "This is an example warning mark";
                break;
            case 0:
            default:
                spMarker->highlightColor = ThemeColor::Error;
                spMarker->textColor = ThemeColor::Text;
                spMarker->name = "Error";
                spMarker->description = "This is an example error mark";
            }
            buffer.AddRangeMarker(spMarker);
            SwitchMode(EditorMode::Normal);
        }
        else if (strCommand == ":ZShowCR")
        {
            pWindow->ToggleFlag(WindowFlags::ShowCR);
        }
        else if (strCommand == ":ZThemeToggle")
        {
            // An easy test command to check per-buffer themeing
            // Just gets the current theme on the buffer and makes a new
            // one that is opposite
            auto theme = pWindow->GetBuffer().GetTheme();
            auto spNewTheme = std::make_shared<ZepTheme>();
            if (theme.GetThemeType() == ThemeType::Dark)
            {
                spNewTheme->SetThemeType(ThemeType::Light);
            }
            else
            {
                spNewTheme->SetThemeType(ThemeType::Dark);
            }
            pWindow->GetBuffer().SetTheme(spNewTheme);
        }
        else if (strCommand == ":ls")
        {
            std::ostringstream str;
            str << "--- buffers ---" << '\n';
            int index = 0;
            for (auto& editor_buffer : GetEditor().GetBuffers())
            {
                if (!editor_buffer->GetName().empty())
                {
                    std::string displayText = editor_buffer->GetName();
                    displayText = string_replace(displayText, "\n", "^J");
                    if (editor_buffer->IsHidden())
                    {
                        str << "h";
                    }
                    else
                    {
                        str << " ";
                    }

                    if (&GetCurrentWindow()->GetBuffer() == editor_buffer.get())
                    {
                        str << "*";
                    }
                    else
                    {
                        str << " ";
                    }

                    if (editor_buffer->TestFlags(FileFlags::Dirty))
                    {
                        str << "+";
                    }
                    else
                    {
                        str << " ";
                    }
                    str << index++ << " : " << displayText << '\n';
                }
            }
            GetEditor().SetCommandText(str.str());
        }
        else if (strCommand.find(":bu") == 0)
        {
            auto strTok = string_split(strCommand, " ");

            if (strTok.size() > 1)
            {
                try
                {
                    auto index = std::stoi(strTok[1]);
                    auto current = 0;
                    for (auto& editor_buffer : GetEditor().GetBuffers())
                    {
                        if (index == current)
                        {
                            GetCurrentWindow()->SetBuffer(editor_buffer.get());
                        }
                        current++;
                    }
                }
                catch (std::exception&)
                {
                }
            }
        }
        else
        {
            GetEditor().SetCommandText("Not a command");
        }
        return true;
    }
    else if (!m_currentCommand.empty() && m_currentCommand[0] == '/' || m_currentCommand[0] == '?')
    {
        // TODO: Busy editing the search string; do the search
        if (m_currentCommand.length() > 0)
        {
            auto pWindow = GetEditor().GetActiveTabWindow()->GetActiveWindow();
            auto& buffer = pWindow->GetBuffer();
            auto searchString = m_currentCommand.substr(1);

            buffer.ClearRangeMarkers(RangeMarkerType::Search);

            uint32_t numMarkers = 0;
            BufferLocation start = 0;

            if (!searchString.empty())
            {
                static const uint32_t MaxMarkers = 1000;
                while (numMarkers < MaxMarkers)
                {
                    auto found = buffer.Find(start, (utf8*)&searchString[0], (utf8*)&searchString[searchString.length()]);
                    if (found == InvalidOffset)
                    {
                        break;
                    }

                    start = found + 1;

                    auto spMarker = std::make_shared<RangeMarker>();
                    spMarker->backgroundColor = ThemeColor::VisualSelectBackground;
                    spMarker->textColor = ThemeColor::Text;
                    spMarker->range = BufferRange(found, BufferLocation(found + searchString.length()));
                    spMarker->displayType = RangeMarkerDisplayType::Background;
                    spMarker->markerType = RangeMarkerType::Search;
                    buffer.AddRangeMarker(spMarker);

                    numMarkers++;
                }
            }

            SearchDirection dir = (m_currentCommand[0] == '/') ? SearchDirection::Forward : SearchDirection::Backward;
            m_lastSearchDirection = dir;

            // Find the one on or in front of the cursor, in either direction.
            auto startLocation = m_exCommandStartLocation;
            if (dir == SearchDirection::Forward)
                startLocation--;
            else
                startLocation++;

            auto pMark = buffer.FindNextMarker(startLocation, dir, RangeMarkerType::Search);
            if (pMark)
            {
                pWindow->SetBufferCursor(pMark->range.first);
                pMark->backgroundColor = ThemeColor::Info;
            }
            else
            {
                pWindow->SetBufferCursor(m_exCommandStartLocation);
            }
        }
    }
    return false;
}

} // namespace Zep
