#ifndef QUICKGUI_PALETTES_HPP_
#define QUICKGUI_PALETTES_HPP_

#include <type_traits>
#include "quickgui/color.hpp"

namespace quickgui {

#define _declare_values()                               \
    static ucolor get(uint32_t pos) noexcept { return values[pos % (uint32_t)C4_COUNTOF(values)]; } \
    constexpr inline static const ucolor values[] =
#define _(r,g,b) ucolor(UINT8_C(0x##r),UINT8_C(0x##g),UINT8_C(0x##b))


/*
 * see https://docs.bokeh.org/en/2.4.0/docs/reference/palettes.html
 */
namespace palettes {

/** https://github.com/d3/d3-3.x-api-reference/blob/master/Ordinal-Scales.md#category10  */
struct d3_10
{
    _declare_values() {
        _(1f,77,b4),
        _(ff,7f,0e),
        _(2c,a0,2c),
        _(d6,27,28),
        _(94,67,bd),
        _(8c,56,4b),
        _(e3,77,c2),
        _(7f,7f,7f),
        _(bc,bd,22),
        _(17,be,cf)
    };
};

/** https://github.com/d3/d3-3.x-api-reference/blob/master/Ordinal-Scales.md#category20  */
struct d3_20a
{
    _declare_values() {
        _(1f,77,b4),
        _(ae,c7,e8),
        _(ff,7f,0e),
        _(ff,bb,78),
        _(2c,a0,2c),
        _(98,df,8a),
        _(d6,27,28),
        _(ff,98,96),
        _(94,67,bd),
        _(c5,b0,d5),
        _(8c,56,4b),
        _(c4,9c,94),
        _(e3,77,c2),
        _(f7,b6,d2),
        _(7f,7f,7f),
        _(c7,c7,c7),
        _(bc,bd,22),
        _(db,db,8d),
        _(17,be,cf),
        _(9e,da,e5),
    };
};

/** https://github.com/d3/d3-3.x-api-reference/blob/master/Ordinal-Scales.md#category20b  */
struct d3_20b
{
    _declare_values() {
        _(39,3b,79),
        _(52,54,a3),
        _(6b,6e,cf),
        _(9c,9e,de),
        _(63,79,39),
        _(8c,a2,52),
        _(b5,cf,6b),
        _(ce,db,9c),
        _(8c,6d,31),
        _(bd,9e,39),
        _(e7,ba,52),
        _(e7,cb,94),
        _(84,3c,39),
        _(ad,49,4a),
        _(d6,61,6b),
        _(e7,96,9c),
        _(7b,41,73),
        _(a5,51,94),
        _(ce,6d,bd),
        _(de,9e,d6),
    };
};

/** https://github.com/d3/d3-3.x-api-reference/blob/master/Ordinal-Scales.md#category20c  */
struct d3_20c
{
    _declare_values() {
        _(31,82,bd),
        _(6b,ae,d6),
        _(9e,ca,e1),
        _(c6,db,ef),
        _(e6,55,0d),
        _(fd,8d,3c),
        _(fd,ae,6b),
        _(fd,d0,a2),
        _(31,a3,54),
        _(74,c4,76),
        _(a1,d9,9b),
        _(c7,e9,c0),
        _(75,6b,b1),
        _(9e,9a,c8),
        _(bc,bd,dc),
        _(da,da,eb),
        _(63,63,63),
        _(96,96,96),
        _(bd,bd,bd),
        _(d9,d9,d9),
    };
};

/** a concatenation of d3_20, d3_20b and d3_20a */
struct d3_60
{
    _declare_values() {
        // 20
        _(1f,77,b4),
        _(ae,c7,e8),
        _(ff,7f,0e),
        _(ff,bb,78),
        _(2c,a0,2c),
        _(98,df,8a),
        _(d6,27,28),
        _(ff,98,96),
        _(94,67,bd),
        _(c5,b0,d5),
        _(8c,56,4b),
        _(c4,9c,94),
        _(e3,77,c2),
        _(f7,b6,d2),
        _(7f,7f,7f),
        _(c7,c7,c7),
        _(bc,bd,22),
        _(db,db,8d),
        _(17,be,cf),
        _(9e,da,e5),
        // 20b
        _(39,3b,79),
        _(52,54,a3),
        _(6b,6e,cf),
        _(9c,9e,de),
        _(63,79,39),
        _(8c,a2,52),
        _(b5,cf,6b),
        _(ce,db,9c),
        _(8c,6d,31),
        _(bd,9e,39),
        _(e7,ba,52),
        _(e7,cb,94),
        _(84,3c,39),
        _(ad,49,4a),
        _(d6,61,6b),
        _(e7,96,9c),
        _(7b,41,73),
        _(a5,51,94),
        _(ce,6d,bd),
        _(de,9e,d6),
        // 20c
        _(31,82,bd),
        _(6b,ae,d6),
        _(9e,ca,e1),
        _(c6,db,ef),
        _(e6,55,0d),
        _(fd,8d,3c),
        _(fd,ae,6b),
        _(fd,d0,a2),
        _(31,a3,54),
        _(74,c4,76),
        _(a1,d9,9b),
        _(c7,e9,c0),
        _(75,6b,b1),
        _(9e,9a,c8),
        _(bc,bd,dc),
        _(da,da,eb),
        _(63,63,63),
        _(96,96,96),
        _(bd,bd,bd),
        _(d9,d9,d9),
    };
};

} // namespace palettes
#undef _
#undef _declare_values

} // namespace quickgui

#endif /* QUICKGUI_PALETTES_HPP_ */
