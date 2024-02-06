#include <cerberus/core/rcp.hxx>
#include <vector>

namespace cerberus
{
    void RCP::reset()
    {
        rsp.Reset();
        rdp.Reset();
        vi.Reset();
    }

    void RCP::redraw(std::vector<uint8_t>& data)
    {
        vi.Redraw(data);
    }

    int RCP::getWidth()
    {
        return vi.width_;
    }

    int RCP::getHeight()
    {
        return vi.height_;
    }

    void RCP::setAudioCallback(void (*callback)(const int16_t*, uint32_t, int))
    {
        ai.SetAudioCallback(callback);
    }
} // namespace cerberus