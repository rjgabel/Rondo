#include "apu.h"
#include "gb.h"

static const bool WAVEFORMS[4][8] = {{0, 0, 0, 0, 0, 0, 0, 1},
                                     {1, 0, 0, 0, 0, 0, 0, 1},
                                     {1, 0, 0, 0, 0, 1, 1, 1},
                                     {0, 1, 1, 1, 1, 1, 1, 0}};

static u8 ch1_get_sample(GameBoy* gb) {
    if (!gb->ch1_timer--) {
        gb->ch1_timer = gb->ch1_period;
        gb->ch1_index = (gb->ch1_index + 1) & 7;
    }
    return WAVEFORMS[gb->ch1_duty][gb->ch1_index] ? gb->ch1_env_init : 0;
}

static u8 ch2_get_sample(GameBoy* gb) {
    if (!gb->ch2_timer--) {
        gb->ch2_timer = gb->ch2_period;
        gb->ch2_index = (gb->ch2_index + 1) & 7;
    }
    return WAVEFORMS[gb->ch2_duty][gb->ch2_index] ? gb->ch2_env_init : 0;
}

static u8 ch3_get_sample(GameBoy* gb) {
    if (!gb->ch3_timer--) {
        gb->ch3_timer = gb->ch3_period;
        gb->ch3_index = (gb->ch3_index + 1) & 31;
    }
    u8 sample = gb->wave_ram[gb->ch3_index];
    if (gb->ch3_vol == 3) {
        sample >>= 2;
    } else if (gb->ch3_vol == 2) {
        sample >>= 1;
    } else if (gb->ch3_vol == 0) {
        sample = 0;
    }
    return sample;
}

static u8 ch4_get_sample(GameBoy* gb) {
    if (!gb->ch4_timer--) {
        gb->ch4_timer = gb->ch4_period;
        // Clock LFSR
        bool new_bit = (gb->ch4_lfsr & 1) == (gb->ch4_lfsr >> 1 & 1);
        gb->ch4_lfsr =
            new_bit ? (gb->ch4_lfsr | 0x8000) : (gb->ch4_lfsr & 0x7FFF);
        if (gb->ch4_width) {
            gb->ch4_lfsr =
                new_bit ? (gb->ch4_lfsr | 0x0080) : (gb->ch4_lfsr & 0xFF7F);
        }
        gb->ch4_lfsr >>= 1;
    }
    return (gb->ch4_lfsr & 1) ? gb->ch4_env_init : 0;
}

void render_audio_sample(GameBoy* gb) {
    u8 ch1_sample = gb->ch1_active ? ch1_get_sample(gb) : 0;
    u8 ch2_sample = gb->ch2_active ? ch2_get_sample(gb) : 0;
    u8 ch3_sample =
        gb->ch3_active ? (ch3_get_sample(gb), ch3_get_sample(gb)) : 0;
    u8 ch4_sample = gb->ch4_active ? ch4_get_sample(gb) : 0;

    s16 converted =
        0x7FFF - (ch1_sample + ch2_sample + ch3_sample + ch4_sample) * 0x444;
    play_sample(converted, converted);
}

void ch1_trigger(GameBoy* gb) {
    if (gb->ch1_dac) {
        gb->ch1_active = true;
        if (!gb->ch1_len_ctr) {
            gb->ch1_len_ctr = gb->ch1_len;
        }
    }
}

void ch2_trigger(GameBoy* gb) {
    if (gb->ch2_dac) {
        gb->ch2_active = true;
        if (!gb->ch2_len_ctr) {
            gb->ch2_len_ctr = gb->ch2_len;
        }
    }
}

void ch3_trigger(GameBoy* gb) {
    if (gb->ch3_dac) {
        gb->ch3_active = true;
        if (!gb->ch3_len_ctr) {
            gb->ch3_len_ctr = gb->ch3_len;
        }
    }
}

void ch4_trigger(GameBoy* gb) {
    if (gb->ch4_dac) {
        gb->ch4_active = true;
        gb->ch4_lfsr = 0;
        if (!gb->ch4_len_ctr) {
            gb->ch4_len_ctr = gb->ch4_len;
        }
    }
}

void update_ch4_period(GameBoy* gb) {
    u32 period = gb->ch4_divider << 2;
    if (period == 0) {
        period = 2;
    }
    period << gb->ch4_shift;
    gb->ch4_period = period;
}

#define UPDATE_LENGTH(c)                                                       \
    {                                                                          \
        if (gb->ch##c##_active && gb->ch##c##_len_en) {                        \
            gb->ch##c##_len_ctr++;                                             \
            if (!gb->ch##c##_len_ctr) {                                        \
                gb->ch##c##_active = false;                                    \
            }                                                                  \
        }                                                                      \
    }

void div_apu_event(GameBoy* gb) {
    gb->div_apu_counter++;
    gb->div_apu_counter &= 7;
    if (!(gb->div_apu_counter & 7)) {
        // Envelope sweep
    }
    if (!(gb->div_apu_counter & 3)) {
        // CH1 freq sweep
    }
    if (!(gb->div_apu_counter & 1)) {
        // Sound length
        UPDATE_LENGTH(1);
        UPDATE_LENGTH(2);
        UPDATE_LENGTH(3);
        UPDATE_LENGTH(4);
    }
}
