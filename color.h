#pragma once

#include "tgaimage.h"
#include <string>

// Basic colors
constexpr TGAColor white   = {255, 255, 255, 255};
constexpr TGAColor black   = {  0,   0,   0, 255};
constexpr TGAColor gray    = {128, 128, 128, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor blue    = {255,   0,   0, 255};

// Bright and auxiliary colors
constexpr TGAColor yellow  = {  0, 255, 255, 255};
constexpr TGAColor cyan    = {255, 255,   0, 255};
constexpr TGAColor magenta = {255,   0, 255, 255};
constexpr TGAColor orange  = {  0, 165, 255, 255};
constexpr TGAColor purple  = {128,   0, 128, 255};
constexpr TGAColor pink    = {180, 105, 255, 255};

// Noble / Metallic colors (useful for lighting tests)
constexpr TGAColor gold    = {  0, 215, 255, 255};
constexpr TGAColor silver  = {192, 192, 192, 255};
constexpr TGAColor bronze  = { 63, 127, 205, 255};

// Natural colors
constexpr TGAColor sky     = {250, 206, 135, 255}; // Sky blue
constexpr TGAColor grass   = { 34, 139,  34, 255}; // Forest green
constexpr TGAColor wood    = { 19,  69, 139, 255}; // Brown

// Dark shades (good for shadows or background)
constexpr TGAColor dark_red   = {  0,   0, 139, 255};
constexpr TGAColor dark_blue  = {139,   0,   0, 255};
constexpr TGAColor dark_gray  = { 40,  40,  40, 255};

// Helper function to convert a string from the command line into a TGAColor
inline TGAColor get_color_by_name(const std::string& name) {
    if (name == "black") return black;
    if (name == "gray") return gray;
    if (name == "red") return red;
    if (name == "green") return green;
    if (name == "blue") return blue;
    if (name == "yellow") return yellow;
    if (name == "cyan") return cyan;
    if (name == "magenta") return magenta;
    if (name == "orange") return orange;
    if (name == "purple") return purple;
    if (name == "pink") return pink;
    if (name == "gold") return gold;
    if (name == "silver") return silver;
    if (name == "bronze") return bronze;
    if (name == "sky") return sky;
    if (name == "grass") return grass;
    if (name == "wood") return wood;
    if (name == "dark_red") return dark_red;
    if (name == "dark_blue") return dark_blue;
    if (name == "dark_gray") return dark_gray;
    return white; // Default fallback color
}