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

    template<class CPUType>
    class N64
    {
    public:
        N64() : cpu(scheduler, rcp) {}

        bool loadCartridge(const std::filesystem::path& path)
        {
            bool loaded = cpu.loadCartridge(path);
            if (loaded)
                reset();
            return loaded;
        }

        bool loadIpl(const std::filesystem::path& path)
        {
            return cpu.loadIpl(path);
        }

        // Run about a frames worth of instructions
        void run()
        {
            cpu.run();
        }

        void reset()
        {
            scheduler.reset();
            cpu.reset();
            rcp.reset();
        }

        void SetAudioCallback(void (*callback)(const int16_t*, uint32_t, int))
        {
            rcp.setAudioCallback(callback);
        }

        void SetPollInputCallback(void (*callback)())
        {
            cpu.setPollInputCallback(callback);
        }

        void SetReadInputCallback(int32_t (*callback)(uint32_t, hydra::ButtonType))
        {
            cpu.setReadInputCallback(callback);
        }

        int GetWidth()
        {
            return rcp.getWidth();
        }

        int GetHeight()
        {
            return rcp.getHeight();
        }

        void RenderVideo(std::vector<uint8_t>& data)
        {
            rcp.redraw(data);
        }

    private:
        Scheduler scheduler;
        RCP rcp;
        CPUType cpu;
        std::shared_ptr<Config> config;
    };
} // namespace cerberus
