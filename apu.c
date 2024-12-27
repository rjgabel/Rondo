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

void render_audio_sample(GameBoy* gb) {
    u8 ch1_sample = ch1_get_sample(gb);

    s16 converted = ch1_sample ? -0x8000 : 0x7FFF;
    play_sample(converted, converted);
}
