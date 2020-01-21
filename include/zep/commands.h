#pragma once

#include "zep/buffer.h"

namespace Zep
{

class ZepCommand
{
public:
    ZepCommand(ZepBuffer& currentMode, BufferLocation cursorBefore = -1, BufferLocation cursorAfter = -1)
        : m_buffer(currentMode)
        , m_cursorBefore(cursorBefore)
        , m_cursorAfter(cursorAfter)
    {
    }

    virtual ~ZepCommand()
    {
    }

    virtual void Redo() = 0;
    virtual void Undo() = 0;

    virtual BufferLocation GetCursorAfter() const
    {
        return m_cursorAfter;
    }
    virtual BufferLocation GetCursorBefore() const
    {
        return m_cursorBefore;
    }

protected:
    ZepBuffer& m_buffer;
    BufferLocation m_cursorBefore = -1;
    BufferLocation m_cursorAfter = -1;
};

class ZepCommand_BeginGroup : public ZepCommand
{
public:
    ZepCommand_BeginGroup(ZepBuffer& currentMode) : ZepCommand(currentMode) {}
    virtual void Redo() override {};
    virtual void Undo() override {};
};

class ZepCommand_EndGroup : public ZepCommand
{
public:
    ZepCommand_EndGroup(ZepBuffer& currentMode) : ZepCommand(currentMode) {}
    virtual void Redo() override {};
    virtual void Undo() override {};
};

class ZepCommand_DeleteRange : public ZepCommand
{
public:
    ZepCommand_DeleteRange(ZepBuffer& buffer, const BufferLocation& startOffset, const BufferLocation& endOffset, const BufferLocation& cursor = BufferLocation{-1}, const BufferLocation& cursorAfter = BufferLocation{-1});
    virtual ~ZepCommand_DeleteRange(){};

    virtual void Redo() override;
    virtual void Undo() override;

    BufferLocation m_startOffset;
    BufferLocation m_endOffset;

    std::string m_deleted;
};

enum class ReplaceRangeMode
{
    Fill,
    Replace,
};

class ZepCommand_ReplaceRange : public ZepCommand
{
public:
    ZepCommand_ReplaceRange(ZepBuffer& buffer, ReplaceRangeMode replaceMode, const BufferLocation& startOffset, const BufferLocation& endOffset, const std::string& ch, const BufferLocation& cursor = BufferLocation{-1}, const BufferLocation& cursorAfter = BufferLocation{-1});
    virtual ~ZepCommand_ReplaceRange(){};

    virtual void Redo() override;
    virtual void Undo() override;

    BufferLocation m_startOffset;
    BufferLocation m_endOffset;

    std::string m_strDeleted;
    std::string m_strReplace;
    ReplaceRangeMode m_mode;
};

class ZepCommand_Insert : public ZepCommand
{
public:
    ZepCommand_Insert(ZepBuffer& buffer, const BufferLocation& startOffset, const std::string& str, const BufferLocation& cursor = BufferLocation{-1}, const BufferLocation& cursorAfter = BufferLocation{-1});
    virtual ~ZepCommand_Insert(){};

    virtual void Redo() override;
    virtual void Undo() override;

    BufferLocation m_startOffset;
    std::string m_strInsert;

    BufferLocation m_endOffsetInserted = -1;
};

} // namespace Zep
