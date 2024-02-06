#include "cerberus/core/vr4300i.hxx"
#include <cerberus/core/core.hxx>
#include <cstdint>
#include <hydra/core.hxx>

class HydraCore : public hydra::IBase,
                  public hydra::ISoftwareRendered,
                  public hydra::IFrontendDriven,
                  public hydra::IInput
{
    HYDRA_CLASS

public:
    HydraCore()
    {
        n64.SetAudioCallback([](const int16_t*, uint32_t, int) {});
    }

    // IBase
    bool loadFile(const char* type, const char* rom) override
    {
        printf("Loading %s: %s\n", type, rom);
        std::string stype = type;
        if (stype == "rom")
        {
            n64.loadCartridge(rom);
            return true;
        }
        else if (stype == "bios")
        {
            n64.loadIpl(rom);
            return true;
        }
        return false;
    }

    void reset() override
    {
        n64.reset();
    }

    hydra::Size getNativeSize() override
    {
        return {(uint32_t)n64.GetWidth(), (uint32_t)n64.GetHeight()};
    }

    void setOutputSize(hydra::Size size) override {}

    // ISoftwareRendered
    void setVideoCallback(void (*callback)(void* data, hydra::Size size)) override
    {
        video_callback = callback;
    };

    // IFrontendDriven
    void runFrame() override
    {
        n64.run();
        std::vector<uint8_t> data;
        n64.RenderVideo(data);
        video_callback(data.data(), {(uint32_t)n64.GetWidth(), (uint32_t)n64.GetHeight()});
    }

    uint16_t getFps() override
    {
        return 60;
    };

    void setPollInputCallback(void (*callback)()) override
    {
        n64.SetPollInputCallback(callback);
    }

    void setCheckButtonCallback(int32_t (*callback)(uint32_t, hydra::ButtonType)) override
    {
        n64.SetReadInputCallback(callback);
    }

private:
    cerberus::N64<cerberus::CPU> n64;
    void (*video_callback)(void* data, hydra::Size size) = nullptr;
};

HC_API hydra::IBase* createEmulator()
{
    return new HydraCore;
}

HC_API void destroyEmulator(hydra::IBase* emulator)
{
    delete emulator;
}

HC_API const char* getInfo(hydra::InfoType type)
{
    switch (type)
    {
        case hydra::InfoType::CoreName:
            return "Cerberus";
        case hydra::InfoType::SystemName:
            return "Nintendo 64";
        case hydra::InfoType::Version:
            return "1.0";
        case hydra::InfoType::Author:
            return "OFFTKP";
        case hydra::InfoType::Website:
            return "todo: add me";
        case hydra::InfoType::Description:
            return "An n64 emulator";
        case hydra::InfoType::Extensions:
            return "z64,N64";
        case hydra::InfoType::License:
            return "MIT";
        case hydra::InfoType::Settings:
            return R"(
            [bios]
            type = "filepicker"
            name = "IPL3"
            description = "The IPL is required to run games"
            required = true
        )";
        default:
            return nullptr;
    }
}