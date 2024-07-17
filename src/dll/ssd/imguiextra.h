#pragma once

#define IM_VEC2_CLASS_EXTRA                                                                                            \
    inline ImVec2 operator+(const ImVec2& v) const { return ImVec2(x + v.x, y + v.y); }                                \
                                                                                                                       \
    inline ImVec2 operator-(const ImVec2& v) const { return ImVec2(x - v.x, y - v.y); }                                \
                                                                                                                       \
    inline ImVec2 operator/(float f) const { return ImVec2(x / f, y / f); }                                            \
    inline ImVec2 operator*(float f) const { return ImVec2(x * f, y * f); }                                            \
                                                                                                                       \
    inline ImVec2 operator*(const ImVec2& v) const { return ImVec2(x * v.x, y * v.y); }

#define IM_VEC4_CLASS_EXTRA                                                                                            \
    inline ImVec4 operator*(const ImVec4& v) const { return ImVec4(x * v.x, y * v.y, z * v.z, w * v.w); }              \
                                                                                                                       \
    inline ImVec4 operator*(float f) const { return ImVec4(x * f, y * f, z * f, w * f); }
