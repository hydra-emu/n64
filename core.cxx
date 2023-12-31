#include <hydra_core/core.h>
#include <core/n64_impl.hxx>
#include <core/n64_log.hxx>

static bool ipl_loaded_ = false;
std::function<void(const uint8_t*, uint32_t, uint32_t)> video_callback_ = nullptr;
hc_audio_callback_t audio_callback_ = nullptr;
hc_read_input_callback_t read_input_callback_ = nullptr;

hc_core_t hc_create()
{
    return new hydra::N64::N64();
}

void hc_destroy(hc_core_t core)
{
    delete (hydra::N64::N64*)core;
}

const char* hc_get_info(hc_info info)
{
    switch (info)
    {
        case HC_INFO_CORE_NAME:
            return "n64hydra";
        case HC_INFO_SYSTEM_NAME:
            return "Nintendo 64";
        case HC_INFO_AUTHOR:
            return "OFFTKP";
        case HC_INFO_VERSION:
            return "0.1";
        case HC_INFO_LICENSE:
            return "MIT";
        case HC_INFO_URL:
            return "https://github.com/hydra-emu/n64";
        case HC_INFO_EXTENSIONS:
            return "z64,n64";
        case HC_INFO_DESCRIPTION:   
            return "Nintendo 64 emulator for hydra";
        case HC_INFO_MAX_PLAYERS:
            return "4";
        case HC_INFO_FIRMWARE_FILES:
            return "IPL";
        default:
            return nullptr;
    }
}

bool hc_load_file(hc_core_t core, const char* type, const char* path)
{
    hydra::N64::N64* emu = static_cast<hydra::N64::N64*>(core);
    if (std::string(type) == "IPL")
    {
        ipl_loaded_ = emu->LoadIPL(path);
        return ipl_loaded_;
    }
    else if (std::string(type) == "rom")
    {
        if (!ipl_loaded_)
            return false;
        return emu->LoadCartridge(path);
    }
    return false;
}

void hc_reset(hc_core_t core)
{
    hydra::N64::N64* emu = static_cast<hydra::N64::N64*>(core);
    emu->Reset();
}

static void audio_callback_wrapper(const int16_t* in, uint32_t in_size, int frequency_in)
{
    ma_resampler_config config = ma_resampler_config_init(ma_format_s16, 2, frequency_in, 48000,
                                                          ma_resample_algorithm_linear);
    ma_resampler resampler;

    struct Deleter
    {
        Deleter(ma_resampler& resampler) : resampler_(resampler){};

        ~Deleter()
        {
            ma_resampler_uninit(&resampler_, nullptr);
        };

    private:
        ma_resampler& resampler_;
    } deleter(resampler);

    if (ma_result res = ma_resampler_init(&config, nullptr, &resampler))
    {
        Logger::Fatal("Failed to create resampler: {}", static_cast<int>(res));
    }

    ma_uint64 frames_in = in_size >> 1;
    ma_uint64 frames_out = (48000 / 60) * 2;
    std::vector<int16_t> temp;
    temp.resize(frames_out * 2);
    ma_result result = ma_resampler_process_pcm_frames(&resampler, in, &frames_in,
                                                       temp.data(), &frames_out);
    if (result != MA_SUCCESS)
    {
        Logger::Fatal("Failed to resample: {}", static_cast<int>(result));
    }

    std::vector<int16_t> data;
    data.resize(frames_out * 2);
    std::copy(temp.begin(), temp.begin() + frames_out * 2, data.begin());
    audio_callback_(data.data(), frames_out);
}

static int8_t read_input_callback_wrapper(uint8_t player, uint8_t button)
{
    return read_input_callback_(player, static_cast<hc_input_e>(button));
}

void hc_set_video_callback(hc_core_t core, hc_video_callback_t callback)
{
    video_callback_ = callback;
}

void hc_set_audio_callback(hc_core_t core, hc_audio_callback_t callback)
{
    hydra::N64::N64* emu = static_cast<hydra::N64::N64*>(core);
    audio_callback_ = callback;
    emu->SetAudioCallback(audio_callback_wrapper);
}

void hc_set_poll_input_callback(hc_core_t core, hc_poll_input_callback_t callback)
{
    hydra::N64::N64* emu = static_cast<hydra::N64::N64*>(core);
    emu->SetPollInputCallback(callback);
}

void hc_set_read_input_callback(hc_core_t core, hc_read_input_callback_t callback)
{
    hydra::N64::N64* emu = static_cast<hydra::N64::N64*>(core);
    read_input_callback_ = callback;
    emu->SetReadInputCallback(read_input_callback_wrapper);
}

void hc_add_log_callback(hc_core_t core, const char* target, hc_log_callback_t callback)
{
    Logger::HookCallback(target, callback);
}

void hc_run_frame(hc_core_t core)
{
    hydra::N64::N64* emu = static_cast<hydra::N64::N64*>(core);
    emu->RunFrame();

    uint32_t width = emu->GetWidth();
    uint32_t height = emu->GetHeight();

    std::vector<uint8_t> data;
    emu->RenderVideo(data);

    if (data.size() != 0)
        video_callback_(data.data(), width, height);
}

void hc_set_read_other_callback(hc_read_other_callback_t callback) {}