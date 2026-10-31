#include "comic2/level_data.hpp"

#include <array>

namespace comic2 {
namespace {

// ================= Level 0 Data =================
const std::vector<std::uint8_t> kLvl0TouchVals = {57, 80, 139};
const std::vector<std::uint8_t> kLvl0ProjVals = {139};
const std::vector<std::uint8_t> kLvl0Mode2Vals = {24};
const std::vector<std::vector<RoomTransitionZone>> kLvl0Zones = {
    /* room 0 */ {RoomTransitionZone{0, 0, 16, 0, 1, 0}, RoomTransitionZone{0, 48, 192, 0, 1, 1}, RoomTransitionZone{0, 224, 272, 0, 1, 2}, RoomTransitionZone{0, 304, 416, 0, 1, 3}},
    /* room 1 */ {RoomTransitionZone{1008, 0, 16, 0, 0, 0}, RoomTransitionZone{1008, 48, 192, 0, 0, 1}, RoomTransitionZone{1008, 224, 272, 0, 0, 2}, RoomTransitionZone{1008, 304, 416, 0, 0, 3}, RoomTransitionZone{0, 304, 432, 0, 2, 0}, RoomTransitionZone{0, 0, 256, 0, 2, 1}, RoomTransitionZone{992, 528, 560, 1, 0, 0}},
    /* room 2 */ {RoomTransitionZone{2096, 64, 192, 0, 1, 4}, RoomTransitionZone{2096, 0, 16, 0, 1, 5}},
    /* room 3 */ {},
    /* room 4 */ {},
    /* room 5 */ {},
    /* room 6 */ {},
    /* room 7 */ {},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl0Enemies = {
    /* room 0 */ {RoomEntitySpawnDef{560, 152, 32913, 0, 0, 0}, RoomEntitySpawnDef{528, 200, 32913, 0, 0, 0}, RoomEntitySpawnDef{592, 200, 32913, 0, 0, 0}, RoomEntitySpawnDef{736, 248, 32913, 0, 0, 0}, RoomEntitySpawnDef{896, 360, 32913, 0, 0, 0}, RoomEntitySpawnDef{992, 440, 32913, 0, 0, 0}, RoomEntitySpawnDef{256, 440, 32913, 0, 0, 0}, RoomEntitySpawnDef{176, 280, 32913, 0, 0, 0}, RoomEntitySpawnDef{592, 376, 32913, 0, 0, 0}, RoomEntitySpawnDef{464, 376, 32913, 0, 0, 0}, RoomEntitySpawnDef{528, 440, 32913, 0, 0, 0}},
    /* room 1 */ {RoomEntitySpawnDef{592, 280, 32913, 0, 0, 0}, RoomEntitySpawnDef{560, 312, 32913, 0, 0, 0}, RoomEntitySpawnDef{496, 376, 32913, 0, 0, 0}, RoomEntitySpawnDef{608, 136, 32913, 0, 0, 0}, RoomEntitySpawnDef{144, 72, 32913, 0, 0, 0}, RoomEntitySpawnDef{32, 72, 32913, 0, 0, 0}, RoomEntitySpawnDef{864, 440, 32913, 0, 0, 0}, RoomEntitySpawnDef{992, 352, 32961, 0, 0, 0}, RoomEntitySpawnDef{944, 384, 32961, 0, 0, 0}, RoomEntitySpawnDef{544, 80, 32961, 0, 0, 0}, RoomEntitySpawnDef{144, 560, 32945, 0, 0, 0}, RoomEntitySpawnDef{48, 560, 32945, 0, 0, 0}, RoomEntitySpawnDef{272, 560, 32945, 0, 0, 0}, RoomEntitySpawnDef{400, 560, 32945, 0, 0, 0}, RoomEntitySpawnDef{560, 576, 32945, 0, 0, 0}, RoomEntitySpawnDef{672, 608, 32945, 0, 0, 0}, RoomEntitySpawnDef{656, 528, 32945, 0, 0, 0}, RoomEntitySpawnDef{784, 560, 32945, 0, 0, 0}, RoomEntitySpawnDef{912, 544, 32945, 0, 0, 0}},
    /* room 2 */ {RoomEntitySpawnDef{288, 64, 32961, 0, 0, 0}, RoomEntitySpawnDef{256, 112, 32961, 0, 0, 0}, RoomEntitySpawnDef{288, 160, 32961, 0, 0, 0}, RoomEntitySpawnDef{64, 160, 32961, 0, 0, 0}, RoomEntitySpawnDef{112, 160, 32961, 0, 0, 0}, RoomEntitySpawnDef{80, 128, 32929, 0, 0, 0}, RoomEntitySpawnDef{112, 128, 32929, 0, 0, 0}, RoomEntitySpawnDef{144, 128, 32929, 0, 0, 0}, RoomEntitySpawnDef{1472, 128, 32929, 0, 0, 0}, RoomEntitySpawnDef{1520, 80, 32929, 0, 0, 0}, RoomEntitySpawnDef{1552, 80, 32929, 0, 0, 0}, RoomEntitySpawnDef{1792, 80, 32929, 0, 0, 0}, RoomEntitySpawnDef{1808, 80, 32929, 0, 0, 0}, RoomEntitySpawnDef{1824, 80, 32929, 0, 0, 0}, RoomEntitySpawnDef{1680, 80, 32929, 0, 0, 0}, RoomEntitySpawnDef{1696, 80, 32929, 0, 0, 0}, RoomEntitySpawnDef{1712, 80, 32929, 0, 0, 0}, RoomEntitySpawnDef{1008, 160, 32945, 0, 0, 0}, RoomEntitySpawnDef{1104, 192, 32945, 0, 0, 0}, RoomEntitySpawnDef{1248, 176, 32945, 0, 0, 0}, RoomEntitySpawnDef{1456, 208, 32945, 0, 0, 0}, RoomEntitySpawnDef{1856, 192, 32977, 0, 0, 0}},
    /* room 3 */ {RoomEntitySpawnDef{96, 128, 32945, 0, 0, 0}, RoomEntitySpawnDef{160, 144, 32977, 0, 0, 0}, RoomEntitySpawnDef{192, 176, 32977, 0, 0, 0}, RoomEntitySpawnDef{128, 144, 32993, 0, 0, 0}},
    /* room 4 */ {RoomEntitySpawnDef{64, 1136, 32945, 0, 0, 0}, RoomEntitySpawnDef{144, 1200, 32945, 0, 0, 0}, RoomEntitySpawnDef{208, 352, 32945, 0, 0, 0}, RoomEntitySpawnDef{240, 352, 32945, 0, 0, 0}, RoomEntitySpawnDef{128, 448, 32977, 0, 0, 0}, RoomEntitySpawnDef{144, 432, 32977, 0, 0, 0}, RoomEntitySpawnDef{160, 416, 32977, 0, 0, 0}, RoomEntitySpawnDef{80, 688, 32977, 0, 0, 0}, RoomEntitySpawnDef{208, 768, 32977, 0, 0, 0}, RoomEntitySpawnDef{48, 784, 32977, 0, 0, 0}, RoomEntitySpawnDef{96, 832, 32977, 0, 0, 0}, RoomEntitySpawnDef{128, 896, 32977, 0, 0, 0}, RoomEntitySpawnDef{288, 944, 32977, 0, 0, 0}, RoomEntitySpawnDef{288, 928, 32977, 0, 0, 0}, RoomEntitySpawnDef{224, 1232, 32977, 0, 0, 0}, RoomEntitySpawnDef{208, 976, 32993, 0, 0, 0}, RoomEntitySpawnDef{208, 1088, 32993, 0, 0, 0}, RoomEntitySpawnDef{352, 768, 32993, 0, 0, 0}, RoomEntitySpawnDef{176, 544, 32993, 0, 0, 0}, RoomEntitySpawnDef{376, 144, 32993, 0, 0, 0}},
    /* room 5 */ {},
    /* room 6 */ {},
    /* room 7 */ {RoomEntitySpawnDef{96, 112, 33025, 0, 0, 0}, RoomEntitySpawnDef{560, 224, 33025, 0, 0, 0}, RoomEntitySpawnDef{720, 224, 33025, 0, 0, 0}, RoomEntitySpawnDef{784, 224, 33025, 0, 0, 0}, RoomEntitySpawnDef{688, 224, 33009, 0, 0, 0}},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl0Specials = {
    /* room 0 */ {RoomEntitySpawnDef{656, 240, 16, 0, 0, 0}, RoomEntitySpawnDef{3, 0, 0, 512, 368, 32}, RoomEntitySpawnDef{0, 0, 0, 5, 0, 0}, RoomEntitySpawnDef{1152, 224, 16, 29040, 29040, 1}, RoomEntitySpawnDef{0, 0, 595, 1136, 432, 0}},
    /* room 1 */ {RoomEntitySpawnDef{496, 128, 16, 0, 0, 0}, RoomEntitySpawnDef{4, 0, 0, 432, 64, 16}, RoomEntitySpawnDef{0, 0, 0, 4, 1, 0}},
    /* room 2 */ {RoomEntitySpawnDef{48, 112, 0, 255, 0, 2}},
    /* room 3 */ {RoomEntitySpawnDef{32, 48, 16, 0, 0, 0}},
    /* room 4 */ {RoomEntitySpawnDef{48, 96, 16, 0, 0, 0}, RoomEntitySpawnDef{1, 0, 0, 112, 32, 16}},
    /* room 5 */ {RoomEntitySpawnDef{32, 112, 32, 0, 0, 0}},
    /* room 6 */ {RoomEntitySpawnDef{48, 32, 0, 0, 0, 0}, RoomEntitySpawnDef{0, 3, 0, 176, 112, 16}},
    /* room 7 */ {RoomEntitySpawnDef{736, 224, 16, 29040, 29040, 4}},
};

// ================= Level 1 Data =================
const std::vector<std::uint8_t> kLvl1TouchVals = {81};
const std::vector<std::uint8_t> kLvl1ProjVals = {0};
const std::vector<std::uint8_t> kLvl1Mode2Vals = {0};
const std::vector<std::vector<RoomTransitionZone>> kLvl1Zones = {
    /* room 0 */ {RoomTransitionZone{0, 528, 576, 0, 1, 6}, RoomTransitionZone{784, 96, 128, 1, 1, 0}},
    /* room 1 */ {RoomTransitionZone{0, 1440, 1472, 1, 0, 1}, RoomTransitionZone{400, 960, 992, 1, 4, 0}, RoomTransitionZone{400, 704, 752, 1, 2, 0}, RoomTransitionZone{400, 16, 48, 1, 5, 0}, RoomTransitionZone{400, 832, 832, 1, 7, 1}},
    /* room 2 */ {RoomTransitionZone{0, 240, 272, 1, 1, 2}, RoomTransitionZone{2032, 80, 112, 1, 3, 0}},
    /* room 3 */ {RoomTransitionZone{0, 96, 128, 1, 2, 1}, RoomTransitionZone{0, 0, 0, 1, 7, 2}},
    /* room 4 */ {RoomTransitionZone{0, 96, 128, 1, 1, 1}},
    /* room 5 */ {RoomTransitionZone{0, 80, 112, 1, 1, 3}, RoomTransitionZone{2032, 80, 112, 1, 6, 0}},
    /* room 6 */ {RoomTransitionZone{0, 688, 720, 1, 5, 1}, RoomTransitionZone{192, 32, 32, 1, 7, 0}},
    /* room 7 */ {RoomTransitionZone{0, 32, 32, 1, 6, 1}, RoomTransitionZone{0, 112, 112, 1, 1, 4}, RoomTransitionZone{192, 112, 112, 1, 3, 1}},
    /* room 8 */ {},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl1Enemies = {
    /* room 0 */ {RoomEntitySpawnDef{16, 48, 34608, 0, 0, 0}, RoomEntitySpawnDef{16, 128, 34608, 0, 0, 0}, RoomEntitySpawnDef{272, 512, 34592, 0, 0, 0}, RoomEntitySpawnDef{720, 528, 34592, 0, 0, 0}, RoomEntitySpawnDef{496, 192, 34624, 0, 0, 0}, RoomEntitySpawnDef{368, 224, 34624, 0, 0, 0}, RoomEntitySpawnDef{272, 192, 34624, 0, 0, 0}, RoomEntitySpawnDef{640, 256, 34624, 0, 0, 0}, RoomEntitySpawnDef{640, 272, 34624, 0, 0, 0}, RoomEntitySpawnDef{640, 304, 34624, 0, 0, 0}, RoomEntitySpawnDef{192, 352, 34624, 0, 0, 0}, RoomEntitySpawnDef{176, 368, 34624, 0, 0, 0}, RoomEntitySpawnDef{160, 384, 34624, 0, 0, 0}, RoomEntitySpawnDef{704, 656, 34624, 0, 0, 0}, RoomEntitySpawnDef{688, 672, 34624, 0, 0, 0}, RoomEntitySpawnDef{672, 688, 34624, 0, 0, 0}, RoomEntitySpawnDef{544, 464, 34624, 0, 0, 0}, RoomEntitySpawnDef{416, 496, 34640, 0, 0, 0}, RoomEntitySpawnDef{80, 656, 34640, 0, 0, 0}, RoomEntitySpawnDef{208, 64, 34560, 0, 0, 0}, RoomEntitySpawnDef{336, 64, 34560, 0, 0, 0}, RoomEntitySpawnDef{480, 64, 34560, 0, 0, 0}},
    /* room 1 */ {RoomEntitySpawnDef{96, 128, 34608, 0, 0, 0}, RoomEntitySpawnDef{256, 128, 34608, 0, 0, 0}, RoomEntitySpawnDef{192, 304, 34608, 0, 0, 0}, RoomEntitySpawnDef{192, 544, 34608, 0, 0, 0}, RoomEntitySpawnDef{368, 656, 34608, 0, 0, 0}, RoomEntitySpawnDef{272, 656, 34608, 0, 0, 0}, RoomEntitySpawnDef{192, 656, 34608, 0, 0, 0}, RoomEntitySpawnDef{32, 736, 34608, 0, 0, 0}, RoomEntitySpawnDef{240, 1008, 34608, 0, 0, 0}, RoomEntitySpawnDef{192, 1056, 34608, 0, 0, 0}, RoomEntitySpawnDef{144, 1104, 34608, 0, 0, 0}, RoomEntitySpawnDef{96, 1152, 34608, 0, 0, 0}, RoomEntitySpawnDef{32, 1232, 34608, 0, 0, 0}, RoomEntitySpawnDef{384, 1232, 34608, 0, 0, 0}, RoomEntitySpawnDef{112, 1472, 34576, 0, 0, 0}},
    /* room 2 */ {RoomEntitySpawnDef{80, 64, 34608, 0, 0, 0}, RoomEntitySpawnDef{208, 64, 34608, 0, 0, 0}, RoomEntitySpawnDef{336, 64, 34608, 0, 0, 0}, RoomEntitySpawnDef{464, 64, 34608, 0, 0, 0}, RoomEntitySpawnDef{896, 112, 34608, 0, 0, 0}, RoomEntitySpawnDef{1072, 112, 34608, 0, 0, 0}, RoomEntitySpawnDef{1104, 112, 34608, 0, 0, 0}, RoomEntitySpawnDef{80, 128, 34560, 0, 0, 0}, RoomEntitySpawnDef{208, 128, 34560, 0, 0, 0}, RoomEntitySpawnDef{480, 128, 34560, 0, 0, 0}, RoomEntitySpawnDef{496, 128, 34560, 0, 0, 0}, RoomEntitySpawnDef{1904, 288, 34560, 0, 0, 0}, RoomEntitySpawnDef{1920, 288, 34560, 0, 0, 0}, RoomEntitySpawnDef{1936, 288, 34560, 0, 0, 0}, RoomEntitySpawnDef{1584, 288, 34560, 0, 0, 0}, RoomEntitySpawnDef{1648, 288, 34560, 0, 0, 0}, RoomEntitySpawnDef{1712, 288, 34560, 0, 0, 0}, RoomEntitySpawnDef{224, 288, 34560, 0, 0, 0}, RoomEntitySpawnDef{272, 288, 34560, 0, 0, 0}, RoomEntitySpawnDef{320, 288, 34560, 0, 0, 0}, RoomEntitySpawnDef{464, 288, 34560, 0, 0, 0}, RoomEntitySpawnDef{864, 48, 34560, 0, 0, 0}, RoomEntitySpawnDef{960, 48, 34560, 0, 0, 0}, RoomEntitySpawnDef{1056, 48, 34560, 0, 0, 0}, RoomEntitySpawnDef{1152, 48, 34560, 0, 0, 0}},
    /* room 3 */ {RoomEntitySpawnDef{192, 64, 34560, 0, 0, 0}, RoomEntitySpawnDef{576, 48, 34576, 0, 0, 0}, RoomEntitySpawnDef{656, 48, 34576, 0, 0, 0}, RoomEntitySpawnDef{704, 48, 34576, 0, 0, 0}},
    /* room 4 */ {RoomEntitySpawnDef{192, 144, 34560, 0, 0, 0}, RoomEntitySpawnDef{320, 144, 34560, 0, 0, 0}, RoomEntitySpawnDef{448, 144, 34560, 0, 0, 0}, RoomEntitySpawnDef{576, 144, 34560, 0, 0, 0}, RoomEntitySpawnDef{192, 64, 34560, 0, 0, 0}, RoomEntitySpawnDef{320, 64, 34560, 0, 0, 0}, RoomEntitySpawnDef{448, 64, 34560, 0, 0, 0}, RoomEntitySpawnDef{576, 64, 34560, 0, 0, 0}, RoomEntitySpawnDef{256, 144, 34560, 0, 0, 0}, RoomEntitySpawnDef{384, 144, 34560, 0, 0, 0}, RoomEntitySpawnDef{512, 144, 34560, 0, 0, 0}, RoomEntitySpawnDef{640, 144, 34560, 0, 0, 0}, RoomEntitySpawnDef{256, 64, 34560, 0, 0, 0}, RoomEntitySpawnDef{384, 64, 34560, 0, 0, 0}, RoomEntitySpawnDef{512, 64, 34560, 0, 0, 0}, RoomEntitySpawnDef{640, 64, 34560, 0, 0, 0}},
    /* room 5 */ {RoomEntitySpawnDef{128, 128, 34560, 0, 0, 0}, RoomEntitySpawnDef{256, 128, 34560, 0, 0, 0}, RoomEntitySpawnDef{384, 128, 34560, 0, 0, 0}, RoomEntitySpawnDef{512, 128, 34560, 0, 0, 0}, RoomEntitySpawnDef{640, 128, 34560, 0, 0, 0}, RoomEntitySpawnDef{768, 128, 34560, 0, 0, 0}, RoomEntitySpawnDef{896, 128, 34560, 0, 0, 0}, RoomEntitySpawnDef{1024, 128, 34560, 0, 0, 0}, RoomEntitySpawnDef{1744, 128, 34560, 0, 0, 0}, RoomEntitySpawnDef{1776, 128, 34560, 0, 0, 0}, RoomEntitySpawnDef{1808, 128, 34560, 0, 0, 0}, RoomEntitySpawnDef{1888, 128, 34560, 0, 0, 0}, RoomEntitySpawnDef{1920, 128, 34560, 0, 0, 0}, RoomEntitySpawnDef{1952, 128, 34560, 0, 0, 0}},
    /* room 6 */ {RoomEntitySpawnDef{48, 80, 34656, 0, 0, 0}},
    /* room 7 */ {},
    /* room 8 */ {RoomEntitySpawnDef{320, 64, 34560, 0, 0, 0}, RoomEntitySpawnDef{368, 64, 34560, 0, 0, 0}, RoomEntitySpawnDef{128, 128, 34560, 0, 0, 0}, RoomEntitySpawnDef{176, 128, 34560, 0, 0, 0}},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl1Specials = {
    /* room 0 */ {RoomEntitySpawnDef{176, 128, 16, 26984, 26984, 0}},
    /* room 1 */ {},
    /* room 2 */ {},
    /* room 3 */ {RoomEntitySpawnDef{224, 48, 16, 26984, 26984, 1}},
    /* room 4 */ {},
    /* room 5 */ {},
    /* room 6 */ {},
    /* room 7 */ {},
    /* room 8 */ {RoomEntitySpawnDef{32, 112, 16, 26984, 26984, 0}, RoomEntitySpawnDef{0, 4, 599, 448, 48, 16}},
};

// ================= Level 2 Data =================
const std::vector<std::uint8_t> kLvl2TouchVals = {32};
const std::vector<std::uint8_t> kLvl2ProjVals = {32, 32, 32};
const std::vector<std::uint8_t> kLvl2Mode2Vals = {11, 15};
const std::vector<std::vector<RoomTransitionZone>> kLvl2Zones = {
    /* room 0 */ {RoomTransitionZone{1520, 336, 352, 2, 1, 0}},
    /* room 1 */ {RoomTransitionZone{0, 16, 32, 2, 0, 0}, RoomTransitionZone{3104, 0, 144, 2, 2, 0}},
    /* room 2 */ {RoomTransitionZone{0, 240, 368, 2, 1, 1}},
    /* room 3 */ {},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl2Enemies = {
    /* room 0 */ {RoomEntitySpawnDef{784, 16, 36264, 0, 0, 0}, RoomEntitySpawnDef{816, 16, 36264, 0, 0, 0}, RoomEntitySpawnDef{848, 16, 36264, 0, 0, 0}, RoomEntitySpawnDef{672, 16, 36264, 0, 0, 0}, RoomEntitySpawnDef{704, 16, 36264, 0, 0, 0}, RoomEntitySpawnDef{736, 16, 36264, 0, 0, 0}, RoomEntitySpawnDef{640, 256, 36264, 0, 0, 0}, RoomEntitySpawnDef{880, 256, 36264, 0, 0, 0}, RoomEntitySpawnDef{1184, 288, 36264, 0, 0, 0}, RoomEntitySpawnDef{1232, 288, 36264, 0, 0, 0}, RoomEntitySpawnDef{112, 384, 36248, 0, 0, 0}, RoomEntitySpawnDef{224, 384, 36248, 0, 0, 0}, RoomEntitySpawnDef{112, 240, 36248, 0, 0, 0}, RoomEntitySpawnDef{1152, 144, 36248, 0, 0, 0}, RoomEntitySpawnDef{1312, 144, 36248, 0, 0, 0}, RoomEntitySpawnDef{1264, 32, 36248, 0, 0, 0}},
    /* room 1 */ {RoomEntitySpawnDef{896, 16, 36264, 0, 0, 0}, RoomEntitySpawnDef{1408, 16, 36264, 0, 0, 0}, RoomEntitySpawnDef{2944, 160, 36248, 0, 0, 0}, RoomEntitySpawnDef{3056, 160, 36248, 0, 0, 0}},
    /* room 2 */ {RoomEntitySpawnDef{1296, 240, 36264, 0, 0, 0}, RoomEntitySpawnDef{1376, 240, 36264, 0, 0, 0}, RoomEntitySpawnDef{592, 16, 36264, 0, 0, 0}, RoomEntitySpawnDef{704, 16, 36264, 0, 0, 0}, RoomEntitySpawnDef{816, 16, 36264, 0, 0, 0}, RoomEntitySpawnDef{864, 96, 36264, 0, 0, 0}, RoomEntitySpawnDef{1440, 32, 36264, 0, 0, 0}, RoomEntitySpawnDef{96, 96, 36248, 0, 0, 0}, RoomEntitySpawnDef{432, 64, 36248, 0, 0, 0}, RoomEntitySpawnDef{1232, 80, 36248, 0, 0, 0}, RoomEntitySpawnDef{1344, 80, 36248, 0, 0, 0}, RoomEntitySpawnDef{1200, 176, 36248, 0, 0, 0}, RoomEntitySpawnDef{1280, 176, 36248, 0, 0, 0}, RoomEntitySpawnDef{1360, 176, 36248, 0, 0, 0}, RoomEntitySpawnDef{1312, 352, 36248, 0, 0, 0}, RoomEntitySpawnDef{1392, 352, 36248, 0, 0, 0}, RoomEntitySpawnDef{1056, 336, 36248, 0, 0, 0}, RoomEntitySpawnDef{160, 384, 36248, 0, 0, 0}},
    /* room 3 */ {RoomEntitySpawnDef{448, 16, 36264, 0, 0, 0}, RoomEntitySpawnDef{640, 16, 36264, 0, 0, 0}, RoomEntitySpawnDef{512, 256, 36264, 0, 0, 0}, RoomEntitySpawnDef{528, 256, 36264, 0, 0, 0}, RoomEntitySpawnDef{576, 224, 36264, 0, 0, 0}, RoomEntitySpawnDef{592, 224, 36264, 0, 0, 0}, RoomEntitySpawnDef{640, 256, 36264, 0, 0, 0}, RoomEntitySpawnDef{656, 256, 36264, 0, 0, 0}, RoomEntitySpawnDef{320, 48, 36248, 0, 0, 0}, RoomEntitySpawnDef{448, 48, 36248, 0, 0, 0}, RoomEntitySpawnDef{512, 48, 36248, 0, 0, 0}, RoomEntitySpawnDef{640, 48, 36248, 0, 0, 0}, RoomEntitySpawnDef{144, 256, 36248, 0, 0, 0}, RoomEntitySpawnDef{928, 48, 36248, 0, 0, 0}},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl2Specials = {
    /* room 0 */ {RoomEntitySpawnDef{1472, 128, 0, 255, 0, 0}, RoomEntitySpawnDef{2, 0, 0, 752, 80, 16}},
    /* room 1 */ {},
    /* room 2 */ {RoomEntitySpawnDef{48, 80, 16, 9252, 9509, 2}, RoomEntitySpawnDef{0, 1, 611, 224, 32, 16}, RoomEntitySpawnDef{9252, 9509, 2, 3, 0, 615}},
    /* room 3 */ {RoomEntitySpawnDef{64, 240, 16, 9252, 9509, 2}},
};

// ================= Level 3 Data =================
const std::vector<std::uint8_t> kLvl3TouchVals = {40};
const std::vector<std::uint8_t> kLvl3ProjVals = {0};
const std::vector<std::uint8_t> kLvl3Mode2Vals = {0};
const std::vector<std::vector<RoomTransitionZone>> kLvl3Zones = {
    /* room 0 */ {RoomTransitionZone{1264, 416, 432, 3, 1, 0}},
    /* room 1 */ {RoomTransitionZone{0, 48, 48, 3, 0, 0}},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl3Enemies = {
    /* room 0 */ {RoomEntitySpawnDef{768, 16, 37107, 0, 0, 0}, RoomEntitySpawnDef{768, 128, 37107, 0, 0, 0}, RoomEntitySpawnDef{960, 16, 37107, 0, 0, 0}, RoomEntitySpawnDef{960, 128, 37107, 0, 0, 0}, RoomEntitySpawnDef{880, 64, 37107, 0, 0, 0}, RoomEntitySpawnDef{720, 224, 37107, 0, 0, 0}, RoomEntitySpawnDef{304, 240, 37107, 0, 0, 0}, RoomEntitySpawnDef{160, 368, 37107, 0, 0, 0}, RoomEntitySpawnDef{512, 368, 37107, 0, 0, 0}, RoomEntitySpawnDef{1152, 368, 37107, 0, 0, 0}, RoomEntitySpawnDef{176, 176, 37107, 0, 0, 0}, RoomEntitySpawnDef{704, 368, 37107, 0, 0, 0}},
    /* room 1 */ {RoomEntitySpawnDef{128, 0, 37107, 0, 0, 0}, RoomEntitySpawnDef{128, 128, 37107, 0, 0, 0}, RoomEntitySpawnDef{256, 0, 37107, 0, 0, 0}, RoomEntitySpawnDef{256, 128, 37107, 0, 0, 0}, RoomEntitySpawnDef{384, 0, 37107, 0, 0, 0}, RoomEntitySpawnDef{384, 128, 37107, 0, 0, 0}, RoomEntitySpawnDef{512, 0, 37107, 0, 0, 0}, RoomEntitySpawnDef{512, 128, 37107, 0, 0, 0}, RoomEntitySpawnDef{944, 64, 37123, 0, 0, 0}},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl3Specials = {
    /* room 0 */ {RoomEntitySpawnDef{80, 80, 16, 10022, 10022, 2}, RoomEntitySpawnDef{2, 2, 619, 528, 272, 16}},
    /* room 1 */ {},
};

// ================= Level 4 Data =================
const std::vector<std::uint8_t> kLvl4TouchVals = {81};
const std::vector<std::uint8_t> kLvl4ProjVals = {0};
const std::vector<std::uint8_t> kLvl4Mode2Vals = {0};
const std::vector<std::vector<RoomTransitionZone>> kLvl4Zones = {
    /* room 0 */ {RoomTransitionZone{0, 528, 576, 0, 1, 6}},
    /* room 1 */ {},
    /* room 2 */ {},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl4Enemies = {
    /* room 0 */ {RoomEntitySpawnDef{256, 128, 37594, 0, 0, 0}},
    /* room 1 */ {},
    /* room 2 */ {RoomEntitySpawnDef{464, 128, 37610, 0, 0, 0}, RoomEntitySpawnDef{624, 128, 37610, 0, 0, 0}, RoomEntitySpawnDef{656, 128, 37610, 0, 0, 0}},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl4Specials = {
    /* room 0 */ {RoomEntitySpawnDef{176, 128, 16, 26984, 26984, 0}},
    /* room 1 */ {RoomEntitySpawnDef{32, 112, 16, 26984, 26984, 0}},
    /* room 2 */ {RoomEntitySpawnDef{176, 128, 16, 26984, 26984, 0}, RoomEntitySpawnDef{7, 0, 0, 576, 128, 16}},
};

// ================= Level 5 Data =================
const std::vector<std::uint8_t> kLvl5TouchVals = {0};
const std::vector<std::uint8_t> kLvl5ProjVals = {0};
const std::vector<std::uint8_t> kLvl5Mode2Vals = {0};
const std::vector<std::vector<RoomTransitionZone>> kLvl5Zones = {
    /* room 0 */ {},
    /* room 1 */ {},
    /* room 2 */ {},
    /* room 3 */ {},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl5Enemies = {
    /* room 0 */ {RoomEntitySpawnDef{1760, 32, 38129, 0, 0, 0}, RoomEntitySpawnDef{32, 128, 38129, 0, 0, 0}, RoomEntitySpawnDef{320, 144, 38129, 0, 0, 0}, RoomEntitySpawnDef{400, 176, 38129, 0, 0, 0}, RoomEntitySpawnDef{464, 176, 38129, 0, 0, 0}, RoomEntitySpawnDef{880, 112, 38129, 0, 0, 0}, RoomEntitySpawnDef{1024, 112, 38129, 0, 0, 0}, RoomEntitySpawnDef{1264, 96, 38129, 0, 0, 0}, RoomEntitySpawnDef{1440, 128, 38129, 0, 0, 0}, RoomEntitySpawnDef{1760, 128, 38129, 0, 0, 0}, RoomEntitySpawnDef{848, 64, 38129, 0, 0, 0}, RoomEntitySpawnDef{944, 112, 38129, 0, 0, 0}, RoomEntitySpawnDef{400, 64, 38129, 0, 0, 0}, RoomEntitySpawnDef{464, 64, 38129, 0, 0, 0}, RoomEntitySpawnDef{1856, 256, 38145, 0, 0, 0}, RoomEntitySpawnDef{1600, 256, 38145, 0, 0, 0}, RoomEntitySpawnDef{1536, 272, 38145, 0, 0, 0}, RoomEntitySpawnDef{1232, 272, 38145, 0, 0, 0}, RoomEntitySpawnDef{1040, 272, 38145, 0, 0, 0}, RoomEntitySpawnDef{624, 272, 38145, 0, 0, 0}, RoomEntitySpawnDef{80, 272, 38145, 0, 0, 0}, RoomEntitySpawnDef{640, 288, 38145, 0, 0, 0}, RoomEntitySpawnDef{416, 288, 38145, 0, 0, 0}, RoomEntitySpawnDef{192, 288, 38145, 0, 0, 0}},
    /* room 1 */ {RoomEntitySpawnDef{128, 80, 38129, 0, 0, 0}, RoomEntitySpawnDef{336, 32, 38129, 0, 0, 0}, RoomEntitySpawnDef{736, 32, 38129, 0, 0, 0}, RoomEntitySpawnDef{752, 128, 38129, 0, 0, 0}, RoomEntitySpawnDef{1120, 176, 38129, 0, 0, 0}, RoomEntitySpawnDef{1456, 192, 38129, 0, 0, 0}, RoomEntitySpawnDef{1504, 160, 38129, 0, 0, 0}, RoomEntitySpawnDef{1632, 128, 38129, 0, 0, 0}, RoomEntitySpawnDef{2000, 128, 38129, 0, 0, 0}, RoomEntitySpawnDef{1920, 240, 38145, 0, 0, 0}, RoomEntitySpawnDef{1488, 224, 38145, 0, 0, 0}, RoomEntitySpawnDef{1248, 272, 38145, 0, 0, 0}, RoomEntitySpawnDef{816, 224, 38145, 0, 0, 0}, RoomEntitySpawnDef{864, 272, 38145, 0, 0, 0}, RoomEntitySpawnDef{496, 256, 38145, 0, 0, 0}, RoomEntitySpawnDef{96, 272, 38145, 0, 0, 0}, RoomEntitySpawnDef{1456, 224, 38145, 0, 0, 0}, RoomEntitySpawnDef{1568, 208, 38145, 0, 0, 0}, RoomEntitySpawnDef{1200, 288, 38145, 0, 0, 0}},
    /* room 2 */ {RoomEntitySpawnDef{176, 80, 38129, 0, 0, 0}, RoomEntitySpawnDef{128, 112, 38129, 0, 0, 0}, RoomEntitySpawnDef{64, 144, 38129, 0, 0, 0}, RoomEntitySpawnDef{400, 32, 38129, 0, 0, 0}, RoomEntitySpawnDef{464, 192, 38129, 0, 0, 0}, RoomEntitySpawnDef{1232, 32, 38129, 0, 0, 0}, RoomEntitySpawnDef{1248, 128, 38129, 0, 0, 0}, RoomEntitySpawnDef{1728, 112, 38129, 0, 0, 0}, RoomEntitySpawnDef{2016, 128, 38129, 0, 0, 0}, RoomEntitySpawnDef{1856, 240, 38145, 0, 0, 0}, RoomEntitySpawnDef{1616, 240, 38145, 0, 0, 0}, RoomEntitySpawnDef{1328, 240, 38145, 0, 0, 0}, RoomEntitySpawnDef{1024, 256, 38145, 0, 0, 0}, RoomEntitySpawnDef{288, 288, 38145, 0, 0, 0}, RoomEntitySpawnDef{384, 288, 38145, 0, 0, 0}, RoomEntitySpawnDef{64, 256, 38145, 0, 0, 0}},
    /* room 3 */ {RoomEntitySpawnDef{704, 112, 38161, 0, 0, 0}},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl5Specials = {
    /* room 0 */ {RoomEntitySpawnDef{80, 128, 16, 4111, 4625, 5}, RoomEntitySpawnDef{1, 0, 0, 256, 112, 16}, RoomEntitySpawnDef{4111, 4625, 5, 0, 2, 0}, RoomEntitySpawnDef{1936, 64, 16, 4111, 4625, 0}},
    /* room 1 */ {RoomEntitySpawnDef{928, 64, 16, 4111, 4625, 5}, RoomEntitySpawnDef{0, 0, 0, 128, 128, 16}, RoomEntitySpawnDef{4111, 4625, 5, 1, 0, 0}, RoomEntitySpawnDef{1552, 48, 16, 4111, 4625, 5}, RoomEntitySpawnDef{1, 0, 0, 1936, 48, 16}},
    /* room 2 */ {RoomEntitySpawnDef{1424, 64, 16, 4111, 4625, 5}, RoomEntitySpawnDef{1, 4, 0, 80, 128, 16}, RoomEntitySpawnDef{4111, 4625, 5, 2, 0, 0}, RoomEntitySpawnDef{256, 112, 16, 4111, 4625, 5}, RoomEntitySpawnDef{2, 0, 0, 1616, 64, 16}, RoomEntitySpawnDef{4111, 4625, 5, 2, 0, 0}, RoomEntitySpawnDef{1808, 64, 16, 4111, 4625, 5}},
    /* room 3 */ {RoomEntitySpawnDef{1712, 64, 16, 4111, 4625, 5}},
};

// ================= Level 6 Data =================
const std::vector<std::uint8_t> kLvl6TouchVals = {33, 48, 57};
const std::vector<std::uint8_t> kLvl6ProjVals = {0};
const std::vector<std::uint8_t> kLvl6Mode2Vals = {0};
const std::vector<std::vector<RoomTransitionZone>> kLvl6Zones = {
    /* room 0 */ {RoomTransitionZone{1264, 96, 272, 6, 1, 0}, RoomTransitionZone{1264, 0, 48, 6, 1, 1}},
    /* room 1 */ {RoomTransitionZone{0, 128, 288, 6, 0, 0}, RoomTransitionZone{0, 0, 80, 6, 0, 1}, RoomTransitionZone{1520, 144, 192, 6, 2, 0}},
    /* room 2 */ {RoomTransitionZone{0, 0, 128, 6, 1, 2}},
    /* room 3 */ {},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl6Enemies = {
    /* room 0 */ {RoomEntitySpawnDef{256, 240, 39206, 0, 0, 0}, RoomEntitySpawnDef{176, 400, 39206, 0, 0, 0}, RoomEntitySpawnDef{288, 400, 39206, 0, 0, 0}, RoomEntitySpawnDef{176, 112, 39206, 0, 0, 0}, RoomEntitySpawnDef{96, 160, 39206, 0, 0, 0}, RoomEntitySpawnDef{304, 320, 39206, 0, 0, 0}, RoomEntitySpawnDef{416, 112, 39206, 0, 0, 0}, RoomEntitySpawnDef{512, 144, 39206, 0, 0, 0}, RoomEntitySpawnDef{736, 96, 39206, 0, 0, 0}, RoomEntitySpawnDef{736, 48, 39206, 0, 0, 0}, RoomEntitySpawnDef{912, 80, 39206, 0, 0, 0}, RoomEntitySpawnDef{976, 80, 39206, 0, 0, 0}, RoomEntitySpawnDef{1184, 32, 39206, 0, 0, 0}, RoomEntitySpawnDef{736, 224, 39206, 0, 0, 0}, RoomEntitySpawnDef{736, 176, 39206, 0, 0, 0}, RoomEntitySpawnDef{976, 288, 39222, 0, 0, 0}, RoomEntitySpawnDef{1104, 288, 39222, 0, 0, 0}, RoomEntitySpawnDef{944, 448, 39222, 0, 0, 0}, RoomEntitySpawnDef{284, 240, 39254, 0, 0, 0}, RoomEntitySpawnDef{140, 112, 39254, 0, 0, 0}, RoomEntitySpawnDef{380, 96, 39254, 0, 0, 0}, RoomEntitySpawnDef{716, 112, 39254, 0, 0, 0}, RoomEntitySpawnDef{1084, 48, 39254, 0, 0, 0}},
    /* room 1 */ {RoomEntitySpawnDef{1088, 192, 39206, 0, 0, 0}, RoomEntitySpawnDef{1120, 192, 39206, 0, 0, 0}, RoomEntitySpawnDef{848, 192, 39206, 0, 0, 0}, RoomEntitySpawnDef{896, 192, 39206, 0, 0, 0}, RoomEntitySpawnDef{992, 48, 39206, 0, 0, 0}, RoomEntitySpawnDef{592, 112, 39206, 0, 0, 0}, RoomEntitySpawnDef{592, 32, 39206, 0, 0, 0}, RoomEntitySpawnDef{672, 16, 39206, 0, 0, 0}, RoomEntitySpawnDef{160, 48, 39206, 0, 0, 0}, RoomEntitySpawnDef{256, 32, 39206, 0, 0, 0}, RoomEntitySpawnDef{192, 304, 39222, 0, 0, 0}, RoomEntitySpawnDef{320, 304, 39222, 0, 0, 0}, RoomEntitySpawnDef{448, 304, 39222, 0, 0, 0}, RoomEntitySpawnDef{1416, 288, 39238, 0, 0, 0}, RoomEntitySpawnDef{124, 160, 39254, 0, 0, 0}, RoomEntitySpawnDef{300, 32, 39254, 0, 0, 0}, RoomEntitySpawnDef{636, 96, 39254, 0, 0, 0}, RoomEntitySpawnDef{924, 144, 39254, 0, 0, 0}},
    /* room 2 */ {RoomEntitySpawnDef{80, 224, 39238, 0, 0, 0}, RoomEntitySpawnDef{192, 224, 39238, 0, 0, 0}, RoomEntitySpawnDef{400, 224, 39238, 0, 0, 0}},
    /* room 3 */ {RoomEntitySpawnDef{32, 112, 39238, 0, 0, 0}},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl6Specials = {
    /* room 0 */ {RoomEntitySpawnDef{32, 384, 16, 22616, 22616, 65535}, RoomEntitySpawnDef{0, 0, 0, 848, 432, 16}},
    /* room 1 */ {},
    /* room 2 */ {RoomEntitySpawnDef{304, 512, 16, 13106, 13620, 6}, RoomEntitySpawnDef{0, 1, 0, 208, 112, 16}},
    /* room 3 */ {RoomEntitySpawnDef{320, 32, 16, 13106, 13620, 65535}},
};

// ================= Level 7 Data =================
const std::vector<std::uint8_t> kLvl7TouchVals = {40, 40};
const std::vector<std::uint8_t> kLvl7ProjVals = {0};
const std::vector<std::uint8_t> kLvl7Mode2Vals = {0};
const std::vector<std::vector<RoomTransitionZone>> kLvl7Zones = {
    /* room 0 */ {RoomTransitionZone{1520, 320, 336, 7, 1, 0}},
    /* room 1 */ {RoomTransitionZone{0, 416, 432, 7, 0, 0}},
    /* room 2 */ {},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl7Enemies = {
    /* room 0 */ {RoomEntitySpawnDef{224, 360, 40194, 0, 0, 0}, RoomEntitySpawnDef{560, 360, 40194, 0, 0, 0}, RoomEntitySpawnDef{32, 360, 40194, 0, 0, 0}, RoomEntitySpawnDef{336, 104, 40194, 0, 0, 0}, RoomEntitySpawnDef{640, 232, 40194, 0, 0, 0}, RoomEntitySpawnDef{688, 232, 40194, 0, 0, 0}, RoomEntitySpawnDef{656, 104, 40194, 0, 0, 0}, RoomEntitySpawnDef{752, 104, 40194, 0, 0, 0}, RoomEntitySpawnDef{848, 104, 40194, 0, 0, 0}, RoomEntitySpawnDef{608, 104, 40194, 0, 0, 0}, RoomEntitySpawnDef{240, 232, 40194, 0, 0, 0}, RoomEntitySpawnDef{944, 96, 40210, 0, 0, 0}, RoomEntitySpawnDef{1056, 96, 40210, 0, 0, 0}, RoomEntitySpawnDef{1088, 96, 40210, 0, 0, 0}, RoomEntitySpawnDef{1120, 96, 40210, 0, 0, 0}, RoomEntitySpawnDef{1280, 96, 40210, 0, 0, 0}, RoomEntitySpawnDef{1312, 96, 40210, 0, 0, 0}, RoomEntitySpawnDef{1344, 96, 40210, 0, 0, 0}, RoomEntitySpawnDef{1344, 224, 40210, 0, 0, 0}, RoomEntitySpawnDef{448, 224, 40210, 0, 0, 0}, RoomEntitySpawnDef{552, 224, 40210, 0, 0, 0}, RoomEntitySpawnDef{944, 16, 40226, 0, 0, 0}, RoomEntitySpawnDef{1056, 16, 40226, 0, 0, 0}, RoomEntitySpawnDef{1120, 16, 40226, 0, 0, 0}, RoomEntitySpawnDef{1280, 16, 40226, 0, 0, 0}, RoomEntitySpawnDef{1312, 16, 40226, 0, 0, 0}, RoomEntitySpawnDef{1344, 128, 40226, 0, 0, 0}, RoomEntitySpawnDef{448, 128, 40226, 0, 0, 0}, RoomEntitySpawnDef{552, 128, 40226, 0, 0, 0}},
    /* room 1 */ {RoomEntitySpawnDef{176, 456, 40194, 0, 0, 0}, RoomEntitySpawnDef{288, 456, 40194, 0, 0, 0}, RoomEntitySpawnDef{400, 456, 40194, 0, 0, 0}, RoomEntitySpawnDef{512, 456, 40194, 0, 0, 0}, RoomEntitySpawnDef{1024, 424, 40194, 0, 0, 0}, RoomEntitySpawnDef{656, 280, 40194, 0, 0, 0}, RoomEntitySpawnDef{624, 280, 40194, 0, 0, 0}, RoomEntitySpawnDef{592, 280, 40194, 0, 0, 0}, RoomEntitySpawnDef{112, 160, 40178, 0, 0, 0}, RoomEntitySpawnDef{192, 160, 40178, 0, 0, 0}, RoomEntitySpawnDef{272, 160, 40178, 0, 0, 0}, RoomEntitySpawnDef{352, 160, 40178, 0, 0, 0}, RoomEntitySpawnDef{448, 160, 40178, 0, 0, 0}, RoomEntitySpawnDef{656, 32, 40178, 0, 0, 0}, RoomEntitySpawnDef{768, 32, 40178, 0, 0, 0}, RoomEntitySpawnDef{960, 32, 40178, 0, 0, 0}, RoomEntitySpawnDef{1056, 32, 40178, 0, 0, 0}, RoomEntitySpawnDef{128, 448, 40210, 0, 0, 0}, RoomEntitySpawnDef{240, 448, 40210, 0, 0, 0}, RoomEntitySpawnDef{352, 448, 40210, 0, 0, 0}, RoomEntitySpawnDef{464, 448, 40210, 0, 0, 0}, RoomEntitySpawnDef{736, 448, 40210, 0, 0, 0}, RoomEntitySpawnDef{752, 448, 40210, 0, 0, 0}, RoomEntitySpawnDef{768, 448, 40210, 0, 0, 0}, RoomEntitySpawnDef{128, 336, 40226, 0, 0, 0}, RoomEntitySpawnDef{240, 336, 40226, 0, 0, 0}, RoomEntitySpawnDef{352, 336, 40226, 0, 0, 0}, RoomEntitySpawnDef{464, 336, 40226, 0, 0, 0}},
    /* room 2 */ {RoomEntitySpawnDef{464, 32, 40178, 0, 0, 0}, RoomEntitySpawnDef{144, 128, 40178, 0, 0, 0}, RoomEntitySpawnDef{96, 128, 40178, 0, 0, 0}, RoomEntitySpawnDef{848, 168, 40194, 0, 0, 0}, RoomEntitySpawnDef{880, 168, 40194, 0, 0, 0}, RoomEntitySpawnDef{928, 152, 40194, 0, 0, 0}, RoomEntitySpawnDef{960, 152, 40194, 0, 0, 0}, RoomEntitySpawnDef{640, 184, 40194, 0, 0, 0}, RoomEntitySpawnDef{1008, 136, 40194, 0, 0, 0}, RoomEntitySpawnDef{1040, 136, 40194, 0, 0, 0}, RoomEntitySpawnDef{720, 176, 40210, 0, 0, 0}, RoomEntitySpawnDef{784, 176, 40210, 0, 0, 0}, RoomEntitySpawnDef{720, 64, 40226, 0, 0, 0}, RoomEntitySpawnDef{784, 64, 40226, 0, 0, 0}, RoomEntitySpawnDef{1480, 160, 40242, 0, 0, 0}, RoomEntitySpawnDef{1472, 184, 40194, 256, 0, 0}, RoomEntitySpawnDef{1408, 216, 40194, 0, 0, 0}, RoomEntitySpawnDef{1424, 216, 40194, 0, 0, 0}, RoomEntitySpawnDef{1440, 216, 40194, 0, 0, 0}},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl7Specials = {
    /* room 0 */ {RoomEntitySpawnDef{176, 80, 16, 16962, 17219, 7}, RoomEntitySpawnDef{0, 6, 0, 288, 208, 16}, RoomEntitySpawnDef{16962, 17219, 7, 0, 2, 0}, RoomEntitySpawnDef{176, 336, 16, 16962, 17219, 7}, RoomEntitySpawnDef{0, 1, 0, 384, 336, 16}, RoomEntitySpawnDef{16962, 17219, 6, 2, 1, 623}, RoomEntitySpawnDef{416, 80, 16, 16962, 17219, 7}, RoomEntitySpawnDef{0, 5, 0, 544, 208, 16}, RoomEntitySpawnDef{16962, 17219, 7, 0, 4, 0}},
    /* room 1 */ {RoomEntitySpawnDef{1008, 176, 16, 16962, 17219, 7}},
    /* room 2 */ {RoomEntitySpawnDef{544, 160, 16, 16962, 17219, 7}},
};

// ================= Level 8 Data =================
const std::vector<std::uint8_t> kLvl8TouchVals = {22, 22};
const std::vector<std::uint8_t> kLvl8ProjVals = {22, 22};
const std::vector<std::uint8_t> kLvl8Mode2Vals = {3, 4, 5};
const std::vector<std::vector<RoomTransitionZone>> kLvl8Zones = {
    /* room 0 */ {},
    /* room 1 */ {},
    /* room 2 */ {},
    /* room 3 */ {},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl8Enemies = {
    /* room 0 */ {RoomEntitySpawnDef{240, 32, 41673, 0, 0, 0}, RoomEntitySpawnDef{240, 48, 41673, 0, 0, 0}, RoomEntitySpawnDef{240, 64, 41673, 0, 0, 0}, RoomEntitySpawnDef{240, 80, 41673, 0, 0, 0}, RoomEntitySpawnDef{240, 96, 41673, 0, 0, 0}, RoomEntitySpawnDef{240, 112, 41673, 0, 0, 0}, RoomEntitySpawnDef{464, 32, 41673, 0, 0, 0}, RoomEntitySpawnDef{464, 48, 41673, 0, 0, 0}, RoomEntitySpawnDef{464, 64, 41673, 0, 0, 0}, RoomEntitySpawnDef{464, 80, 41673, 0, 0, 0}, RoomEntitySpawnDef{464, 96, 41673, 0, 0, 0}, RoomEntitySpawnDef{464, 112, 41673, 0, 0, 0}, RoomEntitySpawnDef{992, 80, 41673, 0, 0, 0}, RoomEntitySpawnDef{992, 96, 41673, 0, 0, 0}, RoomEntitySpawnDef{992, 112, 41673, 0, 0, 0}, RoomEntitySpawnDef{992, 128, 41673, 0, 0, 0}},
    /* room 1 */ {RoomEntitySpawnDef{912, 272, 41721, 0, 0, 0}, RoomEntitySpawnDef{1168, 272, 41721, 0, 0, 0}, RoomEntitySpawnDef{1264, 272, 41721, 0, 0, 0}, RoomEntitySpawnDef{1280, 272, 41721, 0, 0, 0}, RoomEntitySpawnDef{1296, 272, 41721, 0, 0, 0}, RoomEntitySpawnDef{1392, 144, 41721, 0, 0, 0}, RoomEntitySpawnDef{1856, 48, 41721, 0, 0, 0}, RoomEntitySpawnDef{1968, 80, 41721, 0, 0, 0}, RoomEntitySpawnDef{1120, 128, 41721, 0, 0, 0}, RoomEntitySpawnDef{1264, 144, 41721, 0, 0, 0}, RoomEntitySpawnDef{96, 160, 41689, 0, 0, 0}, RoomEntitySpawnDef{208, 224, 41689, 0, 0, 0}, RoomEntitySpawnDef{288, 224, 41689, 0, 0, 0}, RoomEntitySpawnDef{368, 224, 41689, 0, 0, 0}, RoomEntitySpawnDef{448, 224, 41689, 0, 0, 0}, RoomEntitySpawnDef{528, 224, 41689, 0, 0, 0}, RoomEntitySpawnDef{208, 128, 41689, 0, 0, 0}, RoomEntitySpawnDef{288, 128, 41689, 0, 0, 0}, RoomEntitySpawnDef{368, 128, 41689, 0, 0, 0}, RoomEntitySpawnDef{448, 128, 41689, 0, 0, 0}, RoomEntitySpawnDef{528, 128, 41689, 0, 0, 0}, RoomEntitySpawnDef{608, 224, 41689, 0, 0, 0}, RoomEntitySpawnDef{656, 288, 41689, 0, 0, 0}, RoomEntitySpawnDef{688, 288, 41689, 0, 0, 0}, RoomEntitySpawnDef{256, 96, 41689, 0, 0, 0}, RoomEntitySpawnDef{304, 16, 41689, 0, 0, 0}, RoomEntitySpawnDef{384, 96, 41689, 0, 0, 0}, RoomEntitySpawnDef{464, 16, 41689, 0, 0, 0}, RoomEntitySpawnDef{752, 48, 41689, 0, 0, 0}, RoomEntitySpawnDef{1120, 144, 41689, 0, 0, 0}, RoomEntitySpawnDef{1488, 96, 41689, 0, 0, 0}, RoomEntitySpawnDef{1520, 288, 41689, 0, 0, 0}},
    /* room 2 */ {RoomEntitySpawnDef{400, 448, 41689, 0, 0, 0}, RoomEntitySpawnDef{224, 448, 41689, 0, 0, 0}, RoomEntitySpawnDef{32, 192, 41689, 0, 0, 0}, RoomEntitySpawnDef{80, 192, 41689, 0, 0, 0}, RoomEntitySpawnDef{16, 128, 41689, 0, 0, 0}, RoomEntitySpawnDef{176, 64, 41689, 0, 0, 0}, RoomEntitySpawnDef{240, 0, 41689, 0, 0, 0}, RoomEntitySpawnDef{448, 80, 41689, 0, 0, 0}, RoomEntitySpawnDef{480, 48, 41689, 0, 0, 0}, RoomEntitySpawnDef{496, 16, 41689, 0, 0, 0}, RoomEntitySpawnDef{544, 160, 41689, 0, 0, 0}, RoomEntitySpawnDef{720, 192, 41689, 0, 0, 0}, RoomEntitySpawnDef{608, 256, 41689, 0, 0, 0}, RoomEntitySpawnDef{720, 320, 41689, 0, 0, 0}, RoomEntitySpawnDef{608, 384, 41689, 0, 0, 0}, RoomEntitySpawnDef{720, 448, 41689, 0, 0, 0}, RoomEntitySpawnDef{864, 352, 41689, 0, 0, 0}, RoomEntitySpawnDef{880, 352, 41689, 0, 0, 0}, RoomEntitySpawnDef{944, 416, 41689, 0, 0, 0}, RoomEntitySpawnDef{944, 432, 41689, 0, 0, 0}, RoomEntitySpawnDef{944, 448, 41689, 0, 0, 0}, RoomEntitySpawnDef{1088, 384, 41689, 0, 0, 0}, RoomEntitySpawnDef{1024, 320, 41689, 0, 0, 0}, RoomEntitySpawnDef{1088, 256, 41689, 0, 0, 0}, RoomEntitySpawnDef{1024, 192, 41689, 0, 0, 0}, RoomEntitySpawnDef{1088, 128, 41689, 0, 0, 0}, RoomEntitySpawnDef{1024, 64, 41689, 0, 0, 0}},
    /* room 3 */ {RoomEntitySpawnDef{160, 176, 41705, 0, 0, 0}, RoomEntitySpawnDef{128, 240, 41705, 0, 0, 0}, RoomEntitySpawnDef{272, 224, 41705, 0, 0, 0}, RoomEntitySpawnDef{224, 224, 41705, 0, 0, 0}, RoomEntitySpawnDef{272, 112, 41705, 0, 0, 0}, RoomEntitySpawnDef{416, 192, 41705, 0, 0, 0}, RoomEntitySpawnDef{416, 80, 41705, 0, 0, 0}, RoomEntitySpawnDef{512, 192, 41705, 0, 0, 0}, RoomEntitySpawnDef{544, 64, 41705, 0, 0, 0}, RoomEntitySpawnDef{640, 288, 41705, 0, 0, 0}, RoomEntitySpawnDef{704, 288, 41705, 0, 0, 0}, RoomEntitySpawnDef{928, 288, 41705, 0, 0, 0}, RoomEntitySpawnDef{928, 208, 41705, 0, 0, 0}, RoomEntitySpawnDef{1040, 192, 41705, 0, 0, 0}, RoomEntitySpawnDef{1040, 272, 41705, 0, 0, 0}, RoomEntitySpawnDef{1136, 192, 41705, 0, 0, 0}, RoomEntitySpawnDef{1136, 272, 41705, 0, 0, 0}, RoomEntitySpawnDef{1504, 192, 41705, 0, 0, 0}, RoomEntitySpawnDef{1664, 160, 41705, 0, 0, 0}, RoomEntitySpawnDef{1840, 208, 41737, 256, 0, 0}, RoomEntitySpawnDef{1744, 208, 41737, 256, 0, 0}, RoomEntitySpawnDef{1712, 224, 41737, 256, 0, 0}, RoomEntitySpawnDef{1872, 224, 41737, 256, 0, 0}, RoomEntitySpawnDef{1680, 240, 41737, 256, 0, 0}, RoomEntitySpawnDef{1904, 240, 41737, 256, 0, 0}, RoomEntitySpawnDef{1788, 192, 41753, 0, 0, 0}},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl8Specials = {
    /* room 0 */ {RoomEntitySpawnDef{48, 112, 16, 17732, 18246, 65535}, RoomEntitySpawnDef{0, 0, 0, 960, 112, 16}},
    /* room 1 */ {RoomEntitySpawnDef{48, 272, 16, 7966, 8480, 8}, RoomEntitySpawnDef{0, 1, 0, 32, 128, 16}, RoomEntitySpawnDef{7966, 8480, 8, 1, 2, 0}, RoomEntitySpawnDef{784, 272, 16, 7966, 8480, 8}, RoomEntitySpawnDef{1, 1, 0, 784, 128, 16}, RoomEntitySpawnDef{7966, 8480, 8, 1, 6, 0}, RoomEntitySpawnDef{912, 128, 16, 7966, 8480, 8}, RoomEntitySpawnDef{1, 5, 0, 1232, 32, 16}, RoomEntitySpawnDef{7966, 8480, 8, 1, 4, 0}, RoomEntitySpawnDef{1552, 272, 16, 7966, 8480, 8}},
    /* room 2 */ {RoomEntitySpawnDef{96, 352, 16, 17732, 18246, 8}, RoomEntitySpawnDef{1, 9, 0, 496, 240, 16}, RoomEntitySpawnDef{6425, 6425, 8, 2, 2, 627}, RoomEntitySpawnDef{880, 96, 16, 6425, 6425, 8}},
    /* room 3 */ {RoomEntitySpawnDef{32, 64, 16, 17732, 18246, 8}, RoomEntitySpawnDef{2, 3, 0, 1232, 64, 16}, RoomEntitySpawnDef{17732, 18246, 8, 3, 2, 0}},
};

// ================= Level 9 Data =================
const std::vector<std::uint8_t> kLvl9TouchVals = {48};
const std::vector<std::uint8_t> kLvl9ProjVals = {0};
const std::vector<std::uint8_t> kLvl9Mode2Vals = {0};
const std::vector<std::vector<RoomTransitionZone>> kLvl9Zones = {
    /* room 0 */ {RoomTransitionZone{2032, 0, 256, 9, 1, 0}},
    /* room 1 */ {RoomTransitionZone{0, 0, 208, 9, 0, 0}, RoomTransitionZone{2032, 0, 32, 9, 2, 0}, RoomTransitionZone{2032, 128, 208, 9, 2, 1}},
    /* room 2 */ {RoomTransitionZone{0, 0, 48, 9, 1, 1}, RoomTransitionZone{0, 304, 336, 9, 1, 2}},
    /* room 3 */ {},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl9Enemies = {
    /* room 0 */ {RoomEntitySpawnDef{112, 256, 43370, 0, 0, 0}, RoomEntitySpawnDef{80, 256, 43370, 0, 0, 0}, RoomEntitySpawnDef{48, 240, 43370, 0, 0, 0}, RoomEntitySpawnDef{144, 112, 43354, 0, 0, 0}, RoomEntitySpawnDef{192, 160, 43354, 0, 0, 0}, RoomEntitySpawnDef{224, 176, 43354, 0, 0, 0}, RoomEntitySpawnDef{368, 208, 43354, 0, 0, 0}, RoomEntitySpawnDef{384, 224, 43354, 0, 0, 0}, RoomEntitySpawnDef{480, 256, 43354, 0, 0, 0}, RoomEntitySpawnDef{736, 160, 43354, 0, 0, 0}, RoomEntitySpawnDef{624, 144, 43354, 0, 0, 0}, RoomEntitySpawnDef{1184, 128, 43354, 0, 0, 0}, RoomEntitySpawnDef{1200, 112, 43354, 0, 0, 0}, RoomEntitySpawnDef{1216, 96, 43354, 0, 0, 0}, RoomEntitySpawnDef{1232, 80, 43354, 0, 0, 0}, RoomEntitySpawnDef{1248, 64, 43354, 0, 0, 0}, RoomEntitySpawnDef{528, 256, 43370, 0, 0, 0}, RoomEntitySpawnDef{720, 256, 43370, 0, 0, 0}, RoomEntitySpawnDef{688, 256, 43370, 0, 0, 0}, RoomEntitySpawnDef{864, 256, 43370, 0, 0, 0}, RoomEntitySpawnDef{896, 256, 43370, 0, 0, 0}, RoomEntitySpawnDef{1312, 176, 43370, 0, 0, 0}, RoomEntitySpawnDef{1296, 192, 43370, 0, 0, 0}, RoomEntitySpawnDef{1280, 208, 43370, 0, 0, 0}, RoomEntitySpawnDef{1264, 224, 43370, 0, 0, 0}, RoomEntitySpawnDef{1248, 240, 43370, 0, 0, 0}, RoomEntitySpawnDef{944, 256, 43370, 0, 0, 0}, RoomEntitySpawnDef{960, 256, 43370, 0, 0, 0}, RoomEntitySpawnDef{976, 256, 43370, 0, 0, 0}},
    /* room 1 */ {},
    /* room 2 */ {RoomEntitySpawnDef{272, 64, 43354, 0, 0, 0}, RoomEntitySpawnDef{304, 0, 43354, 0, 0, 0}, RoomEntitySpawnDef{288, 0, 43354, 0, 0, 0}, RoomEntitySpawnDef{320, 64, 43354, 0, 0, 0}, RoomEntitySpawnDef{640, 16, 43354, 0, 0, 0}, RoomEntitySpawnDef{656, 16, 43354, 0, 0, 0}, RoomEntitySpawnDef{160, 304, 43370, 0, 0, 0}, RoomEntitySpawnDef{272, 304, 43370, 0, 0, 0}, RoomEntitySpawnDef{288, 304, 43370, 0, 0, 0}, RoomEntitySpawnDef{304, 304, 43370, 0, 0, 0}, RoomEntitySpawnDef{560, 304, 43370, 0, 0, 0}, RoomEntitySpawnDef{592, 304, 43370, 0, 0, 0}, RoomEntitySpawnDef{608, 304, 43370, 0, 0, 0}, RoomEntitySpawnDef{864, 144, 43370, 0, 0, 0}, RoomEntitySpawnDef{848, 160, 43370, 0, 0, 0}, RoomEntitySpawnDef{832, 176, 43370, 0, 0, 0}, RoomEntitySpawnDef{816, 192, 43370, 0, 0, 0}, RoomEntitySpawnDef{800, 208, 43370, 0, 0, 0}, RoomEntitySpawnDef{1392, 160, 43370, 0, 0, 0}, RoomEntitySpawnDef{320, 304, 43338, 0, 0, 0}, RoomEntitySpawnDef{608, 304, 43338, 0, 0, 0}, RoomEntitySpawnDef{624, 304, 43338, 0, 0, 0}, RoomEntitySpawnDef{752, 240, 43338, 0, 0, 0}, RoomEntitySpawnDef{768, 240, 43338, 0, 0, 0}, RoomEntitySpawnDef{960, 224, 43338, 0, 0, 0}, RoomEntitySpawnDef{1056, 128, 43338, 0, 0, 0}, RoomEntitySpawnDef{1088, 16, 43338, 0, 0, 0}, RoomEntitySpawnDef{1136, 96, 43338, 0, 0, 0}, RoomEntitySpawnDef{1200, 144, 43338, 0, 0, 0}, RoomEntitySpawnDef{1264, 208, 43338, 0, 0, 0}, RoomEntitySpawnDef{1232, 288, 43338, 0, 0, 0}, RoomEntitySpawnDef{1408, 160, 43386, 256, 0, 0}, RoomEntitySpawnDef{1424, 160, 43386, 256, 0, 0}, RoomEntitySpawnDef{1440, 160, 43386, 256, 0, 0}, RoomEntitySpawnDef{1456, 160, 43386, 256, 0, 0}, RoomEntitySpawnDef{1496, 208, 43322, 0, 0, 0}},
    /* room 3 */ {RoomEntitySpawnDef{64, 176, 43354, 0, 0, 0}, RoomEntitySpawnDef{512, 96, 43354, 0, 0, 0}, RoomEntitySpawnDef{560, 80, 43354, 0, 0, 0}, RoomEntitySpawnDef{800, 64, 43354, 0, 0, 0}, RoomEntitySpawnDef{1072, 240, 43354, 0, 0, 0}, RoomEntitySpawnDef{1232, 240, 43354, 0, 0, 0}, RoomEntitySpawnDef{992, 240, 43354, 0, 0, 0}, RoomEntitySpawnDef{944, 240, 43354, 0, 0, 0}, RoomEntitySpawnDef{272, 304, 43370, 0, 0, 0}, RoomEntitySpawnDef{288, 304, 43370, 0, 0, 0}, RoomEntitySpawnDef{304, 304, 43370, 0, 0, 0}, RoomEntitySpawnDef{416, 304, 43370, 0, 0, 0}, RoomEntitySpawnDef{432, 304, 43370, 0, 0, 0}, RoomEntitySpawnDef{448, 304, 43370, 0, 0, 0}, RoomEntitySpawnDef{464, 304, 43370, 0, 0, 0}, RoomEntitySpawnDef{704, 304, 43370, 0, 0, 0}, RoomEntitySpawnDef{720, 304, 43370, 0, 0, 0}, RoomEntitySpawnDef{736, 304, 43370, 0, 0, 0}, RoomEntitySpawnDef{752, 304, 43370, 0, 0, 0}},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl9Specials = {
    /* room 0 */ {RoomEntitySpawnDef{224, 272, 16, 11307, 11821, 65535}, RoomEntitySpawnDef{0, 0, 0, 752, 272, 16}, RoomEntitySpawnDef{11307, 11821, 9, 2, 0, 631}},
    /* room 1 */ {},
    /* room 2 */ {RoomEntitySpawnDef{208, 320, 16, 11307, 11821, 9}},
    /* room 3 */ {RoomEntitySpawnDef{192, 336, 16, 11307, 11821, 9}},
};

// ================= Level 10 Data =================
const std::vector<std::uint8_t> kLvl10TouchVals = {0};
const std::vector<std::uint8_t> kLvl10ProjVals = {0};
const std::vector<std::uint8_t> kLvl10Mode2Vals = {0};
const std::vector<std::vector<RoomTransitionZone>> kLvl10Zones = {
    /* room 0 */ {RoomTransitionZone{2384, 0, 96, 10, 1, 0}},
    /* room 1 */ {RoomTransitionZone{0, 0, 128, 10, 0, 0}},
    /* room 2 */ {},
    /* room 3 */ {},
    /* room 4 */ {RoomTransitionZone{0, 1552, 1568, 10, 5, 0}},
    /* room 5 */ {RoomTransitionZone{992, 240, 288, 10, 4, 0}},
    /* room 6 */ {},
    /* room 7 */ {},
    /* room 8 */ {},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl10Enemies = {
    /* room 0 */ {RoomEntitySpawnDef{240, 80, 44891, 0, 0, 0}, RoomEntitySpawnDef{320, 96, 44891, 0, 0, 0}, RoomEntitySpawnDef{400, 64, 44891, 0, 0, 0}, RoomEntitySpawnDef{480, 112, 44891, 0, 0, 0}, RoomEntitySpawnDef{560, 48, 44891, 0, 0, 0}, RoomEntitySpawnDef{640, 80, 44891, 0, 0, 0}, RoomEntitySpawnDef{272, 32, 44891, 0, 0, 0}, RoomEntitySpawnDef{352, 48, 44891, 0, 0, 0}, RoomEntitySpawnDef{432, 96, 44891, 0, 0, 0}, RoomEntitySpawnDef{512, 32, 44891, 0, 0, 0}, RoomEntitySpawnDef{592, 80, 44891, 0, 0, 0}, RoomEntitySpawnDef{672, 112, 44891, 0, 0, 0}, RoomEntitySpawnDef{992, 48, 44891, 0, 0, 0}, RoomEntitySpawnDef{1040, 96, 44891, 0, 0, 0}, RoomEntitySpawnDef{1184, 80, 44891, 0, 0, 0}, RoomEntitySpawnDef{1344, 64, 44891, 0, 0, 0}, RoomEntitySpawnDef{1424, 16, 44891, 0, 0, 0}, RoomEntitySpawnDef{1504, 16, 44891, 0, 0, 0}, RoomEntitySpawnDef{1600, 96, 44891, 0, 0, 0}, RoomEntitySpawnDef{1648, 48, 44891, 0, 0, 0}, RoomEntitySpawnDef{1792, 16, 44891, 0, 0, 0}, RoomEntitySpawnDef{1920, 80, 44891, 0, 0, 0}, RoomEntitySpawnDef{2064, 16, 44891, 0, 0, 0}, RoomEntitySpawnDef{1408, 32, 44907, 0, 0, 0}, RoomEntitySpawnDef{1456, 80, 44907, 0, 0, 0}, RoomEntitySpawnDef{1520, 128, 44907, 0, 0, 0}, RoomEntitySpawnDef{1728, 16, 44907, 0, 0, 0}, RoomEntitySpawnDef{1808, 128, 44907, 0, 0, 0}, RoomEntitySpawnDef{1856, 64, 44907, 0, 0, 0}, RoomEntitySpawnDef{1936, 16, 44907, 0, 0, 0}, RoomEntitySpawnDef{2192, 48, 44907, 0, 0, 0}, RoomEntitySpawnDef{2304, 16, 44907, 0, 0, 0}, RoomEntitySpawnDef{800, 80, 44907, 0, 0, 0}, RoomEntitySpawnDef{960, 32, 44907, 0, 0, 0}, RoomEntitySpawnDef{1584, 64, 44907, 0, 0, 0}, RoomEntitySpawnDef{1632, 0, 44907, 0, 0, 0}, RoomEntitySpawnDef{1904, 0, 44907, 0, 0, 0}, RoomEntitySpawnDef{1968, 48, 44907, 0, 0, 0}, RoomEntitySpawnDef{2000, 80, 44907, 0, 0, 0}},
    /* room 1 */ {RoomEntitySpawnDef{160, 112, 44907, 0, 0, 0}, RoomEntitySpawnDef{192, 96, 44907, 0, 0, 0}, RoomEntitySpawnDef{352, 96, 44907, 0, 0, 0}, RoomEntitySpawnDef{320, 112, 44907, 0, 0, 0}, RoomEntitySpawnDef{1856, 64, 44907, 0, 0, 0}, RoomEntitySpawnDef{1920, 16, 44907, 0, 0, 0}, RoomEntitySpawnDef{2064, 64, 44907, 0, 0, 0}, RoomEntitySpawnDef{2224, 64, 44907, 0, 0, 0}, RoomEntitySpawnDef{160, 144, 44939, 0, 0, 0}, RoomEntitySpawnDef{560, 160, 44939, 0, 0, 0}, RoomEntitySpawnDef{592, 128, 44939, 0, 0, 0}, RoomEntitySpawnDef{640, 96, 44939, 0, 0, 0}, RoomEntitySpawnDef{688, 128, 44939, 0, 0, 0}, RoomEntitySpawnDef{784, 144, 44939, 0, 0, 0}, RoomEntitySpawnDef{992, 112, 44939, 0, 0, 0}, RoomEntitySpawnDef{1040, 64, 44939, 0, 0, 0}, RoomEntitySpawnDef{1328, 128, 44939, 0, 0, 0}, RoomEntitySpawnDef{1664, 80, 44939, 0, 0, 0}, RoomEntitySpawnDef{528, 80, 44923, 0, 0, 0}, RoomEntitySpawnDef{528, 128, 44923, 0, 0, 0}, RoomEntitySpawnDef{640, 160, 44923, 0, 0, 0}, RoomEntitySpawnDef{688, 80, 44923, 0, 0, 0}, RoomEntitySpawnDef{688, 128, 44923, 0, 0, 0}, RoomEntitySpawnDef{848, 144, 44923, 0, 0, 0}, RoomEntitySpawnDef{800, 64, 44923, 0, 0, 0}, RoomEntitySpawnDef{1056, 48, 44923, 0, 0, 0}, RoomEntitySpawnDef{1056, 96, 44923, 0, 0, 0}, RoomEntitySpawnDef{2192, 32, 44923, 0, 0, 0}},
    /* room 2 */ {RoomEntitySpawnDef{80, 80, 44955, 0, 0, 0}},
    /* room 3 */ {RoomEntitySpawnDef{96, 112, 44955, 0, 0, 0}, RoomEntitySpawnDef{128, 56, 44971, 0, 0, 0}, RoomEntitySpawnDef{240, 56, 44971, 0, 0, 0}},
    /* room 4 */ {RoomEntitySpawnDef{160, 1392, 44987, 0, 0, 0}, RoomEntitySpawnDef{160, 1440, 44987, 0, 0, 0}, RoomEntitySpawnDef{144, 1504, 44987, 0, 0, 0}, RoomEntitySpawnDef{128, 1536, 44987, 0, 0, 0}, RoomEntitySpawnDef{80, 1552, 44987, 0, 0, 0}, RoomEntitySpawnDef{32, 1456, 44987, 0, 0, 0}, RoomEntitySpawnDef{16, 1552, 44987, 0, 0, 0}, RoomEntitySpawnDef{16, 1376, 44987, 0, 0, 0}, RoomEntitySpawnDef{80, 1424, 44987, 0, 0, 0}, RoomEntitySpawnDef{16, 592, 45003, 0, 0, 0}, RoomEntitySpawnDef{176, 592, 45003, 0, 0, 0}, RoomEntitySpawnDef{32, 544, 45003, 0, 0, 0}, RoomEntitySpawnDef{160, 544, 45003, 0, 0, 0}, RoomEntitySpawnDef{48, 496, 45003, 0, 0, 0}, RoomEntitySpawnDef{144, 496, 45003, 0, 0, 0}, RoomEntitySpawnDef{64, 448, 45003, 0, 0, 0}, RoomEntitySpawnDef{128, 448, 45003, 0, 0, 0}},
    /* room 5 */ {RoomEntitySpawnDef{176, 32, 45003, 0, 0, 0}, RoomEntitySpawnDef{240, 64, 45003, 0, 0, 0}, RoomEntitySpawnDef{304, 96, 45003, 0, 0, 0}, RoomEntitySpawnDef{352, 112, 45003, 0, 0, 0}, RoomEntitySpawnDef{368, 112, 45003, 0, 0, 0}, RoomEntitySpawnDef{416, 96, 45003, 0, 0, 0}, RoomEntitySpawnDef{480, 64, 45003, 0, 0, 0}, RoomEntitySpawnDef{544, 32, 45003, 0, 0, 0}, RoomEntitySpawnDef{912, 112, 45035, 0, 0, 0}, RoomEntitySpawnDef{896, 256, 44987, 0, 0, 0}, RoomEntitySpawnDef{784, 240, 44987, 0, 0, 0}, RoomEntitySpawnDef{720, 288, 44987, 0, 0, 0}, RoomEntitySpawnDef{656, 224, 44987, 0, 0, 0}, RoomEntitySpawnDef{576, 272, 44987, 0, 0, 0}, RoomEntitySpawnDef{480, 272, 44987, 0, 0, 0}, RoomEntitySpawnDef{448, 240, 44987, 0, 0, 0}, RoomEntitySpawnDef{256, 272, 44987, 0, 0, 0}, RoomEntitySpawnDef{128, 256, 44987, 0, 0, 0}, RoomEntitySpawnDef{32, 144, 45003, 0, 0, 0}, RoomEntitySpawnDef{80, 96, 45003, 0, 0, 0}, RoomEntitySpawnDef{96, 16, 45003, 0, 0, 0}, RoomEntitySpawnDef{752, 128, 45019, 0, 0, 0}, RoomEntitySpawnDef{848, 96, 45019, 0, 0, 0}, RoomEntitySpawnDef{880, 96, 45019, 0, 0, 0}, RoomEntitySpawnDef{896, 96, 45019, 256, 0, 0}},
    /* room 6 */ {RoomEntitySpawnDef{400, 208, 45003, 0, 0, 0}, RoomEntitySpawnDef{416, 208, 45003, 0, 0, 0}, RoomEntitySpawnDef{400, 144, 45003, 0, 0, 0}, RoomEntitySpawnDef{368, 80, 45003, 0, 0, 0}, RoomEntitySpawnDef{384, 80, 45003, 0, 0, 0}, RoomEntitySpawnDef{400, 80, 45003, 0, 0, 0}, RoomEntitySpawnDef{416, 80, 45003, 0, 0, 0}, RoomEntitySpawnDef{288, 112, 45003, 0, 0, 0}, RoomEntitySpawnDef{304, 112, 45003, 0, 0, 0}, RoomEntitySpawnDef{208, 192, 45003, 0, 0, 0}, RoomEntitySpawnDef{128, 64, 45003, 0, 0, 0}, RoomEntitySpawnDef{144, 64, 45003, 0, 0, 0}, RoomEntitySpawnDef{128, 320, 44987, 0, 0, 0}, RoomEntitySpawnDef{192, 336, 44987, 0, 0, 0}, RoomEntitySpawnDef{272, 320, 44987, 0, 0, 0}, RoomEntitySpawnDef{336, 352, 44987, 0, 0, 0}, RoomEntitySpawnDef{336, 320, 44987, 0, 0, 0}, RoomEntitySpawnDef{400, 336, 44987, 0, 0, 0}},
    /* room 7 */ {RoomEntitySpawnDef{192, 592, 44923, 0, 0, 0}, RoomEntitySpawnDef{144, 480, 44923, 0, 0, 0}, RoomEntitySpawnDef{240, 400, 44923, 0, 0, 0}, RoomEntitySpawnDef{288, 192, 44923, 0, 0, 0}, RoomEntitySpawnDef{32, 64, 44923, 0, 0, 0}, RoomEntitySpawnDef{672, 608, 44923, 0, 0, 0}, RoomEntitySpawnDef{704, 624, 44923, 0, 0, 0}, RoomEntitySpawnDef{720, 672, 44923, 0, 0, 0}, RoomEntitySpawnDef{704, 704, 44923, 0, 0, 0}},
    /* room 8 */ {RoomEntitySpawnDef{112, 224, 44923, 0, 0, 0}, RoomEntitySpawnDef{48, 192, 44923, 0, 0, 0}, RoomEntitySpawnDef{256, 224, 44923, 0, 0, 0}, RoomEntitySpawnDef{304, 160, 44923, 0, 0, 0}},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl10Specials = {
    /* room 0 */ {RoomEntitySpawnDef{80, 144, 0, 255, 0, 65535}, RoomEntitySpawnDef{0, 0, 0, 960, 96, 0}},
    /* room 1 */ {RoomEntitySpawnDef{1408, 128, 0, 255, 0, 10}, RoomEntitySpawnDef{3, 0, 0, 2128, 80, 0}},
    /* room 2 */ {RoomEntitySpawnDef{144, 80, 0, 255, 0, 10}},
    /* room 3 */ {RoomEntitySpawnDef{256, 112, 0, 255, 0, 10}},
    /* room 4 */ {RoomEntitySpawnDef{96, 16, 0, 255, 0, 10}},
    /* room 5 */ {},
    /* room 6 */ {RoomEntitySpawnDef{48, 16, 0, 255, 0, 10}, RoomEntitySpawnDef{1, 1, 0, 240, 16, 0}, RoomEntitySpawnDef{255, 0, 10, 7, 0, 0}},
    /* room 7 */ {RoomEntitySpawnDef{96, 672, 0, 255, 0, 10}, RoomEntitySpawnDef{6, 1, 0, 624, 704, 0}},
    /* room 8 */ {RoomEntitySpawnDef{192, 288, 0, 255, 0, 10}},
};

// ================= Level 11 Data =================
const std::vector<std::uint8_t> kLvl11TouchVals = {76};
const std::vector<std::uint8_t> kLvl11ProjVals = {97};
const std::vector<std::uint8_t> kLvl11Mode2Vals = {0};
const std::vector<std::vector<RoomTransitionZone>> kLvl11Zones = {
    /* room 0 */ {RoomTransitionZone{2032, 96, 176, 11, 1, 0}, RoomTransitionZone{2032, 224, 272, 11, 1, 1}, RoomTransitionZone{0, 0, 272, 65535, 0, 0}},
    /* room 1 */ {RoomTransitionZone{0, 0, 176, 11, 0, 0}, RoomTransitionZone{0, 224, 272, 11, 0, 1}},
    /* room 2 */ {},
    /* room 3 */ {},
    /* room 4 */ {},
    /* room 5 */ {},
    /* room 6 */ {},
    /* room 7 */ {},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl11Enemies = {
    /* room 0 */ {RoomEntitySpawnDef{368, 288, 47416, 0, 0, 0}, RoomEntitySpawnDef{560, 224, 47384, 0, 0, 0}, RoomEntitySpawnDef{576, 208, 47384, 0, 0, 0}, RoomEntitySpawnDef{576, 240, 47384, 0, 0, 0}, RoomEntitySpawnDef{592, 224, 47384, 0, 0, 0}, RoomEntitySpawnDef{720, 224, 47384, 0, 0, 0}, RoomEntitySpawnDef{928, 224, 47384, 0, 0, 0}, RoomEntitySpawnDef{944, 240, 47384, 0, 0, 0}, RoomEntitySpawnDef{960, 256, 47384, 0, 0, 0}, RoomEntitySpawnDef{976, 272, 47384, 0, 0, 0}, RoomEntitySpawnDef{992, 288, 47384, 0, 0, 0}, RoomEntitySpawnDef{1616, 0, 47384, 0, 0, 0}, RoomEntitySpawnDef{1680, 0, 47384, 0, 0, 0}, RoomEntitySpawnDef{1760, 0, 47384, 0, 0, 0}, RoomEntitySpawnDef{1360, 0, 47384, 0, 0, 0}, RoomEntitySpawnDef{800, 296, 47400, 0, 0, 0}, RoomEntitySpawnDef{1136, 296, 47400, 0, 0, 0}, RoomEntitySpawnDef{1264, 72, 47400, 0, 0, 0}, RoomEntitySpawnDef{1440, 296, 47400, 0, 0, 0}, RoomEntitySpawnDef{1632, 168, 47400, 0, 0, 0}, RoomEntitySpawnDef{1584, 296, 47400, 0, 0, 0}, RoomEntitySpawnDef{1488, 72, 47400, 0, 0, 0}},
    /* room 1 */ {RoomEntitySpawnDef{240, 224, 47384, 0, 0, 0}, RoomEntitySpawnDef{352, 224, 47384, 0, 0, 0}, RoomEntitySpawnDef{928, 32, 47384, 0, 0, 0}, RoomEntitySpawnDef{944, 16, 47384, 0, 0, 0}, RoomEntitySpawnDef{944, 48, 47384, 0, 0, 0}, RoomEntitySpawnDef{960, 32, 47384, 0, 0, 0}, RoomEntitySpawnDef{1216, 160, 47384, 0, 0, 0}, RoomEntitySpawnDef{1280, 160, 47384, 0, 0, 0}, RoomEntitySpawnDef{1392, 48, 47384, 0, 0, 0}, RoomEntitySpawnDef{1408, 32, 47384, 0, 0, 0}, RoomEntitySpawnDef{1408, 64, 47384, 0, 0, 0}, RoomEntitySpawnDef{464, 296, 47400, 0, 0, 0}, RoomEntitySpawnDef{544, 296, 47400, 0, 0, 0}, RoomEntitySpawnDef{656, 296, 47400, 0, 0, 0}, RoomEntitySpawnDef{752, 296, 47400, 0, 0, 0}, RoomEntitySpawnDef{880, 296, 47400, 0, 0, 0}, RoomEntitySpawnDef{1280, 104, 47400, 0, 0, 0}, RoomEntitySpawnDef{1456, 296, 47400, 0, 0, 0}, RoomEntitySpawnDef{1792, 296, 47400, 0, 0, 0}},
    /* room 2 */ {RoomEntitySpawnDef{176, 144, 47384, 0, 0, 0}, RoomEntitySpawnDef{192, 128, 47384, 0, 0, 0}, RoomEntitySpawnDef{192, 160, 47384, 0, 0, 0}, RoomEntitySpawnDef{208, 144, 47384, 0, 0, 0}, RoomEntitySpawnDef{672, 288, 47384, 0, 0, 0}, RoomEntitySpawnDef{672, 272, 47384, 0, 0, 0}, RoomEntitySpawnDef{672, 256, 47384, 0, 0, 0}, RoomEntitySpawnDef{672, 240, 47384, 0, 0, 0}, RoomEntitySpawnDef{672, 224, 47384, 0, 0, 0}, RoomEntitySpawnDef{672, 208, 47384, 0, 0, 0}, RoomEntitySpawnDef{368, 168, 47400, 0, 0, 0}, RoomEntitySpawnDef{928, 264, 47400, 0, 0, 0}},
    /* room 3 */ {RoomEntitySpawnDef{352, 48, 47368, 0, 0, 0}, RoomEntitySpawnDef{384, 128, 47368, 0, 0, 0}, RoomEntitySpawnDef{320, 128, 47368, 0, 0, 0}, RoomEntitySpawnDef{48, 48, 47368, 0, 0, 0}},
    /* room 4 */ {RoomEntitySpawnDef{1600, 176, 47368, 0, 0, 0}, RoomEntitySpawnDef{1840, 80, 47368, 0, 0, 0}, RoomEntitySpawnDef{2000, 288, 47368, 0, 0, 0}, RoomEntitySpawnDef{1728, 288, 47368, 0, 0, 0}, RoomEntitySpawnDef{1200, 96, 47368, 0, 0, 0}, RoomEntitySpawnDef{1488, 128, 47368, 0, 0, 0}, RoomEntitySpawnDef{992, 288, 47368, 0, 0, 0}, RoomEntitySpawnDef{672, 192, 47368, 0, 0, 0}, RoomEntitySpawnDef{16, 112, 47368, 0, 0, 0}, RoomEntitySpawnDef{112, 192, 47384, 0, 0, 0}, RoomEntitySpawnDef{384, 112, 47384, 0, 0, 0}, RoomEntitySpawnDef{496, 160, 47384, 0, 0, 0}, RoomEntitySpawnDef{752, 144, 47384, 0, 0, 0}, RoomEntitySpawnDef{1040, 64, 47384, 0, 0, 0}, RoomEntitySpawnDef{1152, 48, 47384, 0, 0, 0}, RoomEntitySpawnDef{1264, 64, 47384, 0, 0, 0}, RoomEntitySpawnDef{2000, 272, 47384, 0, 0, 0}, RoomEntitySpawnDef{1984, 176, 47384, 0, 0, 0}},
    /* room 5 */ {RoomEntitySpawnDef{96, 80, 47384, 0, 0, 0}, RoomEntitySpawnDef{352, 32, 47384, 0, 0, 0}, RoomEntitySpawnDef{352, 128, 47384, 0, 0, 0}},
    /* room 6 */ {RoomEntitySpawnDef{160, 96, 47368, 0, 0, 0}, RoomEntitySpawnDef{256, 192, 47368, 0, 0, 0}, RoomEntitySpawnDef{352, 128, 47368, 0, 0, 0}, RoomEntitySpawnDef{112, 160, 47384, 0, 0, 0}, RoomEntitySpawnDef{96, 16, 47384, 0, 0, 0}},
    /* room 7 */ {},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl11Specials = {
    /* room 0 */ {RoomEntitySpawnDef{0, 272, 0, 255, 0, 65535}, RoomEntitySpawnDef{0, 0, 0, 1024, 272, 16}, RoomEntitySpawnDef{255, 0, 11, 3, 0, 0}, RoomEntitySpawnDef{1232, 48, 0, 255, 0, 11}},
    /* room 1 */ {RoomEntitySpawnDef{1520, 80, 16, 255, 0, 11}, RoomEntitySpawnDef{5, 0, 0, 1968, 176, 0}},
    /* room 2 */ {RoomEntitySpawnDef{64, 176, 16, 255, 0, 11}, RoomEntitySpawnDef{4, 0, 0, 912, 240, 16}},
    /* room 3 */ {RoomEntitySpawnDef{192, 112, 16, 255, 0, 11}},
    /* room 4 */ {RoomEntitySpawnDef{32, 96, 16, 255, 0, 11}, RoomEntitySpawnDef{2, 0, 0, 1696, 160, 0}, RoomEntitySpawnDef{255, 0, 11, 0, 3, 0}},
    /* room 5 */ {RoomEntitySpawnDef{192, 112, 16, 255, 0, 11}},
    /* room 6 */ {RoomEntitySpawnDef{192, 272, 0, 255, 0, 11}},
    /* room 7 */ {RoomEntitySpawnDef{48, 112, 16, 255, 0, 11}, RoomEntitySpawnDef{2, 1, 0, 656, 96, 16}},
};

// ================= Level 12 Data =================
const std::vector<std::uint8_t> kLvl12TouchVals = {76};
const std::vector<std::uint8_t> kLvl12ProjVals = {0};
const std::vector<std::uint8_t> kLvl12Mode2Vals = {0};
const std::vector<std::vector<RoomTransitionZone>> kLvl12Zones = {
    /* room 0 */ {},
    /* room 1 */ {RoomTransitionZone{0, 0, 80, 12, 2, 0}, RoomTransitionZone{0, 128, 272, 12, 2, 1}},
    /* room 2 */ {RoomTransitionZone{2032, 0, 80, 12, 1, 0}, RoomTransitionZone{2032, 128, 272, 12, 1, 1}},
    /* room 3 */ {},
    /* room 4 */ {},
    /* room 5 */ {},
    /* room 6 */ {},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl12Enemies = {
    /* room 0 */ {RoomEntitySpawnDef{368, 112, 49003, 0, 0, 0}},
    /* room 1 */ {RoomEntitySpawnDef{1344, 176, 48923, 0, 0, 0}, RoomEntitySpawnDef{1152, 112, 48923, 0, 0, 0}, RoomEntitySpawnDef{1168, 112, 48923, 0, 0, 0}, RoomEntitySpawnDef{1168, 80, 49003, 0, 0, 0}, RoomEntitySpawnDef{1072, 176, 49003, 0, 0, 0}, RoomEntitySpawnDef{1696, 272, 49003, 0, 0, 0}, RoomEntitySpawnDef{576, 272, 49003, 0, 0, 0}, RoomEntitySpawnDef{512, 80, 49003, 0, 0, 0}, RoomEntitySpawnDef{1920, 208, 48955, 256, 0, 0}, RoomEntitySpawnDef{1968, 208, 48955, 256, 0, 0}, RoomEntitySpawnDef{1936, 64, 48955, 256, 0, 0}, RoomEntitySpawnDef{1952, 64, 48955, 256, 0, 0}, RoomEntitySpawnDef{1296, 176, 48955, 256, 0, 0}, RoomEntitySpawnDef{1136, 112, 48955, 256, 0, 0}, RoomEntitySpawnDef{608, 240, 48955, 256, 0, 0}, RoomEntitySpawnDef{576, 176, 48955, 256, 0, 0}, RoomEntitySpawnDef{576, 112, 48955, 256, 0, 0}, RoomEntitySpawnDef{480, 112, 48955, 256, 0, 0}, RoomEntitySpawnDef{416, 112, 48955, 256, 0, 0}, RoomEntitySpawnDef{656, 112, 48955, 256, 0, 0}, RoomEntitySpawnDef{432, 176, 48955, 256, 0, 0}, RoomEntitySpawnDef{1584, 176, 48955, 256, 0, 0}, RoomEntitySpawnDef{1488, 176, 48955, 256, 0, 0}, RoomEntitySpawnDef{1088, 208, 48955, 256, 0, 0}, RoomEntitySpawnDef{1456, 176, 48955, 256, 0, 0}, RoomEntitySpawnDef{1040, 224, 48955, 256, 0, 0}, RoomEntitySpawnDef{1120, 112, 48955, 256, 0, 0}, RoomEntitySpawnDef{1440, 176, 48955, 256, 0, 0}},
    /* room 2 */ {RoomEntitySpawnDef{1200, 112, 48923, 0, 0, 0}, RoomEntitySpawnDef{1248, 112, 48923, 0, 0, 0}, RoomEntitySpawnDef{1312, 112, 48923, 0, 0, 0}, RoomEntitySpawnDef{752, 208, 48939, 0, 0, 0}, RoomEntitySpawnDef{608, 224, 48939, 0, 0, 0}, RoomEntitySpawnDef{720, 224, 48939, 0, 0, 0}, RoomEntitySpawnDef{464, 224, 48939, 0, 0, 0}, RoomEntitySpawnDef{1824, 80, 49003, 0, 0, 0}, RoomEntitySpawnDef{1568, 80, 49003, 0, 0, 0}, RoomEntitySpawnDef{1392, 272, 49003, 0, 0, 0}, RoomEntitySpawnDef{800, 96, 49003, 0, 0, 0}, RoomEntitySpawnDef{64, 48, 48955, 256, 0, 0}, RoomEntitySpawnDef{112, 48, 48955, 256, 0, 0}, RoomEntitySpawnDef{592, 128, 48955, 256, 0, 0}, RoomEntitySpawnDef{752, 48, 48955, 256, 0, 0}, RoomEntitySpawnDef{784, 48, 48955, 256, 0, 0}, RoomEntitySpawnDef{1040, 0, 48955, 256, 0, 0}, RoomEntitySpawnDef{1104, 112, 48955, 256, 0, 0}, RoomEntitySpawnDef{1120, 112, 48955, 256, 0, 0}, RoomEntitySpawnDef{1616, 112, 48955, 256, 0, 0}, RoomEntitySpawnDef{1648, 112, 48955, 256, 0, 0}, RoomEntitySpawnDef{1872, 48, 48955, 256, 0, 0}, RoomEntitySpawnDef{1936, 208, 48955, 256, 0, 0}, RoomEntitySpawnDef{1872, 240, 48955, 256, 0, 0}, RoomEntitySpawnDef{1680, 128, 48955, 256, 0, 0}, RoomEntitySpawnDef{1664, 48, 48955, 256, 0, 0}, RoomEntitySpawnDef{688, 48, 48955, 256, 0, 0}, RoomEntitySpawnDef{656, 48, 48955, 256, 0, 0}},
    /* room 3 */ {RoomEntitySpawnDef{768, 128, 48923, 0, 0, 0}, RoomEntitySpawnDef{704, 160, 48923, 0, 0, 0}, RoomEntitySpawnDef{640, 192, 48923, 0, 0, 0}, RoomEntitySpawnDef{928, 96, 48923, 0, 0, 0}, RoomEntitySpawnDef{944, 96, 48923, 0, 0, 0}, RoomEntitySpawnDef{960, 96, 48923, 0, 0, 0}, RoomEntitySpawnDef{976, 96, 48923, 0, 0, 0}, RoomEntitySpawnDef{416, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{432, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{448, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{464, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{176, 144, 48939, 0, 0, 0}, RoomEntitySpawnDef{160, 144, 48939, 0, 0, 0}, RoomEntitySpawnDef{144, 144, 48939, 0, 0, 0}, RoomEntitySpawnDef{480, 144, 48939, 0, 0, 0}},
    /* room 4 */ {RoomEntitySpawnDef{160, 80, 48923, 0, 0, 0}, RoomEntitySpawnDef{272, 80, 48923, 0, 0, 0}, RoomEntitySpawnDef{128, 144, 48923, 0, 0, 0}, RoomEntitySpawnDef{224, 144, 48923, 0, 0, 0}, RoomEntitySpawnDef{304, 144, 48923, 0, 0, 0}, RoomEntitySpawnDef{304, 208, 48923, 0, 0, 0}, RoomEntitySpawnDef{224, 208, 48923, 0, 0, 0}, RoomEntitySpawnDef{144, 208, 48923, 0, 0, 0}, RoomEntitySpawnDef{208, 208, 48923, 0, 0, 0}, RoomEntitySpawnDef{112, 240, 49003, 0, 0, 0}},
    /* room 5 */ {RoomEntitySpawnDef{144, 240, 48923, 0, 0, 0}, RoomEntitySpawnDef{208, 240, 48923, 0, 0, 0}, RoomEntitySpawnDef{256, 240, 48923, 0, 0, 0}, RoomEntitySpawnDef{128, 64, 48923, 0, 0, 0}, RoomEntitySpawnDef{320, 64, 48923, 0, 0, 0}, RoomEntitySpawnDef{368, 64, 48923, 0, 0, 0}, RoomEntitySpawnDef{496, 64, 48923, 0, 0, 0}, RoomEntitySpawnDef{560, 224, 48939, 0, 0, 0}, RoomEntitySpawnDef{576, 224, 48939, 0, 0, 0}, RoomEntitySpawnDef{592, 224, 48939, 0, 0, 0}, RoomEntitySpawnDef{592, 32, 48971, 0, 0, 0}, RoomEntitySpawnDef{656, 32, 48971, 0, 0, 0}, RoomEntitySpawnDef{784, 32, 48971, 0, 0, 0}, RoomEntitySpawnDef{928, 32, 48971, 0, 0, 0}, RoomEntitySpawnDef{1056, 32, 48971, 0, 0, 0}, RoomEntitySpawnDef{1184, 32, 48971, 0, 0, 0}, RoomEntitySpawnDef{1456, 144, 48971, 0, 0, 0}, RoomEntitySpawnDef{1632, 144, 48971, 0, 0, 0}, RoomEntitySpawnDef{1744, 144, 48971, 0, 0, 0}, RoomEntitySpawnDef{1808, 144, 48971, 0, 0, 0}, RoomEntitySpawnDef{64, 272, 49003, 0, 0, 0}, RoomEntitySpawnDef{80, 96, 49003, 0, 0, 0}, RoomEntitySpawnDef{448, 96, 49003, 0, 0, 0}, RoomEntitySpawnDef{672, 96, 49003, 0, 0, 0}, RoomEntitySpawnDef{1956, 240, 49019, 0, 0, 0}, RoomEntitySpawnDef{1968, 240, 48987, 256, 0, 0}, RoomEntitySpawnDef{1968, 256, 48987, 256, 0, 0}},
    /* room 6 */ {RoomEntitySpawnDef{32, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{64, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{96, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{128, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{160, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{192, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{224, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{256, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{288, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{320, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{352, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{384, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{416, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{448, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{480, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{512, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{544, 16, 48939, 0, 0, 0}, RoomEntitySpawnDef{576, 16, 48939, 0, 0, 0}},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl12Specials = {
    /* room 0 */ {RoomEntitySpawnDef{656, 96, 16, 20816, 21330, 11}, RoomEntitySpawnDef{7, 1, 635, 48, 112, 16}},
    /* room 1 */ {RoomEntitySpawnDef{1936, 240, 16, 255, 0, 12}, RoomEntitySpawnDef{0, 1, 0, 1088, 176, 16}, RoomEntitySpawnDef{255, 0, 12, 3, 0, 0}},
    /* room 2 */ {RoomEntitySpawnDef{80, 96, 16, 255, 0, 12}, RoomEntitySpawnDef{5, 0, 0, 80, 240, 16}, RoomEntitySpawnDef{255, 0, 12, 5, 1, 0}},
    /* room 3 */ {RoomEntitySpawnDef{928, 64, 16, 255, 0, 12}},
    /* room 4 */ {RoomEntitySpawnDef{208, 112, 16, 255, 0, 12}},
    /* room 5 */ {RoomEntitySpawnDef{96, 96, 16, 255, 0, 12}, RoomEntitySpawnDef{2, 0, 0, 96, 272, 16}},
    /* room 6 */ {RoomEntitySpawnDef{48, 128, 16, 255, 0, 12}},
};

// ================= Level 13 Data =================
const std::vector<std::uint8_t> kLvl13TouchVals = {0};
const std::vector<std::uint8_t> kLvl13ProjVals = {0};
const std::vector<std::uint8_t> kLvl13Mode2Vals = {0};
const std::vector<std::vector<RoomTransitionZone>> kLvl13Zones = {
    /* room 0 */ {RoomTransitionZone{0, 2752, 2816, 65535, 0, 0}},
    /* room 1 */ {},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl13Enemies = {
    /* room 0 */ {RoomEntitySpawnDef{104, 2712, 50892, 0, 0, 0}, RoomEntitySpawnDef{192, 912, 50924, 0, 0, 0}, RoomEntitySpawnDef{128, 896, 50924, 0, 0, 0}, RoomEntitySpawnDef{0, 912, 50924, 0, 0, 0}, RoomEntitySpawnDef{64, 896, 50924, 0, 0, 0}, RoomEntitySpawnDef{0, 800, 50924, 0, 0, 0}, RoomEntitySpawnDef{64, 800, 50924, 0, 0, 0}, RoomEntitySpawnDef{128, 800, 50924, 0, 0, 0}, RoomEntitySpawnDef{192, 800, 50924, 0, 0, 0}, RoomEntitySpawnDef{176, 624, 50924, 0, 0, 0}, RoomEntitySpawnDef{112, 608, 50924, 0, 0, 0}, RoomEntitySpawnDef{48, 592, 50924, 0, 0, 0}, RoomEntitySpawnDef{0, 576, 50924, 0, 0, 0}, RoomEntitySpawnDef{112, 560, 50924, 0, 0, 0}, RoomEntitySpawnDef{160, 544, 50924, 0, 0, 0}, RoomEntitySpawnDef{0, 224, 50924, 0, 0, 0}, RoomEntitySpawnDef{48, 224, 50924, 0, 0, 0}, RoomEntitySpawnDef{80, 224, 50924, 0, 0, 0}, RoomEntitySpawnDef{112, 224, 50924, 0, 0, 0}, RoomEntitySpawnDef{144, 224, 50924, 0, 0, 0}, RoomEntitySpawnDef{176, 224, 50924, 0, 0, 0}, RoomEntitySpawnDef{96, 1504, 50924, 0, 0, 0}, RoomEntitySpawnDef{64, 1472, 50924, 0, 0, 0}, RoomEntitySpawnDef{128, 1440, 50924, 0, 0, 0}, RoomEntitySpawnDef{16, 1408, 50924, 0, 0, 0}, RoomEntitySpawnDef{160, 1376, 50924, 0, 0, 0}, RoomEntitySpawnDef{32, 1344, 50924, 0, 0, 0}, RoomEntitySpawnDef{96, 1184, 50924, 0, 0, 0}, RoomEntitySpawnDef{64, 1152, 50924, 0, 0, 0}, RoomEntitySpawnDef{128, 1120, 50924, 0, 0, 0}, RoomEntitySpawnDef{16, 1088, 50924, 0, 0, 0}, RoomEntitySpawnDef{160, 1056, 50924, 0, 0, 0}},
    /* room 1 */ {RoomEntitySpawnDef{160, 96, 50956, 0, 0, 0}, RoomEntitySpawnDef{720, 16, 50908, 0, 0, 0}, RoomEntitySpawnDef{720, 48, 50908, 0, 0, 0}, RoomEntitySpawnDef{720, 80, 50908, 0, 0, 0}, RoomEntitySpawnDef{720, 144, 50908, 0, 0, 0}, RoomEntitySpawnDef{928, 128, 50908, 0, 0, 0}, RoomEntitySpawnDef{928, 96, 50908, 0, 0, 0}, RoomEntitySpawnDef{928, 64, 50908, 0, 0, 0}, RoomEntitySpawnDef{928, 0, 50908, 0, 0, 0}, RoomEntitySpawnDef{1280, 16, 50908, 0, 0, 0}, RoomEntitySpawnDef{1280, 48, 50908, 0, 0, 0}, RoomEntitySpawnDef{1280, 80, 50908, 0, 0, 0}, RoomEntitySpawnDef{1280, 112, 50908, 0, 0, 0}, RoomEntitySpawnDef{1600, 32, 50908, 0, 0, 0}, RoomEntitySpawnDef{1600, 64, 50908, 0, 0, 0}, RoomEntitySpawnDef{1600, 96, 50908, 0, 0, 0}, RoomEntitySpawnDef{1600, 128, 50908, 0, 0, 0}, RoomEntitySpawnDef{1920, 16, 50908, 0, 0, 0}, RoomEntitySpawnDef{1920, 48, 50908, 0, 0, 0}, RoomEntitySpawnDef{1920, 80, 50908, 0, 0, 0}, RoomEntitySpawnDef{1920, 144, 50908, 0, 0, 0}, RoomEntitySpawnDef{2240, 16, 50908, 0, 0, 0}, RoomEntitySpawnDef{2256, 48, 50908, 0, 0, 0}, RoomEntitySpawnDef{2272, 80, 50908, 0, 0, 0}, RoomEntitySpawnDef{2256, 112, 50908, 0, 0, 0}, RoomEntitySpawnDef{2240, 144, 50908, 0, 0, 0}, RoomEntitySpawnDef{256, 16, 50940, 0, 0, 0}, RoomEntitySpawnDef{608, 48, 50940, 0, 0, 0}, RoomEntitySpawnDef{848, 80, 50940, 0, 0, 0}, RoomEntitySpawnDef{1120, 48, 50940, 0, 0, 0}, RoomEntitySpawnDef{1568, 64, 50940, 0, 0, 0}, RoomEntitySpawnDef{1760, 112, 50940, 0, 0, 0}, RoomEntitySpawnDef{1904, 32, 50940, 0, 0, 0}, RoomEntitySpawnDef{2160, 80, 50940, 0, 0, 0}, RoomEntitySpawnDef{2448, 32, 50940, 0, 0, 0}, RoomEntitySpawnDef{720, 64, 50940, 0, 0, 0}, RoomEntitySpawnDef{960, 64, 50940, 0, 0, 0}, RoomEntitySpawnDef{2672, 128, 50972, 0, 0, 0}},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl13Specials = {
    /* room 0 */ {RoomEntitySpawnDef{0, 2816, 0, 255, 0, 65535}, RoomEntitySpawnDef{0, 0, 0, 96, 80, 16}},
    /* room 1 */ {RoomEntitySpawnDef{96, 80, 16, 255, 0, 13}, RoomEntitySpawnDef{0, 1, 643, 2752, 112, 16}},
};

// ================= Level 14 Data =================
const std::vector<std::uint8_t> kLvl14TouchVals = {0};
const std::vector<std::uint8_t> kLvl14ProjVals = {50, 16, 16, 16, 16};
const std::vector<std::uint8_t> kLvl14Mode2Vals = {3};
const std::vector<std::vector<RoomTransitionZone>> kLvl14Zones = {
    /* room 0 */ {RoomTransitionZone{1392, 16, 112, 14, 1, 0}},
    /* room 1 */ {RoomTransitionZone{0, 16, 112, 14, 0, 0}, RoomTransitionZone{496, 16, 112, 14, 8, 0}},
    /* room 2 */ {},
    /* room 3 */ {},
    /* room 4 */ {},
    /* room 5 */ {},
    /* room 6 */ {},
    /* room 7 */ {},
    /* room 8 */ {RoomTransitionZone{0, 16, 112, 14, 1, 1}},
    /* room 9 */ {RoomTransitionZone{2032, 0, 64, 14, 10, 0}},
    /* room 10 */ {RoomTransitionZone{0, 48, 192, 14, 9, 0}, RoomTransitionZone{400, 368, 464, 14, 11, 0}},
    /* room 11 */ {RoomTransitionZone{0, 16, 112, 14, 10, 1}},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl14Enemies = {
    /* room 0 */ {RoomEntitySpawnDef{368, 128, 52711, 0, 0, 0}, RoomEntitySpawnDef{880, 16, 52743, 0, 0, 0}, RoomEntitySpawnDef{880, 48, 52743, 0, 0, 0}, RoomEntitySpawnDef{880, 80, 52743, 0, 0, 0}, RoomEntitySpawnDef{880, 128, 52743, 0, 0, 0}, RoomEntitySpawnDef{1008, 16, 52743, 0, 0, 0}, RoomEntitySpawnDef{1008, 48, 52743, 0, 0, 0}, RoomEntitySpawnDef{1008, 80, 52743, 0, 0, 0}, RoomEntitySpawnDef{1008, 128, 52743, 0, 0, 0}, RoomEntitySpawnDef{1136, 16, 52743, 0, 0, 0}, RoomEntitySpawnDef{1136, 48, 52743, 0, 0, 0}, RoomEntitySpawnDef{1136, 80, 52743, 0, 0, 0}, RoomEntitySpawnDef{1136, 128, 52743, 0, 0, 0}},
    /* room 1 */ {RoomEntitySpawnDef{192, 64, 52743, 0, 0, 0}, RoomEntitySpawnDef{112, 560, 52695, 0, 0, 0}, RoomEntitySpawnDef{16, 512, 52695, 0, 0, 0}, RoomEntitySpawnDef{144, 416, 52695, 0, 0, 0}, RoomEntitySpawnDef{144, 336, 52695, 0, 0, 0}},
    /* room 2 */ {RoomEntitySpawnDef{800, 128, 52695, 0, 0, 0}, RoomEntitySpawnDef{752, 48, 52695, 0, 0, 0}, RoomEntitySpawnDef{560, 48, 52695, 0, 0, 0}, RoomEntitySpawnDef{592, 128, 52695, 0, 0, 0}, RoomEntitySpawnDef{368, 48, 52695, 0, 0, 0}, RoomEntitySpawnDef{400, 128, 52695, 0, 0, 0}, RoomEntitySpawnDef{208, 128, 52695, 0, 0, 0}, RoomEntitySpawnDef{128, 96, 52695, 0, 0, 0}, RoomEntitySpawnDef{640, 128, 52695, 0, 0, 0}, RoomEntitySpawnDef{640, 48, 52695, 0, 0, 0}, RoomEntitySpawnDef{448, 48, 52695, 0, 0, 0}, RoomEntitySpawnDef{448, 128, 52695, 0, 0, 0}, RoomEntitySpawnDef{256, 48, 52695, 0, 0, 0}},
    /* room 3 */ {RoomEntitySpawnDef{160, 64, 52727, 0, 0, 0}, RoomEntitySpawnDef{16, 144, 52727, 0, 0, 0}, RoomEntitySpawnDef{208, 144, 52727, 0, 0, 0}, RoomEntitySpawnDef{384, 64, 52727, 0, 0, 0}, RoomEntitySpawnDef{208, 64, 52727, 0, 0, 0}, RoomEntitySpawnDef{384, 144, 52727, 0, 0, 0}, RoomEntitySpawnDef{416, 64, 52727, 0, 0, 0}, RoomEntitySpawnDef{544, 64, 52727, 0, 0, 0}, RoomEntitySpawnDef{640, 64, 52727, 0, 0, 0}, RoomEntitySpawnDef{416, 144, 52727, 0, 0, 0}, RoomEntitySpawnDef{560, 144, 52727, 0, 0, 0}, RoomEntitySpawnDef{640, 144, 52727, 0, 0, 0}},
    /* room 4 */ {RoomEntitySpawnDef{256, 144, 52727, 0, 0, 0}, RoomEntitySpawnDef{336, 128, 52727, 0, 0, 0}, RoomEntitySpawnDef{192, 64, 52727, 0, 0, 0}, RoomEntitySpawnDef{336, 80, 52727, 0, 0, 0}, RoomEntitySpawnDef{96, 80, 52727, 0, 0, 0}, RoomEntitySpawnDef{144, 144, 52727, 0, 0, 0}, RoomEntitySpawnDef{16, 64, 52727, 0, 0, 0}},
    /* room 5 */ {RoomEntitySpawnDef{640, 128, 52727, 0, 0, 0}, RoomEntitySpawnDef{1264, 128, 52727, 0, 0, 0}, RoomEntitySpawnDef{1328, 128, 52727, 0, 0, 0}, RoomEntitySpawnDef{1392, 128, 52727, 0, 0, 0}, RoomEntitySpawnDef{1456, 128, 52727, 0, 0, 0}, RoomEntitySpawnDef{1840, 16, 52743, 0, 0, 0}, RoomEntitySpawnDef{1840, 48, 52743, 0, 0, 0}, RoomEntitySpawnDef{1840, 80, 52743, 0, 0, 0}, RoomEntitySpawnDef{1712, 48, 52743, 0, 0, 0}, RoomEntitySpawnDef{1712, 96, 52743, 0, 0, 0}, RoomEntitySpawnDef{1760, 16, 52743, 0, 0, 0}, RoomEntitySpawnDef{1824, 16, 52743, 0, 0, 0}, RoomEntitySpawnDef{1824, 48, 52743, 0, 0, 0}, RoomEntitySpawnDef{160, 80, 52871, 0, 0, 0}},
    /* room 6 */ {RoomEntitySpawnDef{256, 192, 52743, 0, 0, 0}, RoomEntitySpawnDef{160, 352, 52743, 0, 0, 0}, RoomEntitySpawnDef{192, 416, 52743, 0, 0, 0}, RoomEntitySpawnDef{240, 592, 52743, 0, 0, 0}, RoomEntitySpawnDef{592, 480, 52743, 0, 0, 0}, RoomEntitySpawnDef{320, 496, 52743, 0, 0, 0}, RoomEntitySpawnDef{176, 416, 52743, 0, 0, 0}, RoomEntitySpawnDef{224, 240, 52743, 0, 0, 0}, RoomEntitySpawnDef{464, 240, 52695, 0, 0, 0}, RoomEntitySpawnDef{720, 240, 52695, 0, 0, 0}, RoomEntitySpawnDef{528, 496, 52695, 0, 0, 0}, RoomEntitySpawnDef{624, 496, 52695, 0, 0, 0}, RoomEntitySpawnDef{720, 496, 52695, 0, 0, 0}, RoomEntitySpawnDef{576, 32, 52695, 0, 0, 0}, RoomEntitySpawnDef{912, 464, 52871, 0, 0, 0}, RoomEntitySpawnDef{848, 464, 52871, 0, 0, 0}},
    /* room 7 */ {RoomEntitySpawnDef{200, 128, 52855, 0, 0, 0}, RoomEntitySpawnDef{280, 128, 52855, 0, 0, 0}, RoomEntitySpawnDef{360, 128, 52855, 0, 0, 0}, RoomEntitySpawnDef{440, 128, 52855, 0, 0, 0}, RoomEntitySpawnDef{520, 128, 52855, 0, 0, 0}, RoomEntitySpawnDef{600, 128, 52855, 0, 0, 0}, RoomEntitySpawnDef{680, 128, 52855, 0, 0, 0}, RoomEntitySpawnDef{760, 128, 52855, 0, 0, 0}, RoomEntitySpawnDef{208, 48, 52743, 0, 0, 0}, RoomEntitySpawnDef{336, 48, 52743, 0, 0, 0}, RoomEntitySpawnDef{480, 32, 52743, 0, 0, 0}, RoomEntitySpawnDef{672, 48, 52743, 0, 0, 0}, RoomEntitySpawnDef{816, 32, 52743, 0, 0, 0}, RoomEntitySpawnDef{880, 192, 52871, 0, 0, 0}},
    /* room 8 */ {RoomEntitySpawnDef{688, 112, 52871, 0, 0, 0}, RoomEntitySpawnDef{640, 112, 52871, 0, 0, 0}},
    /* room 9 */ {RoomEntitySpawnDef{160, 128, 52759, 0, 0, 0}, RoomEntitySpawnDef{224, 128, 52759, 0, 0, 0}, RoomEntitySpawnDef{272, 128, 52759, 0, 0, 0}, RoomEntitySpawnDef{320, 128, 52759, 0, 0, 0}, RoomEntitySpawnDef{352, 128, 52759, 0, 0, 0}, RoomEntitySpawnDef{432, 128, 52759, 0, 0, 0}, RoomEntitySpawnDef{480, 128, 52759, 0, 0, 0}, RoomEntitySpawnDef{544, 128, 52775, 0, 0, 0}, RoomEntitySpawnDef{640, 128, 52775, 0, 0, 0}, RoomEntitySpawnDef{656, 128, 52775, 0, 0, 0}, RoomEntitySpawnDef{688, 128, 52775, 0, 0, 0}, RoomEntitySpawnDef{752, 128, 52775, 0, 0, 0}, RoomEntitySpawnDef{928, 128, 52775, 0, 0, 0}, RoomEntitySpawnDef{944, 128, 52775, 0, 0, 0}, RoomEntitySpawnDef{960, 128, 52775, 0, 0, 0}, RoomEntitySpawnDef{1168, 128, 52775, 0, 0, 0}, RoomEntitySpawnDef{1232, 128, 52775, 0, 0, 0}, RoomEntitySpawnDef{1328, 128, 52775, 0, 0, 0}, RoomEntitySpawnDef{1408, 128, 52775, 0, 0, 0}, RoomEntitySpawnDef{1568, 128, 52775, 0, 0, 0}, RoomEntitySpawnDef{1632, 128, 52775, 0, 0, 0}, RoomEntitySpawnDef{1712, 128, 52775, 0, 0, 0}, RoomEntitySpawnDef{1808, 128, 52775, 0, 0, 0}, RoomEntitySpawnDef{432, 32, 52791, 0, 0, 0}, RoomEntitySpawnDef{608, 16, 52791, 0, 0, 0}, RoomEntitySpawnDef{608, 32, 52791, 0, 0, 0}, RoomEntitySpawnDef{608, 48, 52791, 0, 0, 0}, RoomEntitySpawnDef{736, 16, 52791, 0, 0, 0}, RoomEntitySpawnDef{736, 32, 52791, 0, 0, 0}, RoomEntitySpawnDef{752, 112, 52791, 0, 0, 0}, RoomEntitySpawnDef{800, 32, 52791, 0, 0, 0}, RoomEntitySpawnDef{832, 16, 52791, 0, 0, 0}, RoomEntitySpawnDef{1360, 16, 52823, 0, 0, 0}, RoomEntitySpawnDef{1520, 16, 52823, 0, 0, 0}, RoomEntitySpawnDef{1680, 16, 52823, 0, 0, 0}, RoomEntitySpawnDef{1840, 16, 52823, 0, 0, 0}, RoomEntitySpawnDef{1280, 16, 52807, 256, 0, 0}, RoomEntitySpawnDef{1440, 16, 52807, 256, 0, 0}, RoomEntitySpawnDef{1600, 16, 52807, 256, 0, 0}, RoomEntitySpawnDef{1760, 16, 52807, 256, 0, 0}},
    /* room 10 */ {RoomEntitySpawnDef{128, 96, 52871, 0, 0, 0}},
    /* room 11 */ {RoomEntitySpawnDef{336, 48, 52887, 0, 0, 0}, RoomEntitySpawnDef{328, 48, 52839, 256, 0, 0}, RoomEntitySpawnDef{96, 128, 52903, 0, 0, 0}},
};
const std::vector<std::vector<RoomEntitySpawnDef>> kLvl14Specials = {
    /* room 0 */ {RoomEntitySpawnDef{144, 112, 16, 255, 0, 13}},
    /* room 1 */ {RoomEntitySpawnDef{64, 336, 16, 24414, 24414, 14}},
    /* room 2 */ {RoomEntitySpawnDef{944, 80, 16, 24414, 24414, 14}, RoomEntitySpawnDef{1, 0, 0, 688, 48, 16}, RoomEntitySpawnDef{15163, 15420, 14, 3, 0, 0}, RoomEntitySpawnDef{688, 128, 16, 15163, 15420, 14}, RoomEntitySpawnDef{3, 1, 0, 496, 48, 16}, RoomEntitySpawnDef{15163, 15420, 14, 3, 2, 0}, RoomEntitySpawnDef{496, 128, 16, 15163, 15420, 14}, RoomEntitySpawnDef{3, 3, 0, 304, 48, 16}},
    /* room 3 */ {RoomEntitySpawnDef{96, 48, 16, 15163, 15420, 14}, RoomEntitySpawnDef{2, 1, 0, 96, 128, 16}, RoomEntitySpawnDef{15163, 15420, 14, 2, 2, 0}, RoomEntitySpawnDef{288, 48, 16, 15163, 15420, 14}, RoomEntitySpawnDef{2, 3, 0, 288, 128, 16}, RoomEntitySpawnDef{15163, 15420, 14, 2, 4, 0}, RoomEntitySpawnDef{480, 48, 16, 15163, 15420, 14}},
    /* room 4 */ {RoomEntitySpawnDef{32, 128, 16, 16961, 17475, 14}},
    /* room 5 */ {RoomEntitySpawnDef{64, 80, 16, 24414, 24414, 14}, RoomEntitySpawnDef{2, 7, 0, 1952, 80, 16}},
    /* room 6 */ {RoomEntitySpawnDef{48, 48, 16, 25186, 25443, 14}, RoomEntitySpawnDef{5, 1, 0, 864, 464, 16}, RoomEntitySpawnDef{25186, 25443, 65535, 0, 0, 647}},
    /* room 7 */ {RoomEntitySpawnDef{48, 192, 16, 25700, 25957, 14}, RoomEntitySpawnDef{6, 2, 0, 896, 192, 16}, RoomEntitySpawnDef{25700, 25957, 65535, 0, 0, 647}},
    /* room 8 */ {RoomEntitySpawnDef{944, 48, 16, 24672, 24929, 14}, RoomEntitySpawnDef{7, 2, 0, 592, 112, 16}},
    /* room 9 */ {RoomEntitySpawnDef{48, 112, 16, 24672, 24929, 14}},
    /* room 10 */ {},
    /* room 11 */ {},
};

const std::array<LevelRuntimeData, 15> kLevelDataTable = {{
    LevelRuntimeData{
        .level_id = 0,
        .room_count = 8,
        .touch_trigger = {26, 28, kLvl0TouchVals},
        .projectile_trigger = {25, 25, kLvl0ProjVals},
        .mode2_trigger = {140, 140, kLvl0Mode2Vals},
        .transition_zones = kLvl0Zones,
        .active_enemies = kLvl0Enemies,
        .special_entities = kLvl0Specials,
    },
    LevelRuntimeData{
        .level_id = 1,
        .room_count = 9,
        .touch_trigger = {16, 16, kLvl1TouchVals},
        .projectile_trigger = {255, 255, kLvl1ProjVals},
        .mode2_trigger = {255, 255, kLvl1Mode2Vals},
        .transition_zones = kLvl1Zones,
        .active_enemies = kLvl1Enemies,
        .special_entities = kLvl1Specials,
    },
    LevelRuntimeData{
        .level_id = 2,
        .room_count = 4,
        .touch_trigger = {15, 15, kLvl2TouchVals},
        .projectile_trigger = {12, 14, kLvl2ProjVals},
        .mode2_trigger = {57, 58, kLvl2Mode2Vals},
        .transition_zones = kLvl2Zones,
        .active_enemies = kLvl2Enemies,
        .special_entities = kLvl2Specials,
    },
    LevelRuntimeData{
        .level_id = 3,
        .room_count = 2,
        .touch_trigger = {20, 20, kLvl3TouchVals},
        .projectile_trigger = {255, 255, kLvl3ProjVals},
        .mode2_trigger = {255, 255, kLvl3Mode2Vals},
        .transition_zones = kLvl3Zones,
        .active_enemies = kLvl3Enemies,
        .special_entities = kLvl3Specials,
    },
    LevelRuntimeData{
        .level_id = 4,
        .room_count = 3,
        .touch_trigger = {16, 16, kLvl4TouchVals},
        .projectile_trigger = {255, 255, kLvl4ProjVals},
        .mode2_trigger = {255, 255, kLvl4Mode2Vals},
        .transition_zones = kLvl4Zones,
        .active_enemies = kLvl4Enemies,
        .special_entities = kLvl4Specials,
    },
    LevelRuntimeData{
        .level_id = 5,
        .room_count = 4,
        .touch_trigger = {255, 255, kLvl5TouchVals},
        .projectile_trigger = {255, 255, kLvl5ProjVals},
        .mode2_trigger = {255, 255, kLvl5Mode2Vals},
        .transition_zones = kLvl5Zones,
        .active_enemies = kLvl5Enemies,
        .special_entities = kLvl5Specials,
    },
    LevelRuntimeData{
        .level_id = 6,
        .room_count = 4,
        .touch_trigger = {6, 8, kLvl6TouchVals},
        .projectile_trigger = {255, 255, kLvl6ProjVals},
        .mode2_trigger = {255, 255, kLvl6Mode2Vals},
        .transition_zones = kLvl6Zones,
        .active_enemies = kLvl6Enemies,
        .special_entities = kLvl6Specials,
    },
    LevelRuntimeData{
        .level_id = 7,
        .room_count = 3,
        .touch_trigger = {4, 5, kLvl7TouchVals},
        .projectile_trigger = {255, 255, kLvl7ProjVals},
        .mode2_trigger = {255, 255, kLvl7Mode2Vals},
        .transition_zones = kLvl7Zones,
        .active_enemies = kLvl7Enemies,
        .special_entities = kLvl7Specials,
    },
    LevelRuntimeData{
        .level_id = 8,
        .room_count = 4,
        .touch_trigger = {8, 9, kLvl8TouchVals},
        .projectile_trigger = {8, 9, kLvl8ProjVals},
        .mode2_trigger = {13, 15, kLvl8Mode2Vals},
        .transition_zones = kLvl8Zones,
        .active_enemies = kLvl8Enemies,
        .special_entities = kLvl8Specials,
    },
    LevelRuntimeData{
        .level_id = 9,
        .room_count = 4,
        .touch_trigger = {1, 1, kLvl9TouchVals},
        .projectile_trigger = {255, 255, kLvl9ProjVals},
        .mode2_trigger = {255, 255, kLvl9Mode2Vals},
        .transition_zones = kLvl9Zones,
        .active_enemies = kLvl9Enemies,
        .special_entities = kLvl9Specials,
    },
    LevelRuntimeData{
        .level_id = 10,
        .room_count = 9,
        .touch_trigger = {255, 255, kLvl10TouchVals},
        .projectile_trigger = {255, 255, kLvl10ProjVals},
        .mode2_trigger = {255, 255, kLvl10Mode2Vals},
        .transition_zones = kLvl10Zones,
        .active_enemies = kLvl10Enemies,
        .special_entities = kLvl10Specials,
    },
    LevelRuntimeData{
        .level_id = 11,
        .room_count = 8,
        .touch_trigger = {14, 14, kLvl11TouchVals},
        .projectile_trigger = {15, 15, kLvl11ProjVals},
        .mode2_trigger = {255, 255, kLvl11Mode2Vals},
        .transition_zones = kLvl11Zones,
        .active_enemies = kLvl11Enemies,
        .special_entities = kLvl11Specials,
    },
    LevelRuntimeData{
        .level_id = 12,
        .room_count = 7,
        .touch_trigger = {13, 13, kLvl12TouchVals},
        .projectile_trigger = {255, 255, kLvl12ProjVals},
        .mode2_trigger = {255, 255, kLvl12Mode2Vals},
        .transition_zones = kLvl12Zones,
        .active_enemies = kLvl12Enemies,
        .special_entities = kLvl12Specials,
    },
    LevelRuntimeData{
        .level_id = 13,
        .room_count = 2,
        .touch_trigger = {255, 255, kLvl13TouchVals},
        .projectile_trigger = {255, 255, kLvl13ProjVals},
        .mode2_trigger = {255, 255, kLvl13Mode2Vals},
        .transition_zones = kLvl13Zones,
        .active_enemies = kLvl13Enemies,
        .special_entities = kLvl13Specials,
    },
    LevelRuntimeData{
        .level_id = 14,
        .room_count = 12,
        .touch_trigger = {255, 255, kLvl14TouchVals},
        .projectile_trigger = {5, 9, kLvl14ProjVals},
        .mode2_trigger = {123, 123, kLvl14Mode2Vals},
        .transition_zones = kLvl14Zones,
        .active_enemies = kLvl14Enemies,
        .special_entities = kLvl14Specials,
    },
}};

} // namespace

const LevelRuntimeData *get_level_runtime_data(std::uint16_t level_id) {
  if (level_id >= kLevelDataTable.size()) {
    return nullptr;
  }
  return &kLevelDataTable[level_id];
}

std::optional<RoomTransitionZone> check_room_transition_zone(
    std::uint16_t level_id, std::uint16_t room_id,
    std::int16_t player_x, std::int16_t player_y) {
  const auto *lvl = get_level_runtime_data(level_id);
  if (!lvl || room_id >= lvl->transition_zones.size()) {
    return std::nullopt;
  }
  for (const auto &zone : lvl->transition_zones[room_id]) {
    if (player_x == zone.x && player_y >= zone.y_min && player_y <= zone.y_max) {
      return zone;
    }
  }
  return std::nullopt;
}

} // namespace comic2
