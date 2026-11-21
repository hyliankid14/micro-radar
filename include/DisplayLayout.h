#pragma once

#ifdef CROWPANEL_7
constexpr int DISPLAY_WIDTH = 800;
constexpr int DISPLAY_HEIGHT = 480;
constexpr int RADAR_X = 8;
constexpr int RADAR_Y = 8;
constexpr int RADAR_SIZE = 432;
#else
constexpr int DISPLAY_WIDTH = 240;
constexpr int DISPLAY_HEIGHT = 240;
constexpr int RADAR_X = 0;
constexpr int RADAR_Y = 0;
constexpr int RADAR_SIZE = 240;
#endif

constexpr int RADAR_CENTRE_X = RADAR_X + RADAR_SIZE / 2 - 1;
constexpr int RADAR_CENTRE_Y = RADAR_Y + RADAR_SIZE / 2 - 1;
constexpr int RADAR_RADIUS = RADAR_SIZE / 2 - 1;
