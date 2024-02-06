#pragma once

#include <cerberus/devices/rdp_device.hxx>
#include <memory>

namespace cerberus
{
    struct Config
    {
        std::shared_ptr<RDPDevice> videoDevice = std::make_shared<NullRDPDevice>();
    };
} // namespace cerberus