#pragma once

#include "mode.h"
#include <future>
#include <memory>
#include <regex>

namespace Zep
{

struct ZepRepl;

class ZepMode_Repl : public ZepMode
{
public:
    ZepMode_Repl(ZepEditor& editor, ZepRepl* pRepl);
    ~ZepMode_Repl();

    virtual void AddKeyPress(uint32_t key, uint32_t modifiers = 0) override;
    virtual void Begin() override;
    virtual void Notify(std::shared_ptr<ZepMessage> message) override;
    
    static const char* StaticName()
    {
        return "REPL";
    }
    virtual const char* Name() const override
    {
        return StaticName();
    }

private:
    void BeginInput();
    BufferLocation m_startLocation = BufferLocation{ 0 };
    ZepRepl* m_pRepl = nullptr;
};

} // namespace Zep
