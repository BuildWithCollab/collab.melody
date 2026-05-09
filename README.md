# 🎵 collab.melody

> Declarative little melodies and notification sounds for C++23 apps. Define them in JSON. Tweak them without recompiling. Play them with one line.

```cpp
import collab.melody;

auto melody = collab::melody::load_from_json_file("melodies/doorbell.json");
collab::melody::play_blocking(melody);
```

That's the whole API for the common case. 🎯

---

## Contents

- [What's in the box](#whats-in-the-box)
- [The voice kinds](#the-voice-kinds)
- [JSON shape](#json-shape)
- [Public API](#public-api)
  - [`collab.melody.voices`](#collabmelodyvoices)
  - [`collab.melody.melody`](#collabmelodymelody)
  - [`collab.melody.render`](#collabmelodyrender)
  - [`collab.melody.io`](#collabmelodyio)
  - [`collab.melody.player`](#collabmelodyplayer)
  - [`collab.melody`](#collabmelody)
- [C++ authoring](#c-authoring)
- [Bundle a folder of melodies in your app](#bundle-a-folder-of-melodies-in-your-app)
- [Building](#building)
- [Dependencies](#dependencies)
- [License](#license)

---

## What's in the box

| Module | Role |
|---|---|
| `collab.melody.voices`  | Voice structs (`ToneVoice`, `PianoVoice`, `DecayVoice`, …) + the `Voice` variant |
| `collab.melody.melody`  | The `Melody` struct: a name + a list of voices |
| `collab.melody.render`  | Renders a `Melody` to `RenderedAudio` (PCM int16 mono @ 44.1 kHz) |
| `collab.melody.io`      | JSON load/save + directory-of-melodies bulk loader |
| `collab.melody.player`  | `play_blocking(...)` and a reusable `Player` class (miniaudio) |
| `collab.melody`         | Umbrella — re-exports everything |

Most apps just `import collab.melody;` and never touch the submodules.

---

## The voice kinds

A **melody** is a list of voices on a shared timeline. Each voice has a `kind` field that tells the loader which struct to deserialize. Voices overlap freely — set `start_ms` to anywhere, durations can collide.

| `kind`     | What it sounds like                         | Knobs |
|------------|---------------------------------------------|-------|
| `tone`     | Steady tone (sine / square / triangle / saw) with edge fades | `freq_hz`, `duration_ms`, `wave`, `fade_ms`, `volume` |
| `glide`    | Smooth pitch slide (boop / phew / sweep)    | `from_hz`, `to_hz`, `duration_ms`, `wave`, `fade_ms`, `volume` |
| `piano`    | Piano-ish: attack swell + exponential decay + harmonic stack | `freq_hz`, `attack_ms`, `tau_ms`, `volume`, `harmonics` |
| `tremolo`  | Tone with amplitude LFO (pulsing)           | `freq_hz`, `duration_ms`, `lfo_hz`, `depth`, `volume` |
| `vibrato`  | Tone with frequency LFO (wobble)            | `freq_hz`, `duration_ms`, `lfo_hz`, `depth_semitones`, `volume` |
| `decay`    | Pure sine bell — exponential decay only     | `freq_hz`, `duration_ms`, `tau_ms`, `volume` |
| `silence`  | Documents an intentional gap                | `duration_ms` |

Every voice has a `start_ms` (default 0). Every numeric field has a sensible default — JSON only needs to mention what differs.

---

## JSON shape

```jsonc
{
  "name": "doorbell",
  "voices": [
    { "kind": "decay", "start_ms":   0, "freq_hz": 660, "duration_ms": 1000, "tau_ms": 350, "volume": 0.5 },
    { "kind": "decay", "start_ms": 250, "freq_hz": 520, "duration_ms": 1300, "tau_ms": 450, "volume": 0.5 }
  ]
}
```

Fifteen reference melodies ship under `melodies/`. Drop them in, copy them, mutate them, replace them — they're just data.

---

## Public API

All declarations live in `namespace collab::melody`. Most apps only need `import collab.melody;` (the umbrella) and never name a submodule directly.

### `collab.melody.voices`

Data types — voice structs plus the `Voice` variant. Each voice's `kind` member defaults to its label and is the discriminator used by JSON round-trip.

```cpp
namespace collab::melody {

enum class Wave { sine, square, triangle, sawtooth };

struct ToneVoice {
    std::string kind        = "tone";
    int         start_ms    = 0;
    double      freq_hz     = 440.0;
    int         duration_ms = 500;
    Wave        wave        = Wave::sine;
    int         fade_ms     = 8;
    double      volume      = 0.6;
};

struct GlideVoice {
    std::string kind        = "glide";
    int         start_ms    = 0;
    double      from_hz     = 220.0;
    double      to_hz       = 880.0;
    int         duration_ms = 800;
    Wave        wave        = Wave::sine;
    int         fade_ms     = 8;
    double      volume      = 0.6;
};

struct PianoVoice {
    std::string         kind        = "piano";
    int                 start_ms    = 0;
    double              freq_hz     = 440.0;
    int                 attack_ms   = 5;
    int                 tau_ms      = 500;
    double              volume      = 0.4;
    int                 duration_ms = 0;             // 0 = derive from attack + ~5*tau
    std::vector<double> harmonics   = {1.0, 0.4, 0.2};
};

struct TremoloVoice {
    std::string kind        = "tremolo";
    int         start_ms    = 0;
    double      freq_hz     = 440.0;
    int         duration_ms = 1500;
    double      lfo_hz      = 6.0;
    double      depth       = 0.7;                   // 0..1, fraction the volume dips by
    int         fade_ms     = 8;
    double      volume      = 0.6;
};

struct VibratoVoice {
    std::string kind            = "vibrato";
    int         start_ms        = 0;
    double      freq_hz         = 440.0;
    int         duration_ms     = 1500;
    double      lfo_hz          = 6.0;
    double      depth_semitones = 0.5;               // pitch wobble in semitones
    int         fade_ms         = 8;
    double      volume          = 0.6;
};

struct DecayVoice {
    std::string kind        = "decay";
    int         start_ms    = 0;
    double      freq_hz     = 880.0;
    int         duration_ms = 1200;
    int         tau_ms      = 200;                   // 1/e amplitude every tau_ms
    int         fade_in_ms  = 2;
    double      volume      = 0.7;
};

struct SilenceVoice {
    std::string kind        = "silence";
    int         start_ms    = 0;
    int         duration_ms = 100;
};

using Voice = std::variant<
    ToneVoice, GlideVoice, PianoVoice,
    TremoloVoice, VibratoVoice, DecayVoice, SilenceVoice
>;

// nlohmann ADL hooks — fire automatically when a Melody round-trips
// through def_type. Apps don't typically call these directly.
void to_json  (nlohmann::json& j, const Voice& v);
void from_json(const nlohmann::json& j, Voice& v);

}  // namespace collab::melody
```

### `collab.melody.melody`

The top-level container — a name and a list of voices on a shared timeline.

```cpp
namespace collab::melody {

struct Melody {
    std::string        name;
    int                duration_ms = 0;   // 0 = derive from voices
    std::vector<Voice> voices;
};

}
```

### `collab.melody.render`

Mix every voice into a single PCM buffer. int32 accumulator internally so heavy polyphony doesn't smear per-voice clipping into the result.

```cpp
namespace collab::melody {

constexpr int kSampleRate = 44100;        // mono int16 PCM @ 44.1 kHz

struct RenderedAudio {
    std::vector<std::int16_t> samples;
    int                       sample_rate = kSampleRate;
};

// One voice's audible length. PianoVoice with duration_ms=0 is auto-
// derived from attack + 5*tau; everything else returns its duration_ms.
[[nodiscard]] int voice_duration_ms(const Voice& voice);

// max(start_ms + duration_ms) across all voices, or melody.duration_ms
// if non-zero.
[[nodiscard]] int derive_duration_ms(const Melody& melody);

// The thing you usually call.
[[nodiscard]] RenderedAudio render(const Melody& melody);

}
```

### `collab.melody.io`

JSON load/save plus bulk-loading a folder of melodies.

```cpp
namespace collab::melody {

// Throw std::runtime_error with a useful message on file IO / parse errors.
[[nodiscard]] Melody load_from_json_file  (const std::filesystem::path& path);
[[nodiscard]] Melody load_from_json_string(std::string_view json);

// indent < 0  → compact;  indent >= 0  → pretty-printed with that many spaces.
[[nodiscard]] std::string to_json_string(const Melody& melody, int indent = 2);

void save_to_json_file(const Melody& melody, const std::filesystem::path& path);

// Load every *.json in `dir` (non-recursive), keyed by file stem.
// Failures are reported via the callback (if supplied); other files still load.
// Empty / missing dir returns empty map — never throws.
[[nodiscard]] std::map<std::string, Melody> load_all_from_directory(
    const std::filesystem::path& dir,
    std::function<void(const std::filesystem::path&, std::string_view error)> error_callback = {}
);

}
```

### `collab.melody.player`

miniaudio-backed playback. Two flavors — one-shot blocking, or a reusable `Player` that keeps a device warm across many sounds.

```cpp
namespace collab::melody {

// Open device, play to completion, close. Returns false if device init failed.
bool play_blocking(const RenderedAudio& audio);
bool play_blocking(const Melody& melody);          // = play_blocking(render(melody))

// Reusable player. One sound at a time — calling play() while another is
// playing waits for the previous one to finish, then starts the new one.
class Player {
public:
    Player();
    ~Player();

    Player(const Player&)            = delete;
    Player& operator=(const Player&) = delete;
    Player(Player&&)                 = delete;
    Player& operator=(Player&&)      = delete;

    [[nodiscard]] bool play(const RenderedAudio& audio);
    [[nodiscard]] bool play(const Melody& melody);

    [[nodiscard]] bool is_playing() const noexcept;

    void wait();              // block until the current play (if any) finishes
    void cancel() noexcept;   // stop in-flight playback; safe from any thread
};

}
```

### `collab.melody`

Umbrella module. Just re-exports every submodule:

```cpp
export module collab.melody;
export import collab.melody.voices;
export import collab.melody.melody;
export import collab.melody.render;
export import collab.melody.io;
export import collab.melody.player;
```

---

## C++ authoring

```cpp
import collab.melody;

using namespace collab::melody;

Melody chord{
    .name = "chord",
    .voices = {
        PianoVoice{ .start_ms =   0, .freq_hz = 523.25, .attack_ms =  3, .tau_ms = 600, .volume = 0.45 },
        PianoVoice{ .start_ms = 180, .freq_hz = 659.25, .attack_ms =  6, .tau_ms = 600, .volume = 0.45 },
        PianoVoice{ .start_ms = 360, .freq_hz = 783.99, .attack_ms = 12, .tau_ms = 600, .volume = 0.45 },
    }
};

play_blocking(chord);                       // one-shot
save_to_json_file(chord, "chord.json");     // round-trip back to JSON
```

For apps that fire many notifications, hold a `Player` once instead of opening / closing the device each time:

```cpp
import collab.melody;

collab::melody::Player player;
player.play(collab::melody::load_from_json_file("error.json"));
player.wait();
player.play(collab::melody::load_from_json_file("retry.json"));
player.wait();
```

---

## Bundle a folder of melodies in your app

```cpp
import collab.melody;

auto melodies = collab::melody::load_all_from_directory("./sounds",
    [](const auto& path, std::string_view err) {
        std::cerr << "skipping " << path << ": " << err << '\n';
    });

collab::melody::play_blocking(melodies.at("error"));
```

Empty / missing directory returns empty map — never throws.

---

## Building

```bash
# Windows (first session)
xmake f --qt=C:/qt/6.10.2/msvc2022_64 -m release -p windows -a x64 -c -y
xmake build -a

# Mac / Linux
xmake f -m release -c -y
xmake build -a
```

Tests:

```bash
xmake run tests-collab.melody
```

Demos:

```bash
xmake run example.play_melody melodies/chord.json
xmake run example.melody_demo                         # plays everything in melodies/
xmake run example.melody_demo --filter chord
xmake run example.melody_demo --list                  # don't play, just list
```

---

## Dependencies

- **[def_type](https://github.com/BuildWithCollab/Packages)** — JSON serialization via PFR-based reflection (BuildWithCollab registry).
- **[miniaudio](https://miniaud.io)** — playback backend (single-header, cross-platform).
- **[cli11](https://github.com/CLIUtils/CLI11)** — only for the bundled examples.
- **[Catch2](https://github.com/catchorg/Catch2)** — only for the test suite.

`miniaudio` is a hard dep today; if a use case ever shows up that wants offline-only synthesis (render to a WAV, pipe somewhere else), we can split the player out into a sibling library and drop miniaudio from the core.

---

## License

0BSD — see `LICENSE`. Use it however you want.

🏴‍☠️
