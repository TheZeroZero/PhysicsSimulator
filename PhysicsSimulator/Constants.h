#pragma once

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;
const int FPS = 60;
const int TICKS_PER_SECOND = SDL_GetPerformanceFrequency();
const int TICKS_PER_FRAME = TICKS_PER_SECOND / FPS;
const int FONT_SIZE = 19;
const int CIRCLES_COUNT = 5;
const double LINE_SCALE_FORCE = 0.0000003;
const double LINE_SCALE_VELOCITY = 0.6;
//const double SCALE = 57913; //how many meters is in one pixel
const double GRAV = 2.0; //gravitational constant ( 1000000000002.0 )
const double RESTITUTION = .8;
const int BALLS_COUNT = 0;