#pragma once

#include <cstdint>
#include <vector>

namespace cerberus
{

    struct RDPDevice
    {
        virtual ~RDPDevice() = default;
        virtual void initialize(uint32_t width, uint32_t height) = 0;
        virtual void dispatch(const std::vector<uint64_t>& commands) = 0;
        virtual void destroy() = 0;
    };

    struct NullRDPDevice : RDPDevice
    {
        void initialize(uint32_t width, uint32_t height) override {}

        void dispatch(const std::vector<uint64_t>& commands) override {}

        void destroy() override {}
    };

} // namespace cerberus