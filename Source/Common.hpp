#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT _WIN32_WINNT_WIN7

#define BOSS_CHR_ARRAY_LEN 3
#define ENTITY_CHR_ARRAY_LEN 8

#include <winsdkver.h>
#include <sdkddkver.h>
#include <windows.h>
#include <windowsx.h>
#include <winternl.h>
#include <psapi.h>

#include <dxgi1_4.h>
#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

#include <DirectXMath.h>

#include <DirectXPackedVector.h>
#include <Psapi.h>
#include <TlHelp32.h>

#include <cassert>
#include <cinttypes>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <algorithm>
#include <atomic>
#include <bitset>
#include <chrono>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <source_location>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <stack>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <array>
#include <regex>

#include "Minhook/MinHook.h"
#include "ImGui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx12.h"
#include "ImGui/imgui_impl_win32.h"
#include <map>

using IndexType = uint32_t;

extern void Nop(BYTE* dst, unsigned int size);
extern void Patch(BYTE* dst, BYTE* src, unsigned int size);

template<typename T, size_t N>
constexpr auto make_array(T value) -> std::array<T, N>
{
    std::array<T, N> a{};
    for (auto& x : a)
        x = value;
    return a;
}

template <typename EnumType, typename Value>
std::map<EnumType, Value> make_map() {
    std::map<EnumType, Value> m;
    for (size_t i = 0; i < static_cast<size_t>(EnumType::MAX); ++i)
        m[static_cast<EnumType>(i)] = Value();
    return m;
}

inline std::vector<std::string> splitString(const std::string& inputString, const std::string& delimiter)
{
    std::regex re(delimiter);
    std::sregex_token_iterator first{ inputString.begin(), inputString.end(), re, -1 }, last;//the '-1' is what makes the regex split (-1 := what was not matched)
    return { first, last };
}

namespace std
{
    inline std::string to_string(const ImVec2& vec2)
    {
        return "(" + std::to_string(vec2.x) + ", " + std::to_string(vec2.y) + ")";
    }

    inline std::string to_string(const ImVec4& vec4)
    {
        return "(" + std::to_string(vec4.x) + ", " + std::to_string(vec4.y) + ", " + std::to_string(vec4.z) + ", " + std::to_string(vec4.w) + ")";
    }

    template<typename T>
    inline std::string to_string(const std::pair<T, T> pair)
    {
        return "{ " + std::to_string(pair.first) + ", " + std::to_string(pair.second) + " }";
    }
};

namespace ER {

    using namespace std::chrono_literals;
    using namespace DirectX;
    using namespace DirectX::PackedVector;
    namespace fs = std::filesystem;

    template<typename T> inline T RPM(uint64_t lpBaseAddress)
    {
        __try
        {
            T A{};
            A = *(T*)(lpBaseAddress);
            return A;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return T{};
        }
    }

    template<typename T> inline void WPM(uint64_t lpBaseAddress, T Val)
    {
        __try
        {
            *(T*)(lpBaseAddress) = Val;
            return;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    static uint64_t dwGameBase = (uint64_t)GetModuleHandle(NULL);
}