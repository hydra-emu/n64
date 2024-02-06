#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace cerberus
{
    class RCP;
    class CPU;

    struct Vi
    {
        void Reset();
        void Redraw(std::vector<uint8_t>& data);
        uint32_t ReadWord(uint32_t addr);
        void WriteWord(uint32_t addr, uint32_t data);
        void InstallBuses(uint8_t* rdram_ptr);
        void SetInterruptCallback(std::function<void(bool)> callback);

    private:
        uint32_t vi_ctrl_ = 0;
        uint32_t vi_origin_ = 0;
        uint32_t vi_width_ = 0;
        uint32_t vi_v_intr_ = 0x3ff;
        uint32_t vi_v_current_ = 0;
        uint32_t vi_burst_ = 0;
        uint32_t vi_v_sync_ = 0;
        uint32_t vi_h_sync_ = 0;
        uint32_t vi_h_sync_leap_ = 0;
        uint32_t vi_h_start_ = 0;
        uint32_t vi_h_end_ = 0;
        uint32_t vi_v_start_ = 0;
        uint32_t vi_v_end_ = 0;
        uint32_t vi_v_burst_ = 0;
        uint32_t vi_x_scale_ = 0;
        uint32_t vi_y_scale_ = 0;

        int width_ = 320, height_ = 240;
        int num_halflines_ = 262;
        int cycles_per_halfline_ = 1000;

        uint8_t pixel_mode_ = 0;
        uint8_t* rdram_ptr_ = nullptr;
        std::function<void(bool)> interrupt_callback_;

        friend class cerberus::RCP;
        friend class cerberus::CPU;
    };
} // namespace cerberus