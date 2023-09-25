#pragma once

#include <array>
#include <cfenv>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <common/compatibility.hxx>
#include <core/n64_log.hxx>
#include <concepts>
#include <core/n64_addresses.hxx>
#include <core/n64_rcp.hxx>
#include <core/n64_types.hxx>
#include <cstdint>
#include <limits>
#include <memory>
#include <queue>
#include <vector>

#define KB(x) (static_cast<size_t>(x << 10))
#define check_bit(x, y) ((x) & (1u << y))

constexpr auto INSTRS_PER_SECOND = 93'750'000;
constexpr uint32_t KSEG0_START = 0x8000'0000;
constexpr uint32_t KSEG0_END = 0x9FFF'FFFF;
constexpr uint32_t KSEG1_START = 0xA000'0000;
constexpr uint32_t KSEG1_END = 0xBFFF'FFFF;

enum class ExceptionType
{
    Interrupt = 0,
    TLBMissLoad = 2,
    AddressErrorLoad = 4,
    AddressErrorStore = 5,
    Syscall = 8,
    Breakpoint = 9,
    ReservedInstruction = 10,
    CoprocessorUnusable = 11,
    IntegerOverflow = 12,
    Trap = 13,
    FloatingPoint = 15,
};

#define X(name, value) constexpr auto CP0_##name = value;
X(INDEX, 0)
X(RANDOM, 1)
X(ENTRYLO0, 2)
X(ENTRYLO1, 3)
X(CONTEXT, 4)
X(PAGEMASK, 5)
X(WIRED, 6)
X(BADVADDR, 8)
X(COUNT, 9)
X(ENTRYHI, 10)
X(COMPARE, 11)
X(STATUS, 12)
X(CAUSE, 13)
X(EPC, 14)
X(PRID, 15)
X(CONFIG, 16)
X(LLADDR, 17)
X(WATCHLO, 18)
X(WATCHHI, 19)
X(XCONTEXT, 20)
X(PARITYERROR, 26)
X(CACHEERROR, 27)
X(TAGLO, 28)
X(TAGHI, 29)
X(ERROREPC, 30)
#undef X

// TODO: endianess issues, switch to BitField< ... >
union CP0StatusType
{
    struct
    {
        uint64_t IE  : 1;
        uint64_t EXL : 1;
        uint64_t ERL : 1;
        uint64_t KSU : 2;
        uint64_t UX  : 1;
        uint64_t SX  : 1;
        uint64_t KX  : 1;
        uint64_t IM  : 8;
        uint64_t DS  : 9;
        uint64_t RE  : 1;
        uint64_t FR  : 1;
        uint64_t RP  : 1;
        uint64_t CP0 : 1;
        uint64_t CP1 : 1;
        uint64_t CP2 : 1;
        uint64_t CP3 : 1;
        uint64_t     : 32;
    };

    uint64_t full;
};

static_assert(sizeof(CP0StatusType) == sizeof(uint64_t));
#define CP0Status (reinterpret_cast<CP0StatusType&>(cp0_regs_[CP0_STATUS]))

union CP0CauseType
{
    struct
    {
        uint64_t unused1 : 2;
        uint64_t ExCode  : 5;
        uint64_t unused2 : 1;
        uint64_t IP0     : 1;
        uint64_t IP1     : 1;
        uint64_t IP2     : 1;
        uint64_t IP3     : 1;
        uint64_t IP4     : 1;
        uint64_t IP5     : 1;
        uint64_t IP6     : 1;
        uint64_t IP7     : 1;
        uint64_t unused3 : 12;
        uint64_t CE      : 2;
        uint64_t unused4 : 1;
        uint64_t BD      : 1;
        uint64_t unused5 : 32;
    };

    uint64_t full;
};

static_assert(sizeof(CP0CauseType) == sizeof(uint64_t));
#define CP0Cause (reinterpret_cast<CP0CauseType&>(cp0_regs_[CP0_CAUSE]))

union CP0ContextType
{
    struct
    {
        uint64_t         : 4;
        uint64_t BadVPN2 : 19;
        uint64_t PTEBase : 41;
    };

    uint64_t full;
};

static_assert(sizeof(CP0ContextType) == sizeof(uint64_t));
#define CP0Context (reinterpret_cast<CP0ContextType&>(cp0_regs_[CP0_CONTEXT]))

union CP0XContextType
{
    struct
    {
        uint64_t         : 4;
        uint64_t BadVPN2 : 27;
        uint64_t R       : 2;
        uint64_t PTEBase : 31;
    };

    uint64_t full;
};

static_assert(sizeof(CP0XContextType) == sizeof(uint64_t));
#define CP0XContext (reinterpret_cast<CP0XContextType&>(cp0_regs_[CP0_XCONTEXT]))
#define CP0EntryHi (reinterpret_cast<EntryHi&>(cp0_regs_[CP0_ENTRYHI]))

namespace hydra
{
    namespace N64
    {
        class N64;
        class QA;
    } // namespace N64
} // namespace hydra

namespace hydra::N64
{
    enum class ControllerType : uint16_t
    {
        Joypad = 0x0500,
        Mouse = 0x0200,
    };

    namespace Keys
    {
        enum
        {
            A = 0,
            B,
            Z,
            Start,
            AnalogVertical,
            AnalogHorizontal,
            L,
            R,
            CUp,
            CDown,
            CLeft,
            CRight,
            KeypadUp,
            KeypadDown,
            KeypadLeft,
            KeypadRight,

            N64KeyCount,
            ErrorKey = 0xFFFF'FFFF
        };
    }

    enum class CP0Instruction
    {
        TLBWI = 2,
        TLBP = 8,
        TLBR = 1,
        ERET = 24,
        WAIT = 17,
        TLBWR = 6,
    };

    // Bit hack to get signum of number (-1, 0 or 1)
    template <typename T>
    int sgn(T val)
    {
        return (T(0) < val) - (val < T(0));
    }

    constexpr static uint64_t OperationMasks[2] = {std::numeric_limits<uint32_t>::max(),
                                                   std::numeric_limits<uint64_t>::max()};
    // Only kernel mode is used for (most?) n64 licensed games
    enum class OperatingMode
    {
        User,
        Supervisor,
        Kernel
    };

    struct TranslatedAddress
    {
        uint32_t paddr;
        bool cached;
        bool success = false;
    };

    /**
        32-bit address bus

        @see https://n64brew.dev/wiki/Memory_map
    */
    class CPUBus
    {
    public:
        CPUBus(RCP& rcp);
        bool LoadCartridge(std::string path);
        bool LoadIPL(std::string path);

        bool IsEverythingLoaded()
        {
            return rom_loaded_ && ipl_loaded_;
        }

        void Reset();

    private:
        hydra_inline uint8_t* redirect_paddress(uint32_t paddr);
        void map_direct_addresses();

        static std::vector<uint8_t> ipl_;
        std::vector<uint8_t> cart_rom_;
        bool rom_loaded_ = false;
        bool ipl_loaded_ = false;
        std::vector<uint8_t> rdram_{};
        std::vector<uint8_t> sram_{};
        std::array<char, ISVIEWER_AREA_END - ISVIEWER_AREA_START> isviewer_buffer_{};
        std::array<uint8_t, 64> pif_ram_{};
        std::array<uint8_t*, 0x10000> page_table_{};

        // MIPS Interface
        uint32_t mi_mode_ = 0;
        uint32_t mi_version_ = 0;
        MIInterrupt mi_interrupt_{};
        uint32_t mi_mask_ = 0;

        // Peripheral Interface
        uint32_t pi_dram_addr_ = 0;
        uint32_t pi_cart_addr_ = 0;
        uint32_t pi_rd_len_ = 0;
        uint32_t pi_wr_len_ = 0;
        uint32_t pi_status_ = 0;
        bool dma_error_ = false;
        bool io_busy_ = false;
        bool dma_busy_ = false;
        uint32_t pi_bsd_dom1_lat_ = 0;
        uint32_t pi_bsd_dom1_pwd_ = 0;
        uint32_t pi_bsd_dom1_pgs_ = 0;
        uint32_t pi_bsd_dom1_rls_ = 0;
        uint32_t pi_bsd_dom2_lat_ = 0;
        uint32_t pi_bsd_dom2_pwd_ = 0;
        uint32_t pi_bsd_dom2_pgs_ = 0;
        uint32_t pi_bsd_dom2_rls_ = 0;

        // RDRAM Interface
        uint32_t ri_mode_ = 0;
        uint32_t ri_config_ = 0;
        uint32_t ri_current_load_ = 0;
        uint32_t ri_select_ = 0;
        uint32_t ri_refresh_ = 0;
        uint32_t ri_latency_ = 0;

        // Serial Interface
        uint32_t si_dram_addr_ = 0;
        uint32_t si_pif_ad_wr64b_ = 0;
        uint32_t si_pif_ad_rd64b_ = 0;
        uint32_t si_status_ = 0;

        uint64_t time_ = 0;

        RCP& rcp_;
        friend class CPU;
        friend class hydra::N64::N64;
    };

    template <auto MemberFunc>
    static void lut_wrapper(CPU* cpu)
    {
        // Props: calc84maniac
        // > making it into a template parameter lets the compiler avoid using an actual member
        // function pointer at runtime
        (cpu->*MemberFunc)();
    }

    enum class InterruptType
    {
        VI,
        AI,
        PI,
        SI,
        DP,
        SP
    };

    class CPU final
    {
    public:
        CPU(CPUBus& cpubus, RCP& rcp);
        void Tick();
        void Reset();

    private:
        using PipelineStageRet = void;
        using PipelineStageArgs = void;
        CPUBus& cpubus_;
        RCP& rcp_;

        OperatingMode opmode_ = OperatingMode::Kernel;
        // To be used with OpcodeMasks (OpcodeMasks[mode64_])
        bool mode64_ = false;
        /// Registers
        // r0 is hardwired to 0, r31 is the link register
        std::array<MemDataUnionDW, 32> gpr_regs_;
        std::array<MemDataUnionDW, 32> fpr_regs_;
        std::array<MemDataUnionDW, 32> cp0_regs_;
        std::array<TLBEntry, 32> tlb_;
        uint64_t temp;
        // CPU cache
        std::vector<uint8_t> instr_cache_;
        std::vector<uint8_t> data_cache_;
        // Special registers
        uint64_t prev_pc_, pc_, next_pc_, hi_, lo_;
        bool llbit_;
        uint32_t lladdr_;
        uint64_t fcr0_;
        Instruction instruction_;
        FCR31 fcr31_;
        uint32_t cp0_weirdness_;
        uint64_t cp2_weirdness_;
        bool prev_branch_ = false, was_branch_ = false;
        uint32_t tlb_offset_mask_ = 0;
        int pif_channel_ = 0;
        int vis_per_second_ = 0;
        ControllerType controller_type_ = ControllerType::Joypad;
        int32_t mouse_x_, mouse_y_;
        int32_t mouse_delta_x_, mouse_delta_y_;
        std::chrono::time_point<std::chrono::high_resolution_clock> last_second_time_;
        bool should_service_interrupt_ = false;

        hydra_inline TranslatedAddress translate_vaddr(uint32_t vaddr);
        hydra_inline TranslatedAddress translate_vaddr_kernel(uint32_t vaddr);
        hydra_inline TranslatedAddress probe_tlb(uint32_t vaddr);

        uint32_t read_hwio(uint32_t addr);
        void write_hwio(uint32_t addr, uint32_t data);

        // clang-format off
        void SPECIAL(), REGIMM(), J(), JAL(), BEQ(), BNE(), BLEZ(), BGTZ(),
            ADDI(), ADDIU(), SLTI(), SLTIU(), ANDI(), ORI(), XORI(), LUI(),
            COP0(), COP1(), COP2(), COP3(), BEQL(), BNEL(), BLEZL(), BGTZL(),
            DADDI(), DADDIU(), LDL(), LDR(), ERROR(),
            LB(), LH(), LWL(), LW(), LBU(), LHU(), LWR(), LWU(),
            SB(), SH(), SWL(), SW(), SDL(), SDR(), SWR(), CACHE(),
            LL(), LWC1(), LWC2(), LLD(), LDC1(), LDC2(), LD(),
            SC(), SWC1(), SWC2(), SCD(), SDC1(), SDC2(), SD(),
            MTC0(), DMTC0(), MFC0(), DMFC0();

        void s_SLL(), s_SRL(), s_SRA(), s_SLLV(), s_SRLV(), s_SRAV(),
            s_JR(), s_JALR(), s_SYSCALL(), s_BREAK(), s_SYNC(),
            s_MFHI(), s_MTHI(), s_MFLO(), s_MTLO(), s_DSLLV(), s_DSRLV(), s_DSRAV(),
            s_MULT(), s_MULTU(), s_DIV(), s_DIVU(), s_DMULT(), s_DMULTU(), s_DDIV(), s_DDIVU(),
            s_ADD(), s_ADDU(), s_SUB(), s_SUBU(), s_AND(), s_OR(), s_XOR(), s_NOR(),
            s_SLT(), s_SLTU(), s_DADD(), s_DADDU(), s_DSUB(), s_DSUBU(),
            s_TGE(), s_TGEU(), s_TLT(), s_TLTU(), s_TEQ(), s_TNE(),
            s_DSLL(), s_DSRL(), s_DSRA(), s_DSLL32(), s_DSRL32(), s_DSRA32();

        void r_BLTZ(), r_BGEZ(), r_BLTZL(), r_BGEZL(),
            r_TGEI(), r_TGEIU(), r_TLTI(), r_TLTIU(), r_TEQI(), r_TNEI(),
            r_BLTZAL(), r_BGEZAL(), r_BLTZALL(), r_BGEZALL();

        void f_ADD(), f_SUB(), f_MUL(), f_DIV(), f_SQRT(), f_ABS(), f_MOV(), f_NEG(),
            f_ROUNDL(), f_TRUNCL(), f_CEILL(), f_FLOORL(), f_ROUNDW(), f_TRUNCW(), f_CEILW(), f_FLOORW(),
            f_CVTS(), f_CVTD(), f_CVTW(), f_CVTL(), f_CF(), f_CUN(),
            f_CEQ(), f_CUEQ(), f_COLT(), f_CULT(), f_COLE(), f_CULE(),
            f_CSF(), f_CNGLE(), f_CSEQ(), f_CNGL(), f_CLT(), f_CNGE(), f_CLE(), f_CNGT(),
            f_CFC1(), f_MFC1(), f_DMFC1(), f_MTC1(), f_DMTC1(), f_CTC1();

        void MFC2(), DMFC2(), MTC2(), DMTC2(), CFC2(), CTC2(), RDHWR();

        using func_ptr = void (*)(CPU*);
        constexpr static std::array<func_ptr, 64> instruction_table_ =
        {
            &lut_wrapper<&CPU::SPECIAL>, &lut_wrapper<&CPU::REGIMM>, &lut_wrapper<&CPU::J>, &lut_wrapper<&CPU::JAL>, &lut_wrapper<&CPU::BEQ>, &lut_wrapper<&CPU::BNE>, &lut_wrapper<&CPU::BLEZ>, &lut_wrapper<&CPU::BGTZ>,
            &lut_wrapper<&CPU::ADDI>, &lut_wrapper<&CPU::ADDIU>, &lut_wrapper<&CPU::SLTI>, &lut_wrapper<&CPU::SLTIU>, &lut_wrapper<&CPU::ANDI>, &lut_wrapper<&CPU::ORI>, &lut_wrapper<&CPU::XORI>, &lut_wrapper<&CPU::LUI>,
            &lut_wrapper<&CPU::COP0>, &lut_wrapper<&CPU::COP1>, &lut_wrapper<&CPU::COP2>, &lut_wrapper<&CPU::COP3>, &lut_wrapper<&CPU::BEQL>, &lut_wrapper<&CPU::BNEL>, &lut_wrapper<&CPU::BLEZL>, &lut_wrapper<&CPU::BGTZL>,
            &lut_wrapper<&CPU::DADDI>, &lut_wrapper<&CPU::DADDIU>, &lut_wrapper<&CPU::LDL>, &lut_wrapper<&CPU::LDR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::RDHWR>,
            &lut_wrapper<&CPU::LB>, &lut_wrapper<&CPU::LH>, &lut_wrapper<&CPU::LWL>, &lut_wrapper<&CPU::LW>, &lut_wrapper<&CPU::LBU>, &lut_wrapper<&CPU::LHU>, &lut_wrapper<&CPU::LWR>, &lut_wrapper<&CPU::LWU>,
            &lut_wrapper<&CPU::SB>, &lut_wrapper<&CPU::SH>, &lut_wrapper<&CPU::SWL>, &lut_wrapper<&CPU::SW>, &lut_wrapper<&CPU::SDL>, &lut_wrapper<&CPU::SDR>, &lut_wrapper<&CPU::SWR>, &lut_wrapper<&CPU::CACHE>,
            &lut_wrapper<&CPU::LL>, &lut_wrapper<&CPU::LWC1>, &lut_wrapper<&CPU::LWC2>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::LLD>, &lut_wrapper<&CPU::LDC1>, &lut_wrapper<&CPU::LDC2>, &lut_wrapper<&CPU::LD>,
            &lut_wrapper<&CPU::SC>, &lut_wrapper<&CPU::SWC1>, &lut_wrapper<&CPU::SWC2>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::SCD>, &lut_wrapper<&CPU::SDC1>, &lut_wrapper<&CPU::SDC2>, &lut_wrapper<&CPU::SD>,
        };

        constexpr static std::array<func_ptr, 64> special_table_ =
        {
            &lut_wrapper<&CPU::s_SLL>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::s_SRL>, &lut_wrapper<&CPU::s_SRA>, &lut_wrapper<&CPU::s_SLLV>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::s_SRLV>, &lut_wrapper<&CPU::s_SRAV>,
            &lut_wrapper<&CPU::s_JR>, &lut_wrapper<&CPU::s_JALR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::s_SYSCALL>, &lut_wrapper<&CPU::s_BREAK>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::s_SYNC>,
            &lut_wrapper<&CPU::s_MFHI>, &lut_wrapper<&CPU::s_MTHI>, &lut_wrapper<&CPU::s_MFLO>, &lut_wrapper<&CPU::s_MTLO>, &lut_wrapper<&CPU::s_DSLLV>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::s_DSRLV>, &lut_wrapper<&CPU::s_DSRAV>,
            &lut_wrapper<&CPU::s_MULT>, &lut_wrapper<&CPU::s_MULTU>, &lut_wrapper<&CPU::s_DIV>, &lut_wrapper<&CPU::s_DIVU>, &lut_wrapper<&CPU::s_DMULT>, &lut_wrapper<&CPU::s_DMULTU>, &lut_wrapper<&CPU::s_DDIV>, &lut_wrapper<&CPU::s_DDIVU>,
            &lut_wrapper<&CPU::s_ADD>, &lut_wrapper<&CPU::s_ADDU>, &lut_wrapper<&CPU::s_SUB>, &lut_wrapper<&CPU::s_SUBU>, &lut_wrapper<&CPU::s_AND>, &lut_wrapper<&CPU::s_OR>, &lut_wrapper<&CPU::s_XOR>, &lut_wrapper<&CPU::s_NOR>,
            &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::s_SLT>, &lut_wrapper<&CPU::s_SLTU>, &lut_wrapper<&CPU::s_DADD>, &lut_wrapper<&CPU::s_DADDU>, &lut_wrapper<&CPU::s_DSUB>, &lut_wrapper<&CPU::s_DSUBU>,
            &lut_wrapper<&CPU::s_TGE>, &lut_wrapper<&CPU::s_TGEU>, &lut_wrapper<&CPU::s_TLT>, &lut_wrapper<&CPU::s_TLTU>, &lut_wrapper<&CPU::s_TEQ>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::s_TNE>, &lut_wrapper<&CPU::ERROR>,
            &lut_wrapper<&CPU::s_DSLL>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::s_DSRL>, &lut_wrapper<&CPU::s_DSRA>, &lut_wrapper<&CPU::s_DSLL32>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::s_DSRL32>, &lut_wrapper<&CPU::s_DSRA32>,
        };

        constexpr static std::array<func_ptr, 64> float_table_ =
        {
            &lut_wrapper<&CPU::f_ADD>, &lut_wrapper<&CPU::f_SUB>, &lut_wrapper<&CPU::f_MUL>, &lut_wrapper<&CPU::f_DIV>, &lut_wrapper<&CPU::f_SQRT>, &lut_wrapper<&CPU::f_ABS>, &lut_wrapper<&CPU::f_MOV>, &lut_wrapper<&CPU::f_NEG>,
            &lut_wrapper<&CPU::f_ROUNDL>, &lut_wrapper<&CPU::f_TRUNCL>, &lut_wrapper<&CPU::f_CEILL>, &lut_wrapper<&CPU::f_FLOORL>, &lut_wrapper<&CPU::f_ROUNDW>, &lut_wrapper<&CPU::f_TRUNCW>, &lut_wrapper<&CPU::f_CEILW>, &lut_wrapper<&CPU::f_FLOORW>,
            &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>,
            &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>,
            &lut_wrapper<&CPU::f_CVTS>, &lut_wrapper<&CPU::f_CVTD>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::f_CVTW>, &lut_wrapper<&CPU::f_CVTL>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>,
            &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>,
            &lut_wrapper<&CPU::f_CF>, &lut_wrapper<&CPU::f_CUN>, &lut_wrapper<&CPU::f_CEQ>, &lut_wrapper<&CPU::f_CUEQ>, &lut_wrapper<&CPU::f_COLT>, &lut_wrapper<&CPU::f_CULT>, &lut_wrapper<&CPU::f_COLE>, &lut_wrapper<&CPU::f_CULE>,
            &lut_wrapper<&CPU::f_CSF>, &lut_wrapper<&CPU::f_CNGLE>, &lut_wrapper<&CPU::f_CSEQ>, &lut_wrapper<&CPU::f_CNGL>, &lut_wrapper<&CPU::f_CLT>, &lut_wrapper<&CPU::f_CNGE>, &lut_wrapper<&CPU::f_CLE>, &lut_wrapper<&CPU::f_CNGT>,
        };

        constexpr static std::array<func_ptr, 32> regimm_table_ =
        {
            &lut_wrapper<&CPU::r_BLTZ>, &lut_wrapper<&CPU::r_BGEZ>, &lut_wrapper<&CPU::r_BLTZL>, &lut_wrapper<&CPU::r_BGEZL>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>,
            &lut_wrapper<&CPU::r_TGEI>, &lut_wrapper<&CPU::r_TGEIU>, &lut_wrapper<&CPU::r_TLTI>, &lut_wrapper<&CPU::r_TLTIU>, &lut_wrapper<&CPU::r_TEQI>,  &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::r_TNEI>,  &lut_wrapper<&CPU::ERROR>,
            &lut_wrapper<&CPU::r_BLTZAL>, &lut_wrapper<&CPU::r_BGEZAL>, &lut_wrapper<&CPU::r_BLTZALL>, &lut_wrapper<&CPU::r_BGEZALL>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>,
            &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>,
        };
        // clang-format on

        void execute_instruction();
        void execute_cp0_instruction();

        void conditional_branch(bool condition, uint64_t address);
        void conditional_branch_likely(bool condition, uint64_t address);
        void link_register(uint8_t reg);
        void branch_to(uint64_t address);

        uint8_t load_byte(uint64_t address);
        uint16_t load_halfword(uint64_t address);
        uint32_t load_word(uint64_t address);
        uint64_t load_doubleword(uint64_t address);
        void store_byte(uint64_t address, uint8_t value);
        void store_halfword(uint64_t address, uint16_t value);
        void store_word(uint64_t address, uint32_t value);
        void store_doubleword(uint64_t address, uint64_t value);

        bool check_interrupts();
        void update_interrupt_check();
        hydra_inline void set_interrupt(InterruptType type, bool value);
        void handle_event();
        uint32_t timing_pi_access(uint8_t domain, uint32_t length);
        void check_vi_interrupt();
        void throw_exception(uint32_t, ExceptionType, uint8_t = 0);
        uint32_t get_cp0_register_32(uint8_t reg);
        uint64_t get_cp0_register_64(uint8_t reg);
        void set_cp0_register_32(uint8_t reg, uint32_t value);
        void set_cp0_register_64(uint8_t reg, uint64_t value);
        void set_cp0_regs_exception(int64_t address);

        template <class T>
        T get_fpr_reg(int regnum);

        template <class T>
        bool check_nan(T arg);

        template <class T>
        T get_nan();

        template <class T>
        void set_fpr_reg(int regnum, T value);

        template <class T>
        void check_fpu_arg(T arg);

        template <class T>
        void check_fpu_result(T& arg);

        template <class OperatorFunction, class CastFunction>
        void fpu_operate(OperatorFunction op, CastFunction cast);

        template <class Type, class OperatorFunction, class CastFunction>
        void fpu_operate_impl(OperatorFunction op, CastFunction cast);

        template <class Type>
        Type get_fpu_reg(int regnum);

        template <class Type>
        void set_fpu_reg(int regnum, Type value);

        bool check_fpu_exception();

        void pif_command();
        bool joybus_command(const std::vector<uint8_t>&, std::vector<uint8_t>&);
        void get_controller_state(int player, std::vector<uint8_t>& result,
                                  ControllerType controller);
        std::vector<DisassemblerInstruction> disassemble(uint64_t start_vaddr, uint64_t end_vaddr,
                                                         bool register_names);

        template <bool DoLog>
        void log_cpu_state(bool use_crc, uint64_t instructions, uint64_t start = 0);

        bool is_kernel_mode();
        void dump_tlb();
        void dump_pif_ram();
        void dump_rdram();
        uint32_t get_dram_crc();

        std::function<void()> poll_input_callback_;
        std::function<int8_t(uint8_t, uint8_t)> read_input_callback_;

        friend class hydra::N64::N64;
    };
} // namespace hydra::N64
