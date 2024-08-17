/**
 * @file colors.h
 * @author Kyle Pelham
 * @brief Defines a set of colors for use in Prism.
 * 
 * @copyright Copyright (c) 2024
*/

#pragma once
#include "imgui.h"

namespace Prism::Colors
{
    constexpr auto accent           = IM_COL32(0, 120, 215, 255);        // Bright Blue
    constexpr auto highlight        = IM_COL32(255, 193, 7, 255);        // Amber
    constexpr auto niceBlue         = IM_COL32(52, 152, 219, 255);       // Light Sky Blue
    constexpr auto compliment       = IM_COL32(231, 76, 60, 255);        // Soft Red
    constexpr auto background       = IM_COL32(248, 249, 250, 255);      // Light Gray (almost white)
    constexpr auto backgroundDark   = IM_COL32(36, 37, 38, 255);         // Very Dark Gray
    constexpr auto titlebar         = IM_COL32(44, 62, 80, 255);         // Dark Blue-Gray
    constexpr auto titlebarBrighter = IM_COL32(52, 73, 94, 255);         // Steel Blue
    constexpr auto titlebarDarker   = IM_COL32(33, 37, 41, 255);         // Dark Charcoal
    constexpr auto propertyField    = IM_COL32(52, 73, 94, 255);         // Steel Blue
    constexpr auto text             = IM_COL32(33, 37, 41, 255);         // Dark Charcoal
    constexpr auto textBrighter     = IM_COL32(255, 255, 255, 255);      // White
    constexpr auto textDarker       = IM_COL32(87, 96, 111, 255);        // Dim Gray
    constexpr auto textError        = IM_COL32(231, 76, 60, 255);        // Soft Red (same as compliment for consistency)
    constexpr auto button           = IM_COL32(52, 52, 52, 200);         // Dark Gray
    constexpr auto buttonBrighter   = IM_COL32(52, 52, 52, 150);         // Dark Gray (less opaque)
    constexpr auto buttonDarker     = IM_COL32(60, 60, 60, 255);         // Dark Gray (more opaque)
    constexpr auto muted            = IM_COL32(189, 195, 199, 255);      // Light Gray
    constexpr auto groupHeader      = IM_COL32(52, 152, 219, 255);       // Light Sky Blue (same as niceBlue)
    constexpr auto selection        = IM_COL32(88, 101, 242, 255);       // Indigo
    constexpr auto selectionMuted   = IM_COL32(107, 115, 125, 255);      // Muted Dark Gray
    constexpr auto backgroundPopup  = IM_COL32(255, 255, 255, 255);      // White

}