#pragma once

#include <array>
#include <cerberus/common/compatibility.hxx>
#include <cerberus/common/log.hxx>
#include <cerberus/core/addresses.hxx>
#include <cerberus/core/rcp.hxx>
#include <cerberus/core/scheduler.hxx>
#include <cerberus/core/types.hxx>
#include <cfenv>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <queue>
#include <vector>

#include <hydra/core.hxx>

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
#define CP0Status (reinterpret_cast<CP0StatusType&>(cp0s[CP0_STATUS]))

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
#define CP0Cause (reinterpret_cast<CP0CauseType&>(cp0s[CP0_CAUSE]))

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
#define CP0Context (reinterpret_cast<CP0ContextType&>(cp0s[CP0_CONTEXT]))

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
#define CP0XContext (reinterpret_cast<CP0XContextType&>(cp0s[CP0_XCONTEXT]))
#define CP0EntryHi (reinterpret_cast<EntryHi&>(cp0s[CP0_ENTRYHI]))

namespace cerberus
{
    enum class ControllerType : uint16_t
    {
        Joypad = 0x0500,
        Mouse = 0x0200,
    };

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

    using PhysicalAddress = uint32_t;

    template <auto MemberFunc>
    static void lut_wrapper(CPU* cpu)
    {
        // Props: calc84maniac
        // > making it into a template parameter lets the compiler avoid using an
        // actual member function pointer at runtime
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
        CPU(Scheduler& scheduler, RCP& rcp);

        void run()
        {
            static int cycles = 0;
            for (int f = 0; f < 1; f++)
            { // fields
                for (int hl = 0; hl < rcp.vi.num_halflines_; hl++)
                { // halflines
                    rcp.vi.vi_v_current_ = (hl << 1) + f;
                    check_vi_interrupt();
                    while (cycles <= rcp.vi.cycles_per_halfline_)
                    {
                        static int cpu_cycles = 0;
                        cpu_cycles++;
                        Tick();
                        rcp.ai.Step();
                        if (!rcp.rsp.IsHalted())
                        {
                            while (cpu_cycles > 2)
                            {
                                rcp.rsp.Tick();
                                if (!rcp.rsp.IsHalted())
                                {
                                    rcp.rsp.Tick();
                                }
                                cpu_cycles -= 3;
                            }
                        }
                        else
                        {
                            cpu_cycles = 0;
                        }
                        cycles++;
                    }
                    cycles -= rcp.vi.cycles_per_halfline_;
                }
                check_vi_interrupt();
            }
        }

        void setReadInputCallback(int32_t (*callback)(uint32_t, hydra::ButtonType))
        {
            read_input_callback_ = callback;
        }

        void setPollInputCallback(void (*callback)())
        {
            poll_input_callback_ = callback;
        }

        inline void Tick()
        {
            ++cycleClock;
            cycleClock &= 0x1FFFFFFFF;
            if (cycleClock == (cp0s[CP0_COMPARE].UD << 1)) [[unlikely]]
            {
                CP0Cause.IP7 = true;
                update_interrupt_check();
            }
            gprs[0].UD = 0;
            prevBranch = wasBranch;
            wasBranch = false;
            PhysicalAddress paddr = translate_vaddr(Pc);
            uint8_t* ptr = redirect_paddress(paddr);
            instruction.full = hydra::bswap32(*reinterpret_cast<uint32_t*>(ptr));
            if (check_interrupts())
            {
                return;
            }
            // log_cpu_state<CPU_LOGGING>(true, 30'000'000, 0);
            prevPc = Pc;
            Pc = nextPc;
            nextPc += 4;
            (instruction_table_[instruction.IType.op])(this);
        }

        void reset();

        bool loadCartridge(const std::filesystem::path& path);
        bool loadIpl(const std::filesystem::path& path);

    private:
        RCP& rcp;
        Scheduler& scheduler;

        /// Registers
        // r0 is hardwired to 0, r31 is the link register
        std::array<MemDataUnionDW, 32> gprs;
        std::array<MemDataUnionDW, 32> fprs;
        std::array<MemDataUnionDW, 32> cp0s;
        std::array<TLBEntry, 32> tlbEntries;
        // Special registers
        uint64_t prevPc, Pc, nextPc, mulHi, mulLo;
        bool llbit_;      // TODO: refactor?
        uint32_t lladdr_; // TODO: refactor?
        Instruction instruction;
        FCR31 fcr31;
        uint32_t cp0_latch_;
        uint64_t cp2_latch_;
        bool prevBranch = false, wasBranch = false;
        int pifChannel = 0;
        ControllerType controllerType = ControllerType::Joypad;
        int32_t mouse_x_, mouse_y_;
        int32_t mouse_delta_x_, mouse_delta_y_;
        bool interruptPending = false;

        inline uint8_t* redirect_paddress(uint32_t paddr)
        {
            uint8_t* ptr = pageTable[paddr >> 16];
            if (ptr) [[likely]]
            {
                ptr += (paddr & static_cast<uint32_t>(0xFFFF));
                return ptr;
            }
            else if (paddr - 0x1FC00000u < 1984u)
            {
                return &iplData[paddr - 0x1FC00000u];
            }
            return nullptr;
        }

        void map_direct_addresses();

        std::vector<uint8_t> iplData;
        std::vector<uint8_t> cartridgeData;
        std::vector<uint8_t> rdram{};
        std::vector<char> isviewer{};
        std::array<uint8_t, 64> pifRam{};
        std::array<uint8_t*, 0x10000> pageTable{};

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

        uint64_t cycleClock = 0;

        inline PhysicalAddress translate_vaddr(uint32_t vaddr)
        {
            if (is_kernel_mode()) [[likely]]
            {
                return translate_vaddr_kernel(vaddr);
            }
            else
            {
                Logger::Fatal("Non kernel mode :(");
            }
            return {};
        }

        inline PhysicalAddress translate_vaddr_kernel(uint32_t addr)
        {
            if (addr >= 0x80000000 && addr <= 0xBFFFFFFF) [[likely]]
            {
                return addr & 0x1FFFFFFF;
            }
            else if (addr >= 0 && addr <= 0x7FFFFFFF)
            {
                // User segment
                PhysicalAddress paddr = probe_tlb(addr);
                return paddr;
            }
            else if (addr >= 0xC0000000 && addr <= 0xDFFFFFFF)
            {
                // Supervisor segment
                Logger::Warn("Accessing supervisor segment {:08x}", addr);
            }
            else
            {
                // Kernel segment TLB
                Logger::Warn("Accessing kernel segment {:08x}", addr);
            }
            return {};
        }

        inline PhysicalAddress probe_tlb(uint32_t vaddr)
        {
            for (const TLBEntry& entry : tlbEntries)
            {
                if (!entry.initialized)
                {
                    continue;
                }
                uint32_t vpn_mask = ~((entry.mask << 13) | 0x1FFF);
                uint64_t current_vpn = vaddr & vpn_mask;
                uint64_t have_vpn = (entry.entry_hi.VPN2 << 13) & vpn_mask;
                int current_asid = CP0EntryHi.ASID;
                bool global = entry.G;
                if ((have_vpn == current_vpn) && (global || (entry.entry_hi.ASID == current_asid)))
                {
                    uint32_t offset_mask = ((entry.mask << 12) | 0xFFF);
                    bool odd = vaddr & (offset_mask + 1);
                    EntryLo elo;
                    if (odd)
                    {
                        if (!entry.entry_odd.V)
                        {
                            return {};
                        }
                        elo.full = entry.entry_odd.full;
                    }
                    else
                    {
                        if (!entry.entry_even.V)
                        {
                            return {};
                        }
                        elo.full = entry.entry_even.full;
                    }
                    uint32_t paddr = (elo.PFN << 12) | (vaddr & offset_mask);
                    return paddr;
                }
            }
            Logger::Warn("TLB miss at {:08x}", vaddr);
            throw_exception(prevPc, ExceptionType::TLBMissLoad);
            set_cp0_regs_exception(vaddr);
            return {};
        }

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
        inline void set_interrupt(InterruptType type, bool value);
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
        std::function<int32_t(uint32_t, hydra::ButtonType)> read_input_callback_;
    };
} // namespace cerberus
