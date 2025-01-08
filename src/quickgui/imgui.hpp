#ifndef QUICKGUI_GUI_IMGUI_HPP_
#define QUICKGUI_GUI_IMGUI_HPP_

#include "c4/error.hpp"

C4_SUPPRESS_WARNING_CLANG_PUSH
C4_SUPPRESS_WARNING_CLANG("-Wconversion")
C4_SUPPRESS_WARNING_CLANG("-Wsign-conversion")
C4_SUPPRESS_WARNING_CLANG("-Wdouble-promotion")
C4_SUPPRESS_WARNING_CLANG("-Wold-style-cast")
C4_SUPPRESS_WARNING_CLANG("-Wcast-qual")

C4_SUPPRESS_WARNING_GCC_PUSH
C4_SUPPRESS_WARNING_GCC("-Wconversion")
C4_SUPPRESS_WARNING_GCC("-Wsign-conversion")
C4_SUPPRESS_WARNING_GCC("-Wdouble-promotion")
C4_SUPPRESS_WARNING_GCC("-Wuseless-cast")
C4_SUPPRESS_WARNING_GCC("-Wfloat-equal")
C4_SUPPRESS_WARNING_GCC("-Wold-style-cast")
C4_SUPPRESS_WARNING_GCC("-Wcast-qual")

C4_SUPPRESS_WARNING_MSVC_PUSH
C4_SUPPRESS_WARNING_MSVC(4355) // 'this': used in base member initializer list
C4_SUPPRESS_WARNING_MSVC(5039) // 'qsort': pointer or reference to potentially throwing function passed to 'extern "C"' function under -EHc. Undefined behavior may occur if this function throws an exception.
C4_SUPPRESS_WARNING_MSVC(5219) // implicit conversion from 'int' to 'float', possible loss of data

#include "imgui.h"
#include "imgui_internal.h"
#ifdef QUICKGUI_BUILD_TESTS
#include <imgui_test_engine/imgui_te_engine.h>
#include <imgui_test_engine/imgui_te_ui.h>
#include <imgui_test_engine/imgui_te_context.h>
#endif
#include "implot.h"
#include "imgui_spinner.h"
#define IMGUIFS_NO_EXTRA_METHODS
#include "imguifilesystem.h"
#undef IMGUIFS_NO_EXTRA_METHODS

C4_SUPPRESS_WARNING_MSVC_POP
C4_SUPPRESS_WARNING_CLANG_POP
C4_SUPPRESS_WARNING_GCC_POP

#define QUICKGUI_DEFINE_IMVEC_OPERATOR(op)                                   \
C4_ALWAYS_INLINE C4_CONST ImVec2& operator op##= (ImVec2 &lhs, float rhs) noexcept { lhs.x += rhs; lhs.y += rhs; return lhs; } \
C4_ALWAYS_INLINE C4_CONST ImVec2  operator op (ImVec2 lhs, float rhs) noexcept { return ImVec2{lhs.x op rhs, lhs.y op rhs}; } \
C4_ALWAYS_INLINE C4_CONST ImVec2  operator op (float lhs, ImVec2 rhs) noexcept { return ImVec2{lhs op rhs.x, lhs op rhs.y}; } \
C4_ALWAYS_INLINE C4_CONST ImVec2  operator op (ImVec2 lhs, ImVec2 rhs) noexcept { return ImVec2{lhs.x op rhs.x, lhs.y op rhs.y}; }
QUICKGUI_DEFINE_IMVEC_OPERATOR(+)
QUICKGUI_DEFINE_IMVEC_OPERATOR(-)
QUICKGUI_DEFINE_IMVEC_OPERATOR(*)
QUICKGUI_DEFINE_IMVEC_OPERATOR(/)
#undef QUICKGUI_DEFINE_IMVEC_OPERATOR

C4_ALWAYS_INLINE C4_CONST float  dot(ImVec2 lhs, ImVec2 rhs) noexcept { return lhs.x * rhs.x + lhs.y * rhs.y; }
C4_ALWAYS_INLINE C4_CONST float  dot(ImVec2 vec) noexcept { return vec.x * vec.x + vec.y * vec.y; }
C4_ALWAYS_INLINE C4_CONST float  norm(ImVec2 vec) noexcept { return sqrtf(vec.x * vec.x + vec.y * vec.y); }
C4_ALWAYS_INLINE C4_CONST float  rnorm(ImVec2 vec) noexcept { return 1.f / sqrtf(vec.x * vec.x + vec.y * vec.y); }
C4_ALWAYS_INLINE C4_CONST ImVec2 versor(ImVec2 vec) noexcept { float rs = 1.f / sqrtf(vec.x * vec.x + vec.y * vec.y); return ImVec2{vec.x * rs, vec.y * rs}; }
C4_ALWAYS_INLINE C4_CONST float  maxof(ImVec2 vec) noexcept { return vec.x > vec.y ? vec.x : vec.y; }
C4_ALWAYS_INLINE C4_CONST float  minof(ImVec2 vec) noexcept { return vec.x < vec.y ? vec.x : vec.y; }

C4_ALWAYS_INLINE C4_CONST ImVec2 shiftx(ImVec2 vec, float deltax) noexcept { return {vec.x + deltax, vec.y}; }
C4_ALWAYS_INLINE C4_CONST ImVec2 shifty(ImVec2 vec, float deltay) noexcept { return {vec.x, vec.y + deltay}; }


#endif /* QUICKGUI_GUI_IMGUI_HPP_ */
