#ifndef RONDO_GB_H
#define RONDO_GB_H

#include "stdbool.h"
#include "stdint.h"

#define RONDO_BIG_ENDIAN 0

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144

#define FRAME_RATE 59.7275005696058

#define OAM_COUNT 40

// Macro to define CPU register pairs
#if RONDO_BIG_ENDIAN
#define REG_DEF(HI, LO)                                                        \
    union {                                                                    \
        u16 HI##LO;                                                            \
        struct {                                                               \
            u8 HI, LO;                                                         \
        };                                                                     \
    };
#else
#define REG_DEF(HI, LO)                                                        \
    union {                                                                    \
        u16 HI##LO;                                                            \
        struct {                                                               \
            u8 LO, HI;                                                         \
        };                                                                     \
    };
#endif

typedef enum { DMG, SGB, CGB } GBType;

typedef struct GameBoy {
    GBType type;
    void* fbuf;
    bool end_frame;

    // Pointers to various regions of the GB's memory map
    // 0x0000-0x3FFF
    u8* rom_lo;
    // 0x4000-0x7FFF
    u8* rom_hi;
    // 0x8000-0x9FFF
    u8* vram;
    // 0xA000-0xBFFF
    u8* cartram;
    // 0xC000-0xCFFF
    u8* wram_lo;
    // 0xD000-0xDFFF
    u8* wram_hi;
    // 0xFE00-0xFE9F
    u8* oam;
    // 0xFF80-0xFFFF
    u8* hram;

    // Internal CPU registers and flags
    u8 a;
    bool f_z, f_n, f_h, f_c;
    u16 pc, sp;
    REG_DEF(b, c)
    REG_DEF(d, e)
    REG_DEF(h, l)

    bool ime;

    // FF00 (joypad/P1)
    u8 input; // d-pad in the low nibble, buttons in the high
    bool p1_get_dpad;
    bool p1_get_btn;

    u8 sb; // FF01
    u8 sc; // FF02

    // Timer registers
    u16 div; // Upper 8 bits are FF04
    u8 tima; // FF05
    u8 tma;  // FF06
    // TAC (FF07)
    bool tac_en; // Bit 2
    u8 tac_clk;  // Bits 0-1

    u8 if_; // FF0F

    // Audio stuff
    u8 div_apu_counter;

    // Channel 1
    u16 ch1_timer;
    u8 ch1_index;
    bool ch1_dac;
    bool ch1_active;
    // AUD1SWEEP/NR10 (FF10)
    u8 ch1_sweep_time;  // Bits 4-6
    bool ch1_sweep_dir; // Bit 3
    u8 ch1_sweep_shift; // Bits 0-2
    // AUD1LEN/NR11 (FF11)
    u8 ch1_duty; // Bits 6-7
    u8 ch1_len;  // Bits 0-5 (inverted)
    u8 ch1_len_ctr;
    // AUD1ENV/NR12 (FF12)
    u8 ch1_env_init;  // Bits 4-7
    bool ch1_env_dir; // Bit 3
    u8 ch1_env_sweep; // Bits 0-2
    // AUD1LOW/NR13 (FF13)
    // AUD1HIGH/NR14 (FF14)
    u16 ch1_period;  // NR13 bits 0-7, NR14 bits 0-2 (inverted)
    bool ch1_len_en; // NR14 bit 6

    // Channel 2
    u16 ch2_timer;
    u8 ch2_index;
    bool ch2_dac;
    bool ch2_active;
    // AUD2LEN/NR21 (FF16)
    u8 ch2_duty; // Bits 6-7
    u8 ch2_len;  // Bits 0-5 (inverted)
    u8 ch2_len_ctr;
    // AUD2ENV/NR22 (FF17)
    u8 ch2_env_init;  // Bits 4-7
    bool ch2_env_dir; // Bit 3
    u8 ch2_env_sweep; // Bits 0-2
    // AUD2LOW/NR23 (FF18)
    // AUD2HIGH/NR24 (FF19)
    u16 ch2_period;  // NR23 bits 0-7, NR24 bits 0-2 (inverted)
    bool ch2_len_en; // NR24 bit 6

    // Channel 3
    u16 ch3_timer;
    u8 ch3_index;
    bool ch3_dac; // AUD3ENA/NR30 (FF1A), Bit 7
    bool ch3_active;
    // AUD3LEN/NR31 (FF1B)
    u8 ch3_len; // Bits 0-7 (inverted)
    u8 ch3_len_ctr;
    // AUD3LEVEL/NR32 (FF1C)
    u8 ch3_vol; // Bits 5-6
    // AUD3LOW/NR33 (FF1D)
    // AUD3HIGH/NR34 (FF1E)
    u16 ch3_period;  // NR33 bits 0-7, NR34 bits 0-2 (inverted)
    bool ch3_len_en; // NR34 bit 6

    // Channel 4
    u32 ch4_timer;
    u32 ch4_period;
    u16 ch4_lfsr;
    bool ch4_dac;
    bool ch4_active;
    // AUD4LEN/NR41 (FF20)
    u8 ch4_len; // Bits 0-5 (inverted)
    u8 ch4_len_ctr;
    // AUD4ENV/NR42 (FF21)
    u8 ch4_env_init;  // Bits 4-7
    bool ch4_env_dir; // Bit 3
    u8 ch4_env_sweep; // Bits 0-2
    // AUD4POLY/NR43 (FF22)
    u8 ch4_shift;   // Bits 4-7
    bool ch4_width; // Bit 3
    u8 ch4_divider; // Bits 0-2
    // AUD4GO/NR44 (FF23)
    bool ch4_len_en; // Bit 6

    // AUDVOL/NR50 (FF24)
    u8 vol_l; // Bits 4-6
    u8 vol_r; // Bits 0-2

    // AUDTERM/NR51 (FF25)
    bool ch4_l; // Bit 7
    bool ch3_l; // Bit 6
    bool ch2_l; // Bit 5
    bool ch1_l; // Bit 4
    bool ch4_r; // Bit 3
    bool ch3_r; // Bit 2
    bool ch2_r; // Bit 1
    bool ch1_r; // Bit 0

    // AUDENA/NR52 (FF26)
    bool apu_en; // Bit 7

    // FF30-FF3F
    u8 wave_ram[32];

    // LCDC (FF40)
    bool lcd_en;   // Bit 7
    bool win_map;  // Bit 6
    bool win_en;   // Bit 5
    bool tile_sel; // Bit 4
    bool bg_map;   // Bit 3
    bool obj_size; // Bit 2
    bool obj_en;   // Bit 1
    bool bg_en;    // Bit 0

    // STAT (FF41)
    u8 stat;

    u8 scy;     // FF42
    u8 scx;     // FF43
    u8 ly;      // FF44
    u8 lyc;     // FF45
    u8 bgp[4];  // FF47
    u8 obp0[4]; // FF48
    u8 obp1[4]; // FF49
    u8 wy;      // FF4A
    u8 wx;      // FF4B

    u8 ie; // FFFF

    // Ranges from -80 to 375 on each scanline
    s16 dots;
} GameBoy;

// Return null if there was a problem
GameBoy* make_gb(u8* rom, size_t size);
void destroy_gb(GameBoy* gb);

void run_frame(GameBoy* gb);

u8 read(GameBoy* gb, u16 addr);
void write(GameBoy* gb, u16 addr, u8 data);

void cycle(GameBoy* gb);

// Defined in libretro.c
void play_sample(s16 l, s16 r);

#endif
