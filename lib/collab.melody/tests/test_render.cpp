// Render exercises — verify the pipeline produces sane buffers for each
// voice kind without actually opening an audio device. Tests are about
// shape (sample counts, sample rate, non-emptiness) and crashes — not
// audible quality. Audible quality is ear-tested via the example CLIs.

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>

import collab.melody;

using namespace collab::melody;

TEST_CASE("render of empty melody returns empty audio", "[render]") {
    Melody m{ .name = "empty" };
    const auto out = render(m);
    CHECK(out.samples.empty());
    CHECK(out.sample_rate == kSampleRate);
}

TEST_CASE("render produces sample_rate * duration / 1000 samples", "[render]") {
    Melody m{
        .name = "single tone",
        .voices = { ToneVoice{ .start_ms = 0, .duration_ms = 1000 } }
    };
    const auto out = render(m);
    // 44100 samples at 1000 ms.
    CHECK(out.samples.size() == static_cast<std::size_t>(kSampleRate));
    CHECK(out.sample_rate    == kSampleRate);
}

TEST_CASE("render auto-derives total duration from latest voice end", "[render]") {
    Melody m{
        .name = "spaced",
        .voices = {
            ToneVoice{ .start_ms =   0, .duration_ms = 100 },
            ToneVoice{ .start_ms = 500, .duration_ms = 100 },
        }
    };
    // Total should be 500 + 100 = 600 ms.
    const auto out = render(m);
    CHECK(out.samples.size() == static_cast<std::size_t>(kSampleRate * 600 / 1000));
}

TEST_CASE("render respects melody.duration_ms when set", "[render]") {
    Melody m{
        .name        = "padded",
        .duration_ms = 2000,
        .voices = { ToneVoice{ .duration_ms = 100 } }
    };
    const auto out = render(m);
    CHECK(out.samples.size() == static_cast<std::size_t>(kSampleRate * 2));
}

TEST_CASE("render handles each voice kind without crashing", "[render][kinds]") {
    SECTION("ToneVoice")    { CHECK_NOTHROW(render(Melody{ .voices = { ToneVoice{}    } })); }
    SECTION("GlideVoice")   { CHECK_NOTHROW(render(Melody{ .voices = { GlideVoice{}   } })); }
    SECTION("PianoVoice")   { CHECK_NOTHROW(render(Melody{ .voices = { PianoVoice{}   } })); }
    SECTION("TremoloVoice") { CHECK_NOTHROW(render(Melody{ .voices = { TremoloVoice{} } })); }
    SECTION("VibratoVoice") { CHECK_NOTHROW(render(Melody{ .voices = { VibratoVoice{} } })); }
    SECTION("DecayVoice")   { CHECK_NOTHROW(render(Melody{ .voices = { DecayVoice{}   } })); }
    SECTION("SilenceVoice") { CHECK_NOTHROW(render(Melody{ .voices = { SilenceVoice{} } })); }
}

TEST_CASE("render produces non-zero samples for audible voices", "[render]") {
    Melody m{ .voices = { ToneVoice{ .freq_hz = 440.0, .duration_ms = 200 } } };
    const auto out = render(m);
    REQUIRE(!out.samples.empty());

    bool any_nonzero = false;
    for (auto s : out.samples) if (s != 0) { any_nonzero = true; break; }
    CHECK(any_nonzero);
}

TEST_CASE("PianoVoice with duration_ms=0 auto-derives length", "[render][piano]") {
    Melody m{
        .voices = { PianoVoice{ .attack_ms = 5, .tau_ms = 100, .duration_ms = 0 } }
    };
    const auto out = render(m);
    // 5 + 5*100 = 505 ms minimum
    CHECK(out.samples.size() >= static_cast<std::size_t>(kSampleRate * 500 / 1000));
}

TEST_CASE("Overlapping voices don't crash and produce mixed output", "[render][polyphony]") {
    Melody m{
        .name = "chord",
        .voices = {
            PianoVoice{ .start_ms =   0, .freq_hz = 523.25, .tau_ms = 400, .duration_ms = 500 },
            PianoVoice{ .start_ms =   0, .freq_hz = 659.25, .tau_ms = 400, .duration_ms = 500 },
            PianoVoice{ .start_ms =   0, .freq_hz = 783.99, .tau_ms = 400, .duration_ms = 500 },
        }
    };
    const auto out = render(m);
    REQUIRE(!out.samples.empty());
    // All voices started at 0, so first sample shouldn't be silence.
    bool any_early_nonzero = false;
    for (std::size_t i = 0; i < std::min<std::size_t>(out.samples.size(), 1000); ++i) {
        if (out.samples[i] != 0) { any_early_nonzero = true; break; }
    }
    CHECK(any_early_nonzero);
}

TEST_CASE("voice_duration_ms works for each kind", "[render][duration]") {
    CHECK(voice_duration_ms(Voice{ToneVoice    { .duration_ms = 123 }}) == 123);
    CHECK(voice_duration_ms(Voice{GlideVoice   { .duration_ms = 234 }}) == 234);
    CHECK(voice_duration_ms(Voice{TremoloVoice { .duration_ms = 345 }}) == 345);
    CHECK(voice_duration_ms(Voice{VibratoVoice { .duration_ms = 456 }}) == 456);
    CHECK(voice_duration_ms(Voice{DecayVoice   { .duration_ms = 567 }}) == 567);
    CHECK(voice_duration_ms(Voice{SilenceVoice { .duration_ms = 678 }}) == 678);

    // PianoVoice with explicit duration honors it.
    CHECK(voice_duration_ms(Voice{PianoVoice{ .duration_ms = 789 }}) == 789);
    // PianoVoice with duration_ms=0 derives from attack + 5*tau.
    CHECK(voice_duration_ms(Voice{PianoVoice{ .attack_ms = 10, .tau_ms = 100, .duration_ms = 0 }}) == 510);
}
