// 🎵 collab.melody — declarative audio synthesis for app notification sounds
//
// Define a sound as a list of voices on a timeline. Voices come in
// several kinds (tone, glide, piano, tremolo, vibrato, decay, silence)
// each with their own knobs. Render to PCM, play through speakers,
// load from / save to JSON.
//
//   import collab.melody;
//
//   auto melody = collab::melody::load_from_json_file("doorbell.json");
//   collab::melody::play_blocking(collab::melody::render(melody));
//
// The umbrella module re-exports every submodule. Most apps just
// import collab.melody and never touch the submodules directly.

export module collab.melody;

export import collab.melody.voices;
export import collab.melody.melody;
export import collab.melody.render;
export import collab.melody.io;
export import collab.melody.player;
