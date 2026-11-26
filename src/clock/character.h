#pragma once

#include <iterator>

// These IDs are stored in the settings file.
enum ClockCharacterId { SkeaCharacter = 0, KommyCharacter = 1, MuteCharacter = 2, CoffeCharacter = 3 };

enum class ClockLoadResult { Loaded, AssetError, RuntimeError };

struct ClockCharacter {
    Color background;
    Color border = background;
};

const ClockCharacter ClockCharacters[] = {
    { Color(255, 0xDF, 0xF7, 0xF5), Color(255, 0x94, 0xD2, 0xD4) },
    { Color(255, 0xFF, 0xA9, 0xA5) },
    { Color(255, 0xBC, 0xE5, 0xDF), Color(255, 0x14, 0x7D, 0x73) },
    { Color(255, 0xE8, 0xC2, 0x9D), Color(255, 0xA6, 0x4D, 0x39) },
};
int selectedCharacter = SkeaCharacter;

bool IsCoffeCharacter() { return selectedCharacter == CoffeCharacter; }
bool IsKommyCharacter() { return selectedCharacter == KommyCharacter; }

bool IsMuteCharacter() { return selectedCharacter == MuteCharacter; }
