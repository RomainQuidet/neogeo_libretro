#include "mvs_dips.h"

// https://wiki.neogeodev.org/index.php?title=Hardware_DIPs
/*
 b0: settings mode (Bring up the system ROM menu at boot)
 b1: 0 = chute, 1 = chutes
 b2: 0 = normal controller, 1 = Mahjong controller
 b3-4: multiplayer comm. ID code
 b5: enable multiplayer
 b6: freeplay (no $$$ need)
 b7: freeze (pause game)
 */

uint8_t mvs_dips = 0xFF;
