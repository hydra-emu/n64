#pragma once

#include <array>
#include <cerberus/core/audio_interface.hxx>
#include <cerberus/core/rdp.hxx>
#include <cerberus/core/rsp.hxx>
#include <cerberus/core/video_interface.hxx>
#include <cstdint>

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
