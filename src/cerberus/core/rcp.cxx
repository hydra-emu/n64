#include <cerberus/core/rcp.hxx>

namespace cerberus
{
    void RCP::Reset()
    {
        // These default values are in little endian
        rsp.Reset();
        rdp.Reset();
        vi_.Reset();
    }

} // namespace cerberus