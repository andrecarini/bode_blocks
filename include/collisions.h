#ifndef COLLISIONS_H // To make sure you don't declare the function more than once by including the header multiple times.
#define COLLISIONS_H
#include <cstdlib>

extern int block_position;
extern float g_PositionX;
extern float g_PositionZ;
extern float g_PositionY;
extern float g_sphere_position_x;
extern float g_sphere_position_z;

bool plane_collision();
bool sphere_collision();
bool victory_cube_collision();

#endif