#include "libretro.h"
#include "gb.h"

GameBoy* gb;

static retro_video_refresh_t video_cb;

void retro_set_environment(retro_environment_t cb) {
    enum retro_pixel_format format = RETRO_PIXEL_FORMAT_XRGB8888;
    cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &format);
}

void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }

void retro_set_audio_sample(retro_audio_sample_t cb) {}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {}

void retro_set_input_poll(retro_input_poll_t cb) {}

void retro_set_input_state(retro_input_state_t cb) {}

void retro_init(void) {}

void retro_deinit(void) {}

unsigned retro_api_version(void) { return RETRO_API_VERSION; }

void retro_get_system_info(struct retro_system_info* info) {
    info->library_name = "Rondo";
    info->library_version = "dev";
    info->valid_extensions = "gb";
    info->need_fullpath = false;
    info->block_extract = false;
}

void retro_get_system_av_info(struct retro_system_av_info* info) {
    info->geometry.base_width = SCREEN_WIDTH;
    info->geometry.base_height = SCREEN_HEIGHT;
    info->geometry.max_width = SCREEN_WIDTH;
    info->geometry.max_height = SCREEN_HEIGHT;
    info->geometry.aspect_ratio = 0; // Interpreted as base_width/base_height

    info->timing.fps = FRAME_RATE;
    info->timing.sample_rate = 48000.0; // Placeholder
}

void retro_set_controller_port_device(unsigned port, unsigned device) {}

void retro_reset(void) {}

void retro_run(void) {
    run_frame(gb);

    video_cb(gb->fbuf, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH * sizeof(u32));
}

size_t retro_serialize_size(void) { return 0; }

bool retro_serialize(void* data, size_t size) { return false; }

bool retro_unserialize(const void* data, size_t size) { return false; }

void retro_cheat_reset(void) {}

void retro_cheat_set(unsigned index, bool enabled, const char* code) {}

bool retro_load_game(const struct retro_game_info* game) {
    gb = make_gb(game->data, game->size);
    if (!gb) {
        return false;
    }

    return true;
}

bool retro_load_game_special(unsigned game_type,
                             const struct retro_game_info* info,
                             size_t num_info) {
    return false;
}

void retro_unload_game(void) {
    destroy_gb(gb);
    gb = NULL;
}

unsigned retro_get_region(void) { return 0; }

void* retro_get_memory_data(unsigned id) { return NULL; }

size_t retro_get_memory_size(unsigned id) { return 0; }
