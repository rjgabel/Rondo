#include "gb.h"

#include "apu.h"
#include "cpu.h"
#include "lcd.h"
#include "stdio.h"
#include "stdlib.h"

// Critical memory allocation, abort on failure
void* crit_alloc(size_t size) {
    void* ptr = calloc(size, 1);
    if (!ptr) {
        printf("Memory allocation failed!");
        exit(1);
    }
    return ptr;
}

GameBoy* make_gb(u8* rom, size_t size) {
    if (size < 0x8000) {
        printf("File must be at least 0x8000 bytes\n");
        return NULL;
    }

    // Header checks
    u8 rom_size = rom[0x0148];
    if (rom_size > 8) {
        printf("Header byte 0x0148 (rom size) must not be greather than 8\n");
        return NULL;
    }
    if (0x8000 * ((size_t)1 << rom_size) != size) {
        printf("Header size mismatch\n");
        return NULL;
    }

    // Determine console type
    GameBoy* gb;
    if (rom[0x0143] == 0x80 || rom[0x0143] == 0xC0) {
        // Game Boy Color
        printf("Game Boy Color not implemented yet!\n");
        exit(1);
    } else if (rom[0x0146] == 0x03) {
        // Super Game Boy
        printf("Super Game Boy not implemented yet!\n");
        exit(1);
    } else {
        // Monochrome/Original Game Boy
        gb = crit_alloc(sizeof(GameBoy));
        gb->type = DMG;
    }

    gb->vram = crit_alloc(gb->type == CGB ? 0x4000 : 0x2000);
    gb->wram_lo = crit_alloc(gb->type == CGB ? 0x8000 : 0x2000);
    gb->wram_hi = gb->wram_lo + 0x1000;
    gb->oam = crit_alloc(0xA0);
    gb->hram = crit_alloc(0x7F);

    // Cartridge stuff
    if (rom[0x147] != 0x00) {
        printf("Only standard (no mapper) carts supported right now");
        exit(1);
    }
    gb->rom_lo = rom;
    gb->rom_hi = rom + 0x4000;
    gb->cartram = NULL;

    // Initialize registers
    // (TODO: Make these actually correct later;)
    gb->a = 0;
    gb->f_z = false;
    gb->f_n = false;
    gb->f_h = false;
    gb->f_c = false;
    gb->bc = 0;
    gb->de = 0;
    gb->hl = 0;
    gb->pc = 0x0100;
    gb->sp = 0xFFFE;

    gb->ime = false;

    gb->p1_get_btn = gb->p1_get_dpad = false;

    gb->lcd_en = true;

    gb->fbuf = crit_alloc(SCREEN_HEIGHT * SCREEN_WIDTH * sizeof(u32));

    return gb;
}

void destroy_gb(GameBoy* gb) {
    free(gb->vram);
    free(gb->cartram);
    free(gb->wram_lo);
    free(gb->oam);
    free(gb->hram);
    free(gb);
}

void run_frame(GameBoy* gb) {
    while (!gb->end_frame) {
        // if (gb->pc == 0x2E4) {
        //     printf("Reached\n");
        //     exit(1);
        // }
        run_opcode(gb);
    }
    gb->end_frame = false;
}

u8 io_read(GameBoy* gb, u16 addr) {
    addr &= 0x7F;

    // Wave RAM
    if (addr >= 0x30 && addr <= 0x3f) {
        u8 index = (addr - 0x30) * 2;
        return gb->wave_ram[index] << 4 | gb->wave_ram[index + 1];
    }

    switch (addr) {
    case 0x00: // P1 (FF00)
        return (gb->p1_get_dpad ? gb->input & 0xF : 0xF) &
               (gb->p1_get_btn ? gb->input >> 4 : 0xF);
    case 0x01: // SB (FF01)
        return gb->sb;
    case 0x02: // SC (FF02)
        return gb->sc;
    case 0x04: // DIV (FF04)
        return gb->div >> 8;
    case 0x05: // TIMA (FF05)
        return gb->tima;
    case 0x06: // TMA (FF06)
        return gb->tma;
    case 0x07: // TAC (FF07)
        return gb->tac_en << 2 | gb->tac_clk | 0xF8;
    case 0x0F: // IF (FF0F)
        return gb->if_;
    case 0x10: // AUD1SWEEP/NR10 (FF10)
        return gb->ch1_sweep_time << 4 | gb->ch1_sweep_dir << 3 |
               gb->ch1_sweep_shift;
    case 0x11: // AUD1LEN/NR11 (FF11)
        return gb->ch1_duty << 6;
    case 0x12: // AUD1ENV/NR12 (FF12)
        return gb->ch1_env_init << 4 | gb->ch1_env_dir << 3 | gb->ch1_env_sweep;
    case 0x13: // AUD1LOW/NR13 (FF13)
        return 0xFF;
    case 0x14: // AUD1HIGH/NR14 (FF14)
        return gb->ch1_len_en << 6;
    case 0x16: // AUD2LEN/NR21 (FF16)
        return gb->ch2_duty << 6;
    case 0x17: // AUD2ENV/NR22 (FF17)
        return gb->ch2_env_init << 4 | gb->ch2_env_dir << 3 | gb->ch2_env_sweep;
    case 0x18: // AUD2LOW/NR23 (FF18)
        return 0xFF;
    case 0x19: // AUD2HIGH/NR24 (FF19)
        return gb->ch2_len_en << 6;
    case 0x1A: // AUD3ENA/NR30 (FF1A)
        return gb->ch3_dac << 7;
    case 0x1B: // AUD3LEN/NR31 (FF1B)
        return 0xFF;
    case 0x1C: // AUD3LEVEL/NR32 (FF1C)
        return gb->ch3_vol << 5;
    case 0x1D: // AUD3LOW/NR33 (FF1D)
        return 0xFF;
    case 0x1E: // AUD3HIGH/NR34 (FF1E)
        return gb->ch3_len_en << 6;
    case 0x20: // AUD4LEN/NR41 (FF20)
        return 0xFF;
    case 0x21: // AUD4ENV/NR42 (FF21)
        return gb->ch4_env_init << 4 | gb->ch4_env_dir << 3 | gb->ch4_env_sweep;
    case 0x22: // AUD4POLY/NR43 (FF22)
        return gb->ch4_shift << 4 | gb->ch4_width << 3 | gb->ch4_divider;
    case 0x23: // AUD4GO/NR44 (FF23)
        return gb->ch4_len_en << 6;
    case 0x24: // AUDVOL/NR50 (FF24)
        return gb->vol_l << 4 | gb->vol_r;
    case 0x25: // AUDTERM/NR51 (FF25)
        return gb->ch4_l << 7 | gb->ch3_l << 6 | gb->ch2_l << 5 |
               gb->ch1_l << 4 | gb->ch4_r << 3 | gb->ch3_r << 2 |
               gb->ch2_r << 1 | gb->ch1_r;
    case 0x26: // AUDENA/NR52 (FF26)
        return gb->apu_en << 7 | gb->ch4_active << 3 | gb->ch3_active << 2 |
               gb->ch2_active << 1 | gb->ch1_active;
    case 0x40: // LCDC (FF40)
        return (gb->lcd_en << 7) | (gb->win_map << 6) | (gb->win_en << 5) |
               (gb->tile_sel << 4) | (gb->bg_map << 3) | (gb->obj_size << 2) |
               (gb->obj_en << 1) | (gb->bg_en << 0);
    case 0x41: // STAT (FF41)
        return gb->stat;
    case 0x42: // SCY (FF42)
        return gb->scy;
    case 0x43: // SCX (FF43)
        return gb->scx;
    case 0x44: // LY (FF44)
        return gb->ly;
    case 0x45: // LYC (FF45)
        return gb->lyc;
    case 0x46: // DMA (FF46)
        return 0xFF;
    case 0x47: // BGP (FF47)
        return (gb->bgp[0] << 0) | (gb->bgp[1] << 2) | (gb->bgp[2] << 4) |
               (gb->bgp[3] << 6);
    case 0x48: // OBP0 (FF48)
        return (gb->obp0[0] << 0) | (gb->obp0[1] << 2) | (gb->obp0[2] << 4) |
               (gb->obp0[3] << 6);
    case 0x49: // OBP1 (FF49)
        return (gb->obp1[0] << 0) | (gb->obp1[1] << 2) | (gb->obp1[2] << 4) |
               (gb->obp1[3] << 6);
    case 0x4A: // WY (FF4A)
        return gb->wy;
    case 0x4B: // WX (FF4B)
        return gb->wx;
    default:
        printf("Unimplemented read at IO address %x\n", (int)addr);
        exit(1);
    }
}

#define GET_BITS(value, lo, hi) ((value) >> (lo) & (1 << ((hi) - (lo) + 1)) - 1)
#define GET_BIT(value, b) ((value) >> (b) & 1)

void io_write(GameBoy* gb, u16 addr, u8 data) {
    addr &= 0x7F;

    // Wave RAM
    if (addr >= 0x30 && addr <= 0x3f) {
        u8 index = (addr - 0x30) * 2;
        gb->wave_ram[index] = GET_BITS(data, 4, 7);
        gb->wave_ram[index + 1] = GET_BITS(data, 0, 3);
        return;
    }

    switch (addr) {
    case 0x00: // P1 (FF00)
        gb->p1_get_dpad = !(data & (1 << 4));
        gb->p1_get_btn = !(data & (1 << 5));
        break;
    case 0x01: // SB (FF01)
        gb->sb = data;
        break;
    case 0x02: // SC (FF02)
        gb->sc = data;
        break;
    case 0x04: // DIV (FF04)
        gb->div = 0;
        break;
    case 0x05: // TIMA (FF05)
        gb->tima = data;
        break;
    case 0x06: // TMA (FF06)
        gb->tma = data;
        break;
    case 0x07: // TAC (FF07)
        gb->tac_en = data & (1 << 2);
        gb->tac_clk = data & 0x3;
        break;
    case 0x0F: // IF (FF0F)
        gb->if_ = data & 0x1F;
        break;
    case 0x10: // AUD1SWEEP/NR10 (FF10)
        gb->ch1_sweep_time = GET_BITS(data, 4, 6);
        gb->ch1_sweep_dir = GET_BIT(data, 3);
        gb->ch1_sweep_shift = GET_BITS(data, 0, 2);
        break;
    case 0x11: // AUD1LEN/NR11 (FF11)
        gb->ch1_duty = GET_BITS(data, 6, 7);
        gb->ch1_len = (data & 0x3F) ^ 0x3F;
        break;
    case 0x12: // AUD1ENV/NR12 (FF12)
        gb->ch1_env_init = GET_BITS(data, 4, 7);
        gb->ch1_env_dir = GET_BIT(data, 3);
        gb->ch1_env_sweep = GET_BITS(data, 0, 2);
        if (data & 0xF8) {
            gb->ch1_dac = true;
        } else {
            gb->ch1_dac = false;
            gb->ch1_active = false;
        }
        break;
    case 0x13: // AUD1LOW/NR13 (FF13)
        gb->ch1_period = (gb->ch1_period & 0x700) | ~data;
        break;
    case 0x14: // AUD1HIGH/NR14 (FF14)
        gb->ch1_len_en = GET_BIT(data, 6);
        gb->ch1_period = (gb->ch1_period & 0xFF) | GET_BITS(~data, 0, 2) << 8;
        if (data & 0x80) {
            ch1_trigger(gb);
        }
        break;
    case 0x16: // AUD2LEN/NR21 (FF16)
        gb->ch2_duty = GET_BITS(data, 6, 7);
        gb->ch2_len = (data & 0x3F) ^ 0x3F;
        break;
    case 0x17: // AUD2ENV/NR22 (FF17)
        gb->ch2_env_init = GET_BITS(data, 4, 7);
        gb->ch2_env_dir = GET_BIT(data, 3);
        gb->ch2_env_sweep = GET_BITS(data, 0, 2);
        if (data & 0xF8) {
            gb->ch2_dac = true;
        } else {
            gb->ch2_dac = false;
            gb->ch2_active = false;
        }
        break;
    case 0x18: // AUD2LOW/NR23 (FF18)
        gb->ch2_period = (gb->ch2_period & 0x700) | ~data;
        break;
    case 0x19: // AUD2HIGH/NR24 (FF19)
        gb->ch2_len_en = GET_BIT(data, 6);
        gb->ch2_period = (gb->ch2_period & 0xFF) | GET_BITS(~data, 0, 2) << 8;
        if (data & 0x80) {
            ch2_trigger(gb);
        }
        break;
    case 0x1A: // AUD3ENA/NR30 (FF1A)
        if (data & 0x80) {
            gb->ch3_dac = true;
        } else {
            gb->ch3_dac = false;
            gb->ch3_active = false;
        }
        break;
    case 0x1B: // AUD3LEN/NR31 (FF1B)
        gb->ch3_len = ~data;
        break;
    case 0x1C: // AUD3LEVEL/NR32 (FF1C)
        gb->ch3_vol = GET_BITS(data, 5, 6);
        break;
    case 0x1D: // AUD3LOW/NR33 (FF1D)
        gb->ch3_period = (gb->ch3_period & 0x700) | ~data;
        break;
    case 0x1E: // AUD3HIGH/NR34 (FF1E)
        gb->ch3_active = GET_BIT(data, 7);
        gb->ch3_len_en = GET_BIT(data, 6);
        gb->ch3_period = (gb->ch3_period & 0xFF) | GET_BITS(~data, 0, 2) << 8;
        if (data & 0x80) {
            ch3_trigger(gb);
        }
        break;
    case 0x20: // AUD4LEN/NR41 (FF20)
        gb->ch4_len = (data & 0x3F) ^ 0x3F;
        break;
    case 0x21: // AUD4ENV/NR42 (FF21)
        gb->ch4_env_init = GET_BITS(data, 4, 7);
        gb->ch4_env_dir = GET_BIT(data, 3);
        gb->ch4_env_sweep = GET_BITS(data, 0, 2);
        if (data & 0xF8) {
            gb->ch4_dac = true;
        } else {
            gb->ch4_dac = false;
            gb->ch4_active = false;
        }
        break;
    case 0x22: // AUD4POLY/NR43 (FF22)
        gb->ch4_shift = GET_BITS(data, 4, 7);
        gb->ch4_width = GET_BIT(data, 3);
        gb->ch4_divider = GET_BITS(data, 0, 2);
        update_ch4_period(gb);
        break;
    case 0x23: // AUD4GO/NR44 (FF23)
        gb->ch4_len_en = GET_BIT(data, 6);
        if (data & 0x80) {
            ch4_trigger(gb);
        }
        break;
    case 0x24: // AUDVOL/NR50 (FF24)
        gb->vol_l = GET_BITS(data, 4, 6);
        gb->vol_r = GET_BITS(data, 0, 2);
        break;
    case 0x25: // AUDTERM/NR51 (FF25)
        gb->ch4_l = GET_BIT(data, 7);
        gb->ch3_l = GET_BIT(data, 6);
        gb->ch2_l = GET_BIT(data, 5);
        gb->ch1_l = GET_BIT(data, 4);
        gb->ch4_r = GET_BIT(data, 3);
        gb->ch3_r = GET_BIT(data, 2);
        gb->ch2_r = GET_BIT(data, 1);
        gb->ch1_r = GET_BIT(data, 0);
        break;
    case 0x26: // AUDENA/NR52 (FF26)
        gb->apu_en = GET_BIT(data, 7);
        break;
    case 0x40: // LCDC (FF40)
        gb->lcd_en = data & (1 << 7);
        gb->win_map = data & (1 << 6);
        gb->win_en = data & (1 << 5);
        gb->tile_sel = data & (1 << 4);
        gb->bg_map = data & (1 << 3);
        gb->obj_size = data & (1 << 2);
        gb->obj_en = data & (1 << 1);
        gb->bg_en = data & (1 << 0);
        break;
    case 0x41: // STAT (FF41)
        gb->stat = data;
        break;
    case 0x42: // SCY (FF42)
        gb->scy = data;
        break;
    case 0x43: // SCX (FF43)
        gb->scx = data;
        break;
    case 0x44: // LY (FF44)
        break;
    case 0x45: // LYC (FF45)
        gb->lyc = data;
        break;
    case 0x46: // DMA (FF46)
        for (u8 i = 0; i < 0xA0; i++) {
            gb->oam[i] = read(gb, (data << 8) + i);
        }
        break;
    case 0x47: // BGP (FF47)
        gb->bgp[0] = (data >> 0) & 0x3;
        gb->bgp[1] = (data >> 2) & 0x3;
        gb->bgp[2] = (data >> 4) & 0x3;
        gb->bgp[3] = (data >> 6) & 0x3;
        break;
    case 0x48: // OBP0 (FF48)
        gb->obp0[0] = (data >> 0) & 0x3;
        gb->obp0[1] = (data >> 2) & 0x3;
        gb->obp0[2] = (data >> 4) & 0x3;
        gb->obp0[3] = (data >> 6) & 0x3;
        break;
    case 0x49: // OBP1 (FF49)
        gb->obp1[0] = (data >> 0) & 0x3;
        gb->obp1[1] = (data >> 2) & 0x3;
        gb->obp1[2] = (data >> 4) & 0x3;
        gb->obp1[3] = (data >> 6) & 0x3;
        break;
    case 0x4A: // WY (FF4A)
        gb->wy = data;
        break;
    case 0x4B: // WX (FF4B)
        gb->wx = data;
        break;
    case 0x7F:
        // Tetris writes here due to a software bug
        break;
    default:
        printf("Unimplemented write %x at IO address %x\n", (int)data,
               (int)addr);
        exit(1);
    }
}

u8 read(GameBoy* gb, u16 addr) {
    if (addr < 0x8000) {
        // 0x0000 - 0x7FFF (ROM)
        u8* ptr = (addr & 0x4000) ? gb->rom_hi : gb->rom_lo;
        return ptr ? ptr[addr & 0x3FFF] : 0xFF;
    } else if (addr < 0xA000) {
        // 0x8000 - 0x9FFF (VRAM)
        return gb->vram[addr % 0x2000];
    } else if (addr < 0xC000) {
        // 0xA000 - 0xBFFF (External RAM)
        // TODO: implement external RAM
        return 0xFF;
    } else if (addr < 0xFE00) {
        // 0xC000 - 0xFDFF (WRAM)
        // Designed to account for echo RAM
        u8* ptr = (addr & 0x1000) ? gb->wram_hi : gb->wram_lo;
        return ptr[addr & 0x0FFF];
    } else if (addr < 0xFEA0) {
        // 0xFE00 - 0xFE9F (OAM)
        return gb->oam[addr & 0xFF];
    } else if (addr < 0xFF00) {
        // 0xFEA0 - 0xFEFF (unused)
        return 0xFF;
    } else if (addr < 0xFF80) {
        // 0xFF00 - 0xFF7F (IO)
        return io_read(gb, addr);
    } else if (addr < 0xFFFF) {
        // 0xFF80 - 0xFFFE (HRAM)
        return gb->hram[addr & 0x7F];
    } else {
        return gb->ie;
    }
}

void write(GameBoy* gb, u16 addr, u8 data) {
    if (addr < 0x8000) {
        // 0x0000 - 0x7FFF (ROM)
    } else if (addr < 0xA000) {
        // 0x8000 - 0x9FFF (VRAM)
        gb->vram[addr % 0x2000] = data;
    } else if (addr < 0xC000) {
        // 0xA000 - 0xBFFF (External RAM)
        // TODO: implement external RAM
    } else if (addr < 0xFE00) {
        // 0xC000 - 0xFDFF (WRAM)
        // Designed to account for echo RAM
        u8* ptr = (addr & 0x1000) ? gb->wram_hi : gb->wram_lo;
        ptr[addr & 0x0FFF] = data;
    } else if (addr < 0xFEA0) {
        // 0xFE00 - 0xFE9F (OAM)
        gb->oam[addr & 0xFF] = data;
    } else if (addr < 0xFF00) {
        // 0xFEA0 - 0xFEFF (unused)
    } else if (addr < 0xFF80) {
        // 0xFF00 - 0xFF7F (IO)
        io_write(gb, addr, data);
    } else if (addr < 0xFFFF) {
        // 0xFF80 - 0xFFFE (HRAM)
        gb->hram[addr & 0x7F] = data;
    } else {
        // 0xFFFF (IE)
        gb->ie = data & 0x1F;
    }
}

void cycle(GameBoy* gb) {
    if (gb->lcd_en) {
        for (int i = 0; i < 4; i++) {
            lcd_cycle(gb);
        }
    }
    if (gb->apu_en) {
        render_audio_sample(gb);
    }

    // Divider register stuff
    static const u16 TICK_MASKS[4] = {0x03FF, 0x000F, 0x003F, 0x00FF};
    gb->div += 4;
    if (!(gb->div & TICK_MASKS[gb->tac_clk]) && gb->tac_en) {
        // Timer tick
        gb->tima++;
        if (!gb->tima) {
            gb->tima = gb->tma;
            gb->if_ |= (1 << 2);
        }
    }
    if ((gb->div & 0x1FFF) && gb->apu_en) {
        div_apu_event(gb);
    }
}
