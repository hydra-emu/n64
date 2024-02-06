/**
    The hydra core API
*/
#pragma once

#include <cstddef>
#include <cstdint>

#define HYDRA_CORE_MAJOR 0
#define HYDRA_CORE_MINOR 1
#define HYDRA_CORE_PATCH 0

#ifdef __cplusplus
#define HC_EXTERN_C extern "C"
#else
#define HC_EXTERN_C
#endif

#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
#define HC_GLOBAL __attribute__((visibility("default")))
#elif defined(_WIN32)
#ifdef HC_WINDOWS_IMPORT
#define HC_GLOBAL __declspec(dllimport)
#else
#define HC_GLOBAL __declspec(dllexport)
#endif
#elif defined(__EMSCRIPTEN__)
#define HC_GLOBAL __attribute__((used))
#else
#define HC_GLOBAL
#pragma message("WARNING: Unknown platform when building hydra core")
#endif
#define HC_API HC_EXTERN_C HC_GLOBAL

namespace hydra
{

    constexpr uint32_t TOUCH_RELEASED = 0xFFFFFFFF;
    constexpr uint32_t BAD_CHEAT = 0xFFFFFFFF;

    /// Some things we want from type_traits for compile-time type checking, but we don't want to
    /// include the whole thing
    namespace type_traits
    {
        struct true_type
        {
            static constexpr bool value = true;

            operator bool() const noexcept
            {
                return value;
            }
        };

        struct false_type
        {
            static constexpr bool value = false;

            operator bool() const noexcept
            {
                return value;
            }
        };

        template <typename B>
        ::hydra::type_traits::true_type test_ptr_conv(const volatile B*);
        template <typename>
        ::hydra::type_traits::false_type test_ptr_conv(const volatile void*);

        template <typename B, typename D>
        auto test_is_base_of(int) -> decltype(test_ptr_conv<B>(static_cast<D*>(nullptr)));
        template <typename, typename>
        auto test_is_base_of(...) -> ::hydra::type_traits::true_type;

        template <typename Base, typename Derived>
        struct is_base_of
        {
            static constexpr bool value =
                decltype(::hydra::type_traits::test_is_base_of<Base, Derived>(0))::value;
        };

        template <class T>
        struct remove_pointer
        {
            typedef T type;
        };

        template <class T>
        struct remove_pointer<T*>
        {
            typedef T type;
        };
    } // namespace type_traits

    typedef struct
    {
        uint32_t width;
        uint32_t height;
    } Size;

    typedef struct
    {
        uint8_t* data;
        size_t size;
    } SaveState;

    enum struct SampleType
    {
        Int16 = 2,
        Float = 4,
    };

    enum struct ChannelType
    {
        Mono = 1,
        Stereo = 2,
    };

    enum struct ButtonType
    {
        Keypad1Up,
        Keypad1Down,
        Keypad1Left,
        Keypad1Right,
        Keypad2Up,
        Keypad2Down,
        Keypad2Left,
        Keypad2Right,
        A,
        B,
        X,
        Y,
        Z,
        L1,
        R1,
        L2,
        R2,
        L3,
        R3,
        Start,
        Select,
        Touch,
        Analog1Up,
        Analog1Down,
        Analog1Left,
        Analog1Right,
        Analog2Up,
        Analog2Down,
        Analog2Left,
        Analog2Right,

        InputCount,
    };

    enum class InfoType
    {
        CoreName,
        SystemName,
        Description,
        Author,
        Version,
        License,
        Website,
        Extensions, // Comma separated list of extensions
        Settings,   // TOML format to auto-generate core specific settings on runtime
                    // Read TOML.md for more information
        IconData, // Raw pixels in 32 bit RGBA format - you can convert images to this format using
                  // the imagemagick command: `convert my_image.png -depth 8 RGBA:output.raw`
        IconWidth,
        IconHeight,
    };

    enum class PixelFormat
    {
        RGBA,
        BGRA,
    };

#define X_HYDRA_INTERFACES               \
    X_HYDRA_INTERFACE(IBase)             \
    X_HYDRA_INTERFACE(IFrontendDriven)   \
    X_HYDRA_INTERFACE(ISelfDriven)       \
    X_HYDRA_INTERFACE(ISoftwareRendered) \
    X_HYDRA_INTERFACE(IOpenGlRendered)   \
    X_HYDRA_INTERFACE(IAudio)            \
    X_HYDRA_INTERFACE(IInput)            \
    X_HYDRA_INTERFACE(ISaveState)        \
    X_HYDRA_INTERFACE(IMultiplayer)      \
    X_HYDRA_INTERFACE(IReadableMemory)   \
    X_HYDRA_INTERFACE(IRewind)           \
    X_HYDRA_INTERFACE(ICheat)            \
    X_HYDRA_INTERFACE(IConfigurable)

#define X_HYDRA_INTERFACE(name) struct name;
    X_HYDRA_INTERFACES
#undef X_HYDRA_INTERFACE

    enum class InterfaceType
    {
#define X_HYDRA_INTERFACE(name) name,
        X_HYDRA_INTERFACES
#undef X_HYDRA_INTERFACE
            InterfaceCount
    };

    // The base emulator interface, every emulator core inherits this
    struct HC_GLOBAL IBase
    {
        virtual ~IBase() = default;
        /**
            Load a file into the emulator

            @param type The type of file to load, either "rom" or one of the firmware specified in
           getInfo
            @param path The absolute path to the file
            @return True if the file was loaded successfully, false otherwise
        */
        virtual bool loadFile(const char* type, const char* path) = 0;
        // Reset the emulator
        virtual void reset() = 0;
        // Get the native resolution of the console in pixels
        virtual hydra::Size getNativeSize() = 0;
        // Set an output size hint for the emulator for the desired host resolution, the emulator
        // may ignore this
        virtual void setOutputSize(hydra::Size size) = 0;
        // Used by the frontend to check if the emulator has a certain interface
        virtual bool hasInterface(InterfaceType interface) = 0;
#define X_HYDRA_INTERFACE(name) virtual name* as##name() = 0;
        X_HYDRA_INTERFACES
#undef X_HYDRA_INTERFACE

    private:
        /// This function is defined by HYDRA_CORE macro, so when it's not defined the compiler will
        /// throw an error
        virtual void youForgotToAddHydraCoreMacroToYourClass() = 0;
    };

    /// The frontend driven emulator interface, the frontend is responsible for calling runFrame()
    /// every frame
    struct HC_GLOBAL IFrontendDriven
    {
        virtual ~IFrontendDriven() = default;

        // Emulate one frame
        virtual void runFrame() = 0;

        // Returns the target FPS
        virtual uint16_t getFps() = 0;
    };

    // Not yet designed - do not use
    struct HC_GLOBAL ISelfDriven
    {
        virtual ~ISelfDriven() = default;
    };

    // The software rendered emulator interface, software rendered emulators inherit this
    struct HC_GLOBAL ISoftwareRendered
    {
        virtual ~ISoftwareRendered() = default;

        // Sets the callback that the emulator must call every frame to render the frame
        virtual void setVideoCallback(void (*callback)(void* data, hydra::Size size)) = 0;

        virtual hydra::PixelFormat getPixelFormat()
        {
            return hydra::PixelFormat::RGBA;
        }
    };

    // The OpenGL rendered emulator interface, OpenGL rendered emulators inherit this
    struct HC_GLOBAL IOpenGlRendered
    {
        virtual ~IOpenGlRendered() = default;

        // Called when the OpenGL context is created or recreated
        virtual void resetContext() = 0;

        // Called when the OpenGL context is destroyed so that the emulator can clean up
        virtual void destroyContext() = 0;

        // Called every frame to set the current framebuffer object the emulator must render to
        virtual void setFbo(unsigned handle) = 0;

        // Called before the first resetContext call to set the getProcAddress function
        virtual void setGetProcAddress(void* function) = 0;
    };

    // Not yet designed - do not use
    struct HC_GLOBAL IVulkanRendered
    {
        virtual ~IVulkanRendered() = default;
    };

    // The audio interface, emulators that support audio inherit this
    struct HC_GLOBAL IAudio
    {
        virtual ~IAudio() = default;

        virtual SampleType getSampleType()
        {
            return SampleType::Int16;
        }

        virtual ChannelType getChannelType()
        {
            return ChannelType::Stereo;
        }

        virtual uint32_t getSampleRate() = 0;
        virtual void setAudioCallback(void (*callback)(void* data, size_t size)) = 0;
    };

    // The input interface, emulators that support input inherit this
    struct HC_GLOBAL IInput
    {
        virtual ~IInput() = default;

        // Set the pollInputCallback, this is called whenever the emulator needs to poll input
        virtual void setPollInputCallback(void (*callback)()) = 0;

        // Set the getButtonCallback, this is called whenever the emulator needs to get the state of
        // a button
        virtual void setCheckButtonCallback(int32_t (*callback)(uint32_t player,
                                                                ButtonType button)) = 0;
    };

    // Save state interface, emulators that support save states inherit this
    struct HC_GLOBAL ISaveState
    {
        virtual ~ISaveState() = default;

        // Save the emulator state to a buffer
        virtual SaveState saveState() = 0;

        // Load the emulator state from a buffer
        virtual bool loadState(SaveState state) = 0;
    };

    // Multiple player interface, emulators that support multiple players inherit this
    struct HC_GLOBAL IMultiplayer
    {
        virtual ~IMultiplayer() = default;

        // Activate a player, this is called when the frontend wants to start using a player
        virtual void activatePlayer(uint32_t player) = 0;

        // Deactivate a player, this is called when the frontend wants to stop using a player
        virtual void deactivatePlayer(uint32_t player) = 0;

        // Returns the minimum number of players the system supports
        virtual uint32_t getMinimumPlayerCount() = 0;

        // Returns the maximum number of players the system supports
        virtual uint32_t getMaximumPlayerCount() = 0;
    };

    // Readable memory interface, emulators that support the frontend reading their memory inherit
    // this. This will be used for debugging purposes and for RetroAchievements
    struct HC_GLOBAL IReadableMemory
    {
        virtual ~IReadableMemory() = default;

        /**
            Read memory from the emulator

            @param address The address to read from, in little endian
            @param address_size The size of the address in bytes
            @param buffer The buffer to read into, must be at least num_bytes big
            @param num_bytes The number of bytes to read
        */
        virtual void readMemory(void* address, uint32_t address_size, uint8_t* buffer,
                                uint32_t num_bytes) = 0;
    };

    // Rewind interface, emulators that support rewinding inherit this
    struct HC_GLOBAL IRewind
    {
        virtual ~IRewind() = default;

        // Rewind the emulator by one frame
        virtual void rewindFrame() = 0;

        // Returns the maximum number of frames that can be rewound
        virtual uint32_t getRewindFrameCount() = 0;

        // Returns true if the rewind frame count was set successfully, false otherwise
        virtual bool setRewindFrameCount(uint32_t count) = 0;
    };

    // Cheats interface, emulators that support cheats inherit this
    struct HC_GLOBAL ICheat
    {
        virtual ~ICheat() = default;

        /**
            Add a cheat to the emulator. The enabled state of the cheat is suggested to be false by
           default, but the frontend will disable it immediately after adding it anyway.

            @param code The cheat code as a byte array, in big endian
            @param size The size of the cheat code in bytes
            @return The id of the cheat, or hydra::BAD_CHEAT (0xFFFFFFFF) if the cheat could not be
           added
        */
        virtual uint32_t addCheat(const uint8_t* code, uint32_t size) = 0;
        /**
            Remove a cheat from the emulator

            @param id The id of the cheat to remove
        */
        virtual void removeCheat(uint32_t id) = 0;
        /**
            Enable a cheat

            @param id The id of the cheat to enable
        */
        virtual void enableCheat(uint32_t id) = 0;
        /**
            Disable a cheat

            @param id The id of the cheat to disable
        */
        virtual void disableCheat(uint32_t id) = 0;
    };

    struct HC_GLOBAL IConfigurable
    {
        virtual ~IConfigurable() = default;

        virtual void setGetCallback(const char* (*callback)(const char* key)) = 0;
        virtual void setSetCallback(void (*callback)(const char* key, const char* value)) = 0;
    };

    /// Create an emulator and return a base interface pointer
    HC_API IBase* createEmulator();
    /// Destroy an emulator using a base interface pointer
    HC_API void destroyEmulator(IBase* emulator);
    /// Get info about the emulator, used if the info is not already cached
    HC_API const char* getInfo(InfoType type);

/// The HYDRA_CLASS macro must be included in every emulator core class in order to
/// implement part of the IBase interface without needing to write boilerplate code
#define HYDRA_CLASS                                                                     \
                                                                                        \
public:                                                                                 \
    bool hasInterface(::hydra::InterfaceType interface) override                        \
    {                                                                                   \
        switch (interface)                                                              \
        {                                                                               \
            case ::hydra::InterfaceType::IBase:                                         \
            {                                                                           \
                return ::hydra::type_traits::is_base_of<                                \
                    hydra::IBase,                                                       \
                    ::hydra::type_traits::remove_pointer<decltype(this)>::type>::value; \
            }                                                                           \
            case ::hydra::InterfaceType::IFrontendDriven:                               \
            {                                                                           \
                return ::hydra::type_traits::is_base_of<                                \
                    hydra::IFrontendDriven,                                             \
                    ::hydra::type_traits::remove_pointer<decltype(this)>::type>::value; \
            }                                                                           \
            case ::hydra::InterfaceType::ISelfDriven:                                   \
            {                                                                           \
                return ::hydra::type_traits::is_base_of<                                \
                    hydra::ISelfDriven,                                                 \
                    ::hydra::type_traits::remove_pointer<decltype(this)>::type>::value; \
            }                                                                           \
            case ::hydra::InterfaceType::ISoftwareRendered:                             \
            {                                                                           \
                return ::hydra::type_traits::is_base_of<                                \
                    hydra::ISoftwareRendered,                                           \
                    ::hydra::type_traits::remove_pointer<decltype(this)>::type>::value; \
            }                                                                           \
            case ::hydra::InterfaceType::IOpenGlRendered:                               \
            {                                                                           \
                return ::hydra::type_traits::is_base_of<                                \
                    hydra::IOpenGlRendered,                                             \
                    ::hydra::type_traits::remove_pointer<decltype(this)>::type>::value; \
            }                                                                           \
            case ::hydra::InterfaceType::IAudio:                                        \
            {                                                                           \
                return ::hydra::type_traits::is_base_of<                                \
                    hydra::IAudio,                                                      \
                    ::hydra::type_traits::remove_pointer<decltype(this)>::type>::value; \
            }                                                                           \
            case ::hydra::InterfaceType::IInput:                                        \
            {                                                                           \
                return ::hydra::type_traits::is_base_of<                                \
                    hydra::IInput,                                                      \
                    ::hydra::type_traits::remove_pointer<decltype(this)>::type>::value; \
            }                                                                           \
            case ::hydra::InterfaceType::ISaveState:                                    \
            {                                                                           \
                return ::hydra::type_traits::is_base_of<                                \
                    hydra::ISaveState,                                                  \
                    ::hydra::type_traits::remove_pointer<decltype(this)>::type>::value; \
            }                                                                           \
            case ::hydra::InterfaceType::IMultiplayer:                                  \
            {                                                                           \
                return ::hydra::type_traits::is_base_of<                                \
                    hydra::IMultiplayer,                                                \
                    ::hydra::type_traits::remove_pointer<decltype(this)>::type>::value; \
            }                                                                           \
            case ::hydra::InterfaceType::IReadableMemory:                               \
            {                                                                           \
                return ::hydra::type_traits::is_base_of<                                \
                    hydra::IReadableMemory,                                             \
                    ::hydra::type_traits::remove_pointer<decltype(this)>::type>::value; \
            }                                                                           \
            case ::hydra::InterfaceType::IRewind:                                       \
            {                                                                           \
                return ::hydra::type_traits::is_base_of<                                \
                    hydra::IRewind,                                                     \
                    ::hydra::type_traits::remove_pointer<decltype(this)>::type>::value; \
            }                                                                           \
            case ::hydra::InterfaceType::ICheat:                                        \
            {                                                                           \
                return ::hydra::type_traits::is_base_of<                                \
                    hydra::ICheat,                                                      \
                    ::hydra::type_traits::remove_pointer<decltype(this)>::type>::value; \
            }                                                                           \
            case ::hydra::InterfaceType::IConfigurable:                                 \
            {                                                                           \
                return ::hydra::type_traits::is_base_of<                                \
                    hydra::IConfigurable,                                               \
                    ::hydra::type_traits::remove_pointer<decltype(this)>::type>::value; \
            }                                                                           \
            default:                                                                    \
                return false;                                                           \
        }                                                                               \
    }                                                                                   \
    ::hydra::IBase* asIBase() override                                                  \
    {                                                                                   \
        if (hasInterface(::hydra::InterfaceType::IBase))                                \
        {                                                                               \
            return (::hydra::IBase*)(this);                                             \
        }                                                                               \
        return nullptr;                                                                 \
    }                                                                                   \
    ::hydra::IFrontendDriven* asIFrontendDriven() override                              \
    {                                                                                   \
        if (hasInterface(::hydra::InterfaceType::IFrontendDriven))                      \
        {                                                                               \
            return (::hydra::IFrontendDriven*)(this);                                   \
        }                                                                               \
        return nullptr;                                                                 \
    }                                                                                   \
    ::hydra::ISelfDriven* asISelfDriven() override                                      \
    {                                                                                   \
        if (hasInterface(::hydra::InterfaceType::ISelfDriven))                          \
        {                                                                               \
            return (::hydra::ISelfDriven*)(this);                                       \
        }                                                                               \
        return nullptr;                                                                 \
    }                                                                                   \
    ::hydra::ISoftwareRendered* asISoftwareRendered() override                          \
    {                                                                                   \
        if (hasInterface(::hydra::InterfaceType::ISoftwareRendered))                    \
        {                                                                               \
            return (::hydra::ISoftwareRendered*)(this);                                 \
        }                                                                               \
        return nullptr;                                                                 \
    }                                                                                   \
    ::hydra::IOpenGlRendered* asIOpenGlRendered() override                              \
    {                                                                                   \
        if (hasInterface(::hydra::InterfaceType::IOpenGlRendered))                      \
        {                                                                               \
            return (::hydra::IOpenGlRendered*)(this);                                   \
        }                                                                               \
        return nullptr;                                                                 \
    }                                                                                   \
    ::hydra::IAudio* asIAudio() override                                                \
    {                                                                                   \
        if (hasInterface(::hydra::InterfaceType::IAudio))                               \
        {                                                                               \
            return (::hydra::IAudio*)(this);                                            \
        }                                                                               \
        return nullptr;                                                                 \
    }                                                                                   \
    ::hydra::IInput* asIInput() override                                                \
    {                                                                                   \
        if (hasInterface(::hydra::InterfaceType::IInput))                               \
        {                                                                               \
            return (::hydra::IInput*)(this);                                            \
        }                                                                               \
        return nullptr;                                                                 \
    }                                                                                   \
    ::hydra::ISaveState* asISaveState() override                                        \
    {                                                                                   \
        if (hasInterface(::hydra::InterfaceType::ISaveState))                           \
        {                                                                               \
            return (::hydra::ISaveState*)(this);                                        \
        }                                                                               \
        return nullptr;                                                                 \
    }                                                                                   \
    ::hydra::IMultiplayer* asIMultiplayer() override                                    \
    {                                                                                   \
        if (hasInterface(::hydra::InterfaceType::IMultiplayer))                         \
        {                                                                               \
            return (::hydra::IMultiplayer*)(this);                                      \
        }                                                                               \
        return nullptr;                                                                 \
    }                                                                                   \
    ::hydra::IReadableMemory* asIReadableMemory() override                              \
    {                                                                                   \
        if (hasInterface(::hydra::InterfaceType::IReadableMemory))                      \
        {                                                                               \
            return (::hydra::IReadableMemory*)(this);                                   \
        }                                                                               \
        return nullptr;                                                                 \
    }                                                                                   \
    ::hydra::IRewind* asIRewind() override                                              \
    {                                                                                   \
        if (hasInterface(::hydra::InterfaceType::IRewind))                              \
        {                                                                               \
            return (::hydra::IRewind*)(this);                                           \
        }                                                                               \
        return nullptr;                                                                 \
    }                                                                                   \
    ::hydra::ICheat* asICheat() override                                                \
    {                                                                                   \
        if (hasInterface(::hydra::InterfaceType::ICheat))                               \
        {                                                                               \
            return (::hydra::ICheat*)(this);                                            \
        }                                                                               \
        return nullptr;                                                                 \
    }                                                                                   \
    ::hydra::IConfigurable* asIConfigurable() override                                  \
    {                                                                                   \
        if (hasInterface(::hydra::InterfaceType::IConfigurable))                        \
        {                                                                               \
            return (::hydra::IConfigurable*)(this);                                     \
        }                                                                               \
        return nullptr;                                                                 \
    }                                                                                   \
                                                                                        \
private:                                                                                \
    void youForgotToAddHydraCoreMacroToYourClass() override {}

    // Macros to generate HYDRA_CLASS if new interfaces are added

    // #define X_HYDRA_INTERFACE(name) case ::hydra::InterfaceType::name: { return
    // ::hydra::type_traits::is_base_of<hydra::name,
    // ::hydra::type_traits::remove_pointer<decltype(this)>::type>::value; } public:
    //     bool hasInterface(::hydra::InterfaceType interface) override {
    //         switch (interface) {
    //             X_HYDRA_INTERFACES
    //         }
    //     }
    // private:
    //     void youForgotToAddHydraCoreMacroToYourClass() override {}
    // #define X_HYDRA_INTERFACE(name) ::hydra::name* as##name() override { if
    // (hasInterface(::hydra::InterfaceType::name)) { return (::hydra::name*)(this); } return
    // nullptr; } X_HYDRA_INTERFACES
} // namespace hydra
