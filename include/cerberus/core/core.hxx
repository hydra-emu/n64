#pragma once

#include <cerberus/core/config.hxx>
#include <cerberus/core/rcp.hxx>
#include <cerberus/core/scheduler.hxx>
#include <cerberus/core/vr4300i.hxx>
#include <cstdint>
#include <filesystem>
#include <hydra/core.hxx>
#include <string>

namespace cerberus
{
    class N64
    {
    public:
        N64();
        bool LoadCartridge(const std::filesystem::path& path);
        bool LoadIPL(const std::filesystem::path& path);
        void RunFrame();
        void Reset();
        void SetMousePos(int32_t x, int32_t y);
        void SetAudioCallback(void (*callback)(const int16_t*, uint32_t, int));

        void SetPollInputCallback(void (*callback)())
        {
            vr4300.poll_input_callback_ = callback;
        }

        void SetReadInputCallback(int32_t (*callback)(uint32_t, hydra::ButtonType))
        {
            vr4300.read_input_callback_ = callback;
        }

        int GetWidth()
        {
            return rcp.vi_.width_;
        }

        int GetHeight()
        {
            return rcp.vi_.height_;
        }

        void RenderVideo(std::vector<uint8_t>& data)
        {
            rcp.vi_.Redraw(data);
        }

    private:
        Scheduler scheduler;
        RCP rcp;
        CPU vr4300;
        std::shared_ptr<Config> config;
    };
} // namespace cerberus
