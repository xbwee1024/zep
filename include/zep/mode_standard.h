#pragma once

#include "mode.h"

namespace Zep
{

class ZepMode_Standard : public ZepMode
{
public:
    ZepMode_Standard(ZepEditor& editor);
    ~ZepMode_Standard();

    virtual std::shared_ptr<CommandContext> AddKeyPress(uint32_t key, uint32_t modifiers = 0) override;
    virtual void Begin() override;

    static const char* StaticName()
    {
        return "Standard";
    }
    virtual const char* Name() const override
    {
        return StaticName();
    }

private:
    std::string keyCache;
};

} // namespace Zep
