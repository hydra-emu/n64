#pragma once

#include <cstdint>
#include <vector>

namespace cerberus
{

    struct RDPDevice
    {
        virtual ~RDPDevice() = default;
        virtual void dispatch(const std::vector<uint64_t>& commands) = 0;
    };

    struct NullRDPDevice : RDPDevice
    {
        void dispatch(const std::vector<uint64_t>& commands) override {}
    };

} // namespace cerberus