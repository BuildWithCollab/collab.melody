# 🎵 collab.melody

> Declarative little melodies and notification sounds for C++23 apps. Define them in JSON. Tweak them without recompiling. Play them with one line.

```cpp
import collab.melody;

auto melody = collab::melody::load_from_json_file("melodies/doorbell.json");
collab::melody::play_blocking(melody);
```

That's the whole API for the common case. 🎯

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

## C++ authoring

```cpp
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
Player player;
player.play(load_from_json_file("error.json"));
player.wait();
player.play(load_from_json_file("retry.json"));
player.wait();
```

---

## Bundle a folder of melodies in your app

```cpp
auto melodies = load_all_from_directory("./sounds",
    [](const auto& path, std::string_view err) {
        std::cerr << "skipping " << path << ": " << err << '\n';
    });

play_blocking(melodies.at("error"));
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
