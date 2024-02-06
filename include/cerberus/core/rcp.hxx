#pragma once

#include <array>
#include <cerberus/core/audio_interface.hxx>
#include <cerberus/core/rdp.hxx>
#include <cerberus/core/rsp.hxx>
#include <cerberus/core/video_interface.hxx>
#include <cstdint>

namespace cerberus
{
    class RCP final
    {
    public:
        void reset();
        void redraw(std::vector<uint8_t>& data);
        int getWidth();
        int getHeight();
        void setAudioCallback(void (*callback)(const int16_t*, uint32_t, int));

    private:
        Vi vi;
        Ai ai;
        RSP rsp;
        RDP rdp;

        friend class cerberus::CPU;
    };
} // namespace cerberus
