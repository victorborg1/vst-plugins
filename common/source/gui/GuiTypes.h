#pragma once

namespace oscilleon::gui {

enum class LampState {
    OFF,
    ON,
    SYNC
};

enum class KnobMode {
    CONTINUOUS,
    DISCRETE
};

enum class KnobIndicatorStyle {
    NONE,
    BASIC,
    THREE_POINT,
    INTERMEDIATE,
    DISCRETE_STEPS
};

struct Indicator {
    float angle;
    bool dark;
};

}

