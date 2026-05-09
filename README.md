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

- [Designing sounds with a coding agent](#designing-sounds-with-a-coding-agent)
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

## Designing sounds with a coding agent

Don't want to fiddle with `freq_hz` and `tau_ms` yourself? **Point your coding agent at [`llms.txt`](./llms.txt) at the repo root.** It's a self-contained guide that teaches an agent the JSON schema, the musical heuristics ("happy → ascending major intervals", "Apple-style → piano voices with staggered chord bloom"), and the preview workflow.

The collaboration loop:

1. You: "make me a friendly notification sound — short, warm, not jarring"
2. Agent writes a JSON melody, runs `xmake run example.play_melody …` so you hear it
3. You: "the second note is too sharp, soften the attack"
4. Agent edits, plays again
5. Repeat until it's right, then save under a real filename in `melodies/`

You listen, the agent does the typing and the speaker-driving. 🤖🔊

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

All declarations live in `namespace collab::melody`. Most apps only need `import collab.melody;` (the umbrella) — submodules below are listed for reference.

### `collab.melody.voices`

The data model — a `Wave` enum, seven voice structs, and the `Voice` variant tying them together.

`enum class Wave` — `sine`, `square`, `triangle`, `sawtooth`.

Every voice struct carries a `kind` discriminator (`std::string`, defaults to the kind's label — you don't usually set it manually) and a `start_ms` (`int`, default `0`) for timeline placement. The kind-specific fields:

#### `ToneVoice` — steady tone with edge fades

| Field | Type | Default | Meaning |
|-|-|-|-|
| `freq_hz` | `double` | `440` | pitch |
| `duration_ms` | `int` | `500` | length |
| `wave` | `Wave` | `sine` | waveform |
| `fade_ms` | `int` | `8` | linear fade-in / fade-out at edges (anti-click) |
| `volume` | `double` | `0.6` | 0..1 |

#### `GlideVoice` — smooth pitch slide

| Field | Type | Default | Meaning |
|-|-|-|-|
| `from_hz` | `double` | `220` | starting pitch |
| `to_hz` | `double` | `880` | ending pitch |
| `duration_ms` | `int` | `800` | length |
| `wave` | `Wave` | `sine` | waveform |
| `fade_ms` | `int` | `8` | edge fade |
| `volume` | `double` | `0.6` | 0..1 |

#### `PianoVoice` — attack swell + exponential decay + harmonic stack

| Field | Type | Default | Meaning |
|-|-|-|-|
| `freq_hz` | `double` | `440` | fundamental pitch |
| `attack_ms` | `int` | `5` | rise time (small = hammered, large = swelled) |
| `tau_ms` | `int` | `500` | exponential decay time constant |
| `volume` | `double` | `0.4` | 0..1 |
| `duration_ms` | `int` | `0` | total length; `0` derives from `attack + 5*tau` |
| `harmonics` | `std::vector<double>` | `{1.0, 0.4, 0.2}` | per-partial amplitudes (1st, 2nd, 3rd harmonic, …) |

#### `TremoloVoice` — amplitude LFO (pulsing)

| Field | Type | Default | Meaning |
|-|-|-|-|
| `freq_hz` | `double` | `440` | pitch |
| `duration_ms` | `int` | `1500` | length |
| `lfo_hz` | `double` | `6` | LFO rate |
| `depth` | `double` | `0.7` | 0..1, fraction the volume dips by |
| `fade_ms` | `int` | `8` | edge fade |
| `volume` | `double` | `0.6` | 0..1 |

#### `VibratoVoice` — frequency LFO (wobble)

| Field | Type | Default | Meaning |
|-|-|-|-|
| `freq_hz` | `double` | `440` | center pitch |
| `duration_ms` | `int` | `1500` | length |
| `lfo_hz` | `double` | `6` | LFO rate |
| `depth_semitones` | `double` | `0.5` | pitch wobble in semitones |
| `fade_ms` | `int` | `8` | edge fade |
| `volume` | `double` | `0.6` | 0..1 |

#### `DecayVoice` — pure sine bell

| Field | Type | Default | Meaning |
|-|-|-|-|
| `freq_hz` | `double` | `880` | pitch |
| `duration_ms` | `int` | `1200` | total length |
| `tau_ms` | `int` | `200` | 1/e amplitude every `tau_ms` |
| `fade_in_ms` | `int` | `2` | brief attack ramp |
| `volume` | `double` | `0.7` | 0..1 |

#### `SilenceVoice` — explicit timeline gap

| Field | Type | Default | Meaning |
|-|-|-|-|
| `duration_ms` | `int` | `100` | gap length |

Renders to silence (which is what gaps already produce) — useful for documenting intent in JSON.

#### `Voice`

`std::variant<ToneVoice, GlideVoice, PianoVoice, TremoloVoice, VibratoVoice, DecayVoice, SilenceVoice>`. Inspect with `std::holds_alternative<T>` / `std::get<T>`, or dispatch with `std::visit`. JSON round-trip is automatic — the `kind` field discriminates on deserialize.

ADL hooks `to_json(nlohmann::json&, const Voice&)` and `from_json(const nlohmann::json&, Voice&)` are exported but apps don't usually call them directly — they fire when a `Melody` round-trips through `def_type`.

---

### `collab.melody.melody`

#### `Melody`

| Field | Type | Default | Meaning |
|-|-|-|-|
| `name` | `std::string` | `""` | human-readable label |
| `duration_ms` | `int` | `0` | total length; `0` derives from voices |
| `voices` | `std::vector<Voice>` | `{}` | the voices on the timeline |

---

### `collab.melody.render`

Mix every voice in a `Melody` into a single PCM buffer. Internally accumulates in int32 so heavy polyphony doesn't smear per-voice clipping into the result.

#### `RenderedAudio`

| Field | Type | Default | Meaning |
|-|-|-|-|
| `samples` | `std::vector<std::int16_t>` | `{}` | mono PCM samples |
| `sample_rate` | `int` | `kSampleRate` (44100) | rate the buffer was rendered at |

#### Functions

- **`constexpr int kSampleRate = 44100`** — the rate every render is produced at (mono int16).
- **`int voice_duration_ms(const Voice&)`** — audible length of one voice. `PianoVoice` with `duration_ms = 0` returns `attack + 5*tau`; every other voice returns its `duration_ms`.
- **`int derive_duration_ms(const Melody&)`** — total length the buffer needs. Returns `melody.duration_ms` if non-zero, otherwise `max(start_ms + duration_ms)` across voices.
- **`RenderedAudio render(const Melody&)`** — the thing you usually call.

---

### `collab.melody.io`

JSON load/save plus bulk-loading a folder. All loaders throw `std::runtime_error` with a descriptive message on IO or parse failure — the message includes the offending path.

- **`Melody load_from_json_file(const std::filesystem::path& path)`** — read a melody from a `.json` file.
- **`Melody load_from_json_string(std::string_view json)`** — parse a melody from an in-memory JSON string.
- **`std::string to_json_string(const Melody&, int indent = 2)`** — serialize. `indent < 0` is compact; `indent >= 0` is pretty-printed with that many spaces.
- **`void save_to_json_file(const Melody&, const std::filesystem::path& path)`** — pretty-print to a `.json` file. Creates parent directories as needed.
- **`std::map<std::string, Melody> load_all_from_directory(const std::filesystem::path& dir, std::function<void(const std::filesystem::path&, std::string_view error)> error_callback = {})`** — load every `*.json` in `dir` (non-recursive), keyed by file stem. Per-file failures go to `error_callback` if supplied; other files still load. Empty / missing dir returns empty map — never throws.

---

### `collab.melody.player`

miniaudio-backed playback. Two flavors — one-shot blocking, or a reusable `Player` that keeps a device warm across many sounds.

#### Free functions

- **`bool play_blocking(const RenderedAudio&)`** — open the device, play to completion, close. Returns `false` on device-init failure.
- **`bool play_blocking(const Melody&)`** — convenience: `play_blocking(render(melody))`.

#### `class Player`

A reusable player. One sound at a time — `play()` waits for any in-flight play to finish before starting the next. Non-copyable, non-movable.

| Method | Returns | Behavior |
|-|-|-|
| `Player()` / `~Player()` | — | construct / tear down (device opened lazily on first `play`) |
| `play(const RenderedAudio&)` | `bool` | start playback (waits for any in-flight); `false` on device-init failure |
| `play(const Melody&)` | `bool` | renders, then plays |
| `is_playing() const noexcept` | `bool` | `true` between `play()` returning and the audio finishing |
| `wait()` | `void` | block until current playback (if any) finishes |
| `cancel() noexcept` | `void` | stop in-flight playback; safe to call from any thread |

---

### `collab.melody`

Umbrella module — re-exports every submodule above. Most apps `import collab.melody;` and never name a submodule directly.

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
