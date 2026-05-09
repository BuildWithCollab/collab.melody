// Implementation of collab.melody.player using miniaudio.

module;

#define MINIAUDIO_IMPLEMENTATION
#define NOMINMAX  // miniaudio pulls windows.h on Win32; block its min/max macros
#include <miniaudio.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

module collab.melody.player;

import collab.melody.render;
import collab.melody.melody;

namespace collab::melody {

namespace {

// Shared state between the producer (caller) and consumer (audio thread).
// One sound at a time, walked sample-by-sample.
struct PlaybackState {
    std::vector<std::int16_t> samples;
    std::atomic<std::size_t>  pos{0};
    std::atomic<bool>         done{true};
    std::atomic<bool>         cancelled{false};
};

void on_data(ma_device* device, void* output, const void* /*input*/, ma_uint32 frame_count) {
    auto* state = static_cast<PlaybackState*>(device->pUserData);
    auto* out   = static_cast<std::int16_t*>(output);

    if (state->cancelled.load(std::memory_order_acquire)) {
        std::memset(out, 0, frame_count * sizeof(std::int16_t));
        state->done.store(true, std::memory_order_release);
        return;
    }

    std::size_t       pos   = state->pos.load(std::memory_order_acquire);
    const std::size_t total = state->samples.size();

    for (ma_uint32 i = 0; i < frame_count; ++i) {
        out[i] = (pos < total) ? state->samples[pos++] : std::int16_t{0};
    }
    state->pos.store(pos, std::memory_order_release);

    if (pos >= total) state->done.store(true, std::memory_order_release);
}

}  // anonymous namespace

bool play_blocking(const RenderedAudio& audio) {
    if (audio.samples.empty()) return true;  // nothing to play, success

    PlaybackState state;
    state.samples = audio.samples;
    state.pos.store(0);
    state.done.store(false);
    state.cancelled.store(false);

    ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format   = ma_format_s16;
    cfg.playback.channels = 1;
    cfg.sampleRate        = static_cast<ma_uint32>(audio.sample_rate);
    cfg.dataCallback      = &on_data;
    cfg.pUserData         = &state;

    ma_device device;
    if (ma_device_init(nullptr, &cfg, &device) != MA_SUCCESS) {
        return false;
    }
    if (ma_device_start(&device) != MA_SUCCESS) {
        ma_device_uninit(&device);
        return false;
    }

    while (!state.done.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    // Tail flush so the OS buffer fully drains before we tear down.
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    ma_device_uninit(&device);
    return true;
}

bool play_blocking(const Melody& melody) {
    return play_blocking(render(melody));
}

// ─── Player ────────────────────────────────────────────────────────

struct Player::Impl {
    PlaybackState                 state;
    ma_device                     device{};
    bool                          device_started = false;
    int                           current_rate   = 0;
    std::mutex                    mtx;

    Impl() {
        state.done.store(true);
    }

    ~Impl() {
        teardown();
    }

    bool ensure_device(int sample_rate) {
        if (device_started && current_rate == sample_rate) return true;
        teardown();

        ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
        cfg.playback.format   = ma_format_s16;
        cfg.playback.channels = 1;
        cfg.sampleRate        = static_cast<ma_uint32>(sample_rate);
        cfg.dataCallback      = &on_data;
        cfg.pUserData         = &state;

        if (ma_device_init(nullptr, &cfg, &device) != MA_SUCCESS) return false;
        if (ma_device_start(&device) != MA_SUCCESS) {
            ma_device_uninit(&device);
            return false;
        }
        device_started = true;
        current_rate   = sample_rate;
        return true;
    }

    void teardown() {
        if (device_started) {
            ma_device_stop(&device);
            ma_device_uninit(&device);
            device_started = false;
        }
        current_rate = 0;
    }
};

Player::Player() : _impl(std::make_unique<Impl>()) {}

Player::~Player() = default;

bool Player::play(const RenderedAudio& audio) {
    if (!_impl) return false;
    if (audio.samples.empty()) return true;

    // Wait for any prior play to finish.
    wait();

    std::lock_guard<std::mutex> lock(_impl->mtx);
    _impl->state.samples   = audio.samples;
    _impl->state.pos.store(0, std::memory_order_release);
    _impl->state.cancelled.store(false, std::memory_order_release);
    _impl->state.done.store(false, std::memory_order_release);

    return _impl->ensure_device(audio.sample_rate);
}

bool Player::play(const Melody& melody) {
    return play(render(melody));
}

bool Player::is_playing() const noexcept {
    return _impl && !_impl->state.done.load(std::memory_order_acquire);
}

void Player::wait() {
    if (!_impl) return;
    while (!_impl->state.done.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void Player::cancel() noexcept {
    if (!_impl) return;
    _impl->state.cancelled.store(true, std::memory_order_release);
}

}  // namespace collab::melody
