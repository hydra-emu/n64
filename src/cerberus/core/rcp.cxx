#include <cerberus/core/rcp.hxx>

namespace cerberus
{
    void RCP::Reset()
    {
        // These default values are in little endian
        rsp_.Reset();
        rdp_.Reset();
        vi_.Reset();
    }

} // namespace cerberus