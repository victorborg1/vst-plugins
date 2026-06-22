#pragma once
#include <variant>

namespace oscilleon::gui {

struct GridUnit {
    enum class Type {
        Pixels,
        Fraction,
        Percent
    };

    Type type;
    float value;

    static GridUnit Pixels(float px)    { return {Type::Pixels, px}; }
    static GridUnit Fraction(float fr)  { return {Type::Fraction, fr}; }
    static GridUnit Percent(float pc)   { return {Type::Percent, pc}; }
};

} // namespace oscilleon::gui
