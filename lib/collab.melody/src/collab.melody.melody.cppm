// The top-level sound type. A `Melody` is just a name + a list of voices.
// Plain aggregate — PFR auto-discovers fields, def_type round-trips it
// through JSON for free.

module;

#include <string>
#include <vector>

export module collab.melody.melody;

import collab.melody.voices;

export namespace collab::melody {

struct Melody {
    std::string        name;
    int                duration_ms = 0;   // 0 = derive from voices
    std::vector<Voice> voices;
};

}  // namespace collab::melody
