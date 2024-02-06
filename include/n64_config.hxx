#pragma once

#include <memory>
#include <cerberus/devices/rdp_device.hxx>

namespace cerberus
{
    struct Config
    {
        std::shared_ptr<RDPDevice> videoDevice;

    };
}