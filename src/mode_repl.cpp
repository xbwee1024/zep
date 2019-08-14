#include "zep/mode_repl.h"
#include "zep/filesystem.h"
#include "zep/tab_window.h"
#include "zep/window.h"
#include "zep/editor.h"

#include "zep/mcommon/logger.h"
#include "zep/mcommon/threadutils.h"

namespace Zep
{

ZepMode_Repl::ZepMode_Repl(ZepEditor& editor, ZepRepl* pRepl)
    : ZepMode(editor),
    m_pRepl(pRepl)
{
}

ZepMode_Repl::~ZepMode_Repl()
{
}

void ZepMode_Repl::AddKeyPress(uint32_t key, uint32_t modifiers)
{
    auto pGlobalMode = GetEditor().GetCurrentMode();

    // Switch to insert mode and swallow the key
    if (key == 'i' && m_currentMode != EditorMode::Insert)
    {
        m_currentMode = EditorMode::Insert;
        GetCurrentWindow()->SetBufferCursor(MaxCursorMove);
        GetCurrentWindow()->SetCursorType(CursorType::Insert);
        return;
    }

    // If not in insert mode, then let the normal mode do its thing
    if (GetEditorMode() != Zep::EditorMode::Insert)
    {
        return pGlobalMode->AddKeyPress(key, modifiers);
    }
  
    // Set the cursor to the end of the buffer while inserting text
    GetCurrentWindow()->SetBufferCursor(MaxCursorMove);
    GetCurrentWindow()->SetCursorType(CursorType::Insert);

    (void)modifiers;
    if (key == ExtKeys::ESCAPE)
    {
        // Escape back to the default normal mode
        GetEditor().GetCurrentMode()->Begin();
        GetEditor().GetCurrentMode()->SetEditorMode(EditorMode::Normal);
        m_currentMode = Zep::EditorMode::Normal;
        return;
    }
    else if (key == ExtKeys::RETURN)
    {
        auto& buffer = GetCurrentWindow()->GetBuffer();
        std::string str = std::string(buffer.GetText().begin() + m_startLocation, buffer.GetText().end());
        buffer.Insert(buffer.EndLocation(), "\n");

        std::string ret;
        if (m_pRepl)
        {
            ret = m_pRepl->fnParser(str);
        }
        else
        {
            ret = str;
        }

        ret.push_back('\n');
        buffer.Insert(buffer.EndLocation(), ret);

        BeginInput();
        return;
    }
    else if (key == ExtKeys::BACKSPACE)
    {
        auto cursor = GetCurrentWindow()->GetBufferCursor() - 1;
        if (cursor >= m_startLocation)
        {
            GetCurrentWindow()->GetBuffer().Delete(GetCurrentWindow()->GetBufferCursor() - 1, GetCurrentWindow()->GetBufferCursor());
        }
    }
    else
    {
        char c[2];
        c[0] = (char)key;
        c[1] = 0;
        GetCurrentWindow()->GetBuffer().Insert(GetCurrentWindow()->GetBufferCursor(), std::string(c));
    }

    // Ensure cursor is at buffer end
    GetCurrentWindow()->SetBufferCursor(MaxCursorMove);
}

void ZepMode_Repl::BeginInput()
{
    // Input arrows
    auto& buffer = GetCurrentWindow()->GetBuffer();
    /*if (!buffer.GetText().empty())
    {
        buffer.Insert(GetCurrentWindow()->GetBuffer().EndLocation(), "\n");
    }*/

    buffer.Insert(GetCurrentWindow()->GetBuffer().EndLocation(), ">> ");

    GetCurrentWindow()->SetBufferCursor(MaxCursorMove);
    m_startLocation = GetCurrentWindow()->GetBufferCursor();
}

void ZepMode_Repl::Begin()
{
    // Default insert mode
    GetEditor().GetCurrentMode()->Begin();
    GetEditor().GetCurrentMode()->SetEditorMode(EditorMode::Insert);
    m_currentMode = EditorMode::Insert;
    GetCurrentWindow()->SetCursorType(CursorType::Insert);

    GetEditor().SetCommandText("");
    
    BeginInput();
}

void ZepMode_Repl::Notify(std::shared_ptr<ZepMessage> message)
{
    ZepMode::Notify(message);
}

} // namespace Zep
