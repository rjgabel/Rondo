#ifndef RONDO_APU_H
#define RONDO_APU_H

void render_audio_sample(struct GameBoy* gb);
void ch1_trigger(struct GameBoy* gb);
void ch2_trigger(struct GameBoy* gb);
void ch3_trigger(struct GameBoy* gb);
void ch4_trigger(struct GameBoy* gb);

#endif
