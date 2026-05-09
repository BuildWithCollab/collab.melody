// Audio playback via miniaudio.
//
// Two flavors of API:
//
//   play_blocking(audio)  — open device, play to completion, close.
//                           Simplest possible usage; one-shot.
//
//   Player                 — keeps a device-init pool warm so subsequent
//                           plays start with no device-init latency.
//                           Useful for apps that fire many notifications.
//
// Both internally translate the rendered int16 PCM into miniaudio's
// data callback through a single-producer / single-consumer queue.
// The miniaudio details live in the .cpp implementation file.

export module collab.melody.player;

import std;
import collab.melody.render;
import collab.melody.melody;

export namespace collab::melody {

// Block until the rendered audio finishes (or device init fails).
// Returns false if the audio device couldn't be opened.
bool play_blocking(const RenderedAudio& audio);

// Convenience: render + play in one call.
bool play_blocking(const Melody& melody);

// A reusable player. Construct once, play many sounds.
//
// One-at-a-time semantics: calling play(...) while a previous sound
// is still going waits for the previous one to finish, then starts
// the new one. That keeps the API trivial without surprising overlap.
class Player {
public:
    Player();
    ~Player();

    Player(const Player&)            = delete;
    Player& operator=(const Player&) = delete;
    Player(Player&&)                 = delete;
    Player& operator=(Player&&)      = delete;

    // Returns false if device init failed.
    [[nodiscard]] bool play(const RenderedAudio& audio);
    [[nodiscard]] bool play(const Melody& melody);

    // True between a play() call returning and the audio finishing.
    [[nodiscard]] bool is_playing() const noexcept;

    // Block until the current play (if any) finishes.
    void wait();

    // Cancel in-flight playback. Safe to call at any time.
    void cancel() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

}  // namespace collab::melody
