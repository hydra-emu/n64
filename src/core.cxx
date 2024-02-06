#include <chrono>
#include <core.hxx>
#include <n64_scheduler.hxx>
#include <iostream>

// TODO: cmake option
// #define PROFILING
#ifdef PROFILING
#include <valgrind/callgrind.h>
#else
#define CALLGRIND_START_INSTRUMENTATION
#define CALLGRIND_STOP_INSTRUMENTATION
#endif

namespace cerberus
{
    N64::N64() : vr4300(scheduler, rcp) {}

    bool N64::LoadCartridge(const std::filesystem::path& path)
    {
        bool loaded = vr4300.LoadCartridge(path);
        if (loaded)
            Reset();
        return loaded;
    }

    bool N64::LoadIPL(const std::filesystem::path& path)
    {
        return vr4300.LoadIPL(path);
    }

    void N64::RunFrame()
    {
        CALLGRIND_START_INSTRUMENTATION;
        static int cycles = 0;
        for (int f = 0; f < 1; f++)
        { // fields
            for (int hl = 0; hl < vr4300.rcp_.vi_.num_halflines_; hl++)
            { // halflines
                vr4300.rcp_.vi_.vi_v_current_ = (hl << 1) + f;
                vr4300.check_vi_interrupt();
                while (cycles <= vr4300.rcp_.vi_.cycles_per_halfline_)
                {
                    static int cpu_cycles = 0;
                    cpu_cycles++;
                    vr4300.Tick();
                    rcp.ai_.Step();
                    if (!vr4300.rcp_.rsp_.IsHalted())
                    {
                        while (cpu_cycles > 2)
                        {
                            vr4300.rcp_.rsp_.Tick();
                            if (!vr4300.rcp_.rsp_.IsHalted())
                            {
                                vr4300.rcp_.rsp_.Tick();
                            }
                            cpu_cycles -= 3;
                        }
                    }
                    else
                    {
                        cpu_cycles = 0;
                    }
                    cycles++;
                }
                cycles -= vr4300.rcp_.vi_.cycles_per_halfline_;
            }
            vr4300.check_vi_interrupt();
        }
        CALLGRIND_STOP_INSTRUMENTATION;
    }

    void N64::Reset()
    {
        scheduler.reset();
        vr4300.Reset();
        rcp.Reset();
    }

    void N64::SetMousePos(int32_t x, int32_t y)
    {
        vr4300.mouse_delta_x_ = x - vr4300.mouse_x_;
        vr4300.mouse_delta_y_ = y - vr4300.mouse_y_;
        vr4300.mouse_x_ = x;
        vr4300.mouse_y_ = y;
    }

    void N64::SetAudioCallback(void(*callback)(const int16_t*, uint32_t, int))
    {
        rcp.ai_.SetAudioCallback(callback);
    }

} // namespace cerberus