#pragma once
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>
#include "ini.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"
#include <glad/glad.h>
#ifdef _WIN32
#include <glfw/glfw3.h>
#else
#include <GLFW/glfw3.h>
#endif

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

namespace natsukashii::util
{
template <typename T>
static constexpr bool bit(T num, u8 pos)
{
  return (num >> pos) & 1;
}

template <typename T, u8 pos>
static constexpr bool bit(T num)
{
  return (num >> pos) & 1;
}

template <typename T>
void setbit(T& num, u8 pos, bool val)
{
  num ^= (-(!!val) ^ num) & (1 << pos);
}

template <typename T, u8 pos>
void setbit(T& num, bool val)
{
  num ^= (-(!!val) ^ num) & (1 << pos);
}
} // natsukashii::util