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

void render_audio_sample(GameBoy* gb) {
    u8 ch1_sample = gb->ch1_active ? ch1_get_sample(gb) : 0;
    u8 ch2_sample = gb->ch2_active ? ch2_get_sample(gb) : 0;
    u8 ch3_sample = gb->ch3_active ? ch3_get_sample(gb) : 0;

    s16 converted = 0x7FFF - (ch1_sample + ch2_sample + ch3_sample) * 0x5B0;
    play_sample(converted, converted);
}

void ch1_trigger(GameBoy* gb) {
    if (gb->ch1_dac) {
        gb->ch1_active = true;
    }
}

void ch2_trigger(GameBoy* gb) {
    if (gb->ch2_dac) {
        gb->ch2_active = true;
    }
}

void ch3_trigger(GameBoy* gb) {
    if (gb->ch3_dac) {
        gb->ch3_active = true;
    }
}

void ch4_trigger(GameBoy* gb) {
    if (gb->ch4_dac) {
        gb->ch4_active = true;
    }
}
