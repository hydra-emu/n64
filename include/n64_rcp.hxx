#pragma once

#include <array>
#include <cstdint>
#include <n64_ai.hxx>
#include <n64_rdp.hxx>
#include <n64_rsp.hxx>
#include <n64_vi.hxx>

namespace cerberus
{
    class N64;
} // namespace cerberus

namespace cerberus
{
    class RCP final
    {
    public:
        void Reset();

    private:
        Vi vi_;
        Ai ai_;
        RSP rsp_;
        RDP rdp_;

        friend class cerberus::N64;
        friend class cerberus::CPU;
    };
} // namespace cerberus
