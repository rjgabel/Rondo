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
    return WAVEFORMS[gb->ch1_duty][gb->ch1_index];
}

static u8 ch2_get_sample(GameBoy* gb) {
    if (!gb->ch2_timer--) {
        gb->ch2_timer = gb->ch2_period;
        gb->ch2_index = (gb->ch2_index + 1) & 7;
    }
    return WAVEFORMS[gb->ch2_duty][gb->ch2_index];
}

void render_audio_sample(GameBoy* gb) {
    u8 ch1_sample = ch1_get_sample(gb);
    u8 ch2_sample = ch2_get_sample(gb);

    s16 converted = (ch1_sample + ch2_sample) * 0x3FFF;
    play_sample(converted, converted);
}
