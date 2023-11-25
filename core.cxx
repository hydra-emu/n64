#include <hydra/core.hxx>

class HydraCore : public hydra::IBase
{
    HYDRA_CLASS
public:
    bool loadFile(const char* type, const char* rom) override;
    void reset() override;
    hydra::Size getNativeSize() override;
    void setOutputSize(hydra::Size size) override;
};

HC_GLOBAL hydra::IBase* createEmulator()
{
    return new HydraCore;
}

HC_GLOBAL void destroyEmulator(hydra::IBase* emulator)
{
    delete emulator;
}

HC_GLOBAL const char* getInfo(hydra::InfoType type)
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
    case hydra::InfoType::Description:
        return "An n64 emulator";
    case hydra::InfoType::Extensions:
        return "z64";
    case hydra::InfoType::License:
        return "MIT";
    case hydra::InfoType::Firmware:
        return "IPL";
    default:
        return nullptr;
    }
}