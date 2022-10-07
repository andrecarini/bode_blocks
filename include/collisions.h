#ifndef COLLISIONS_H // To make sure you don't declare the function more than once by including the header multiple times.
#define COLLISIONS_H
#include <cstdlib>

extern int block_position;
extern float g_PositionX;
extern float g_PositionZ;
extern float g_PositionY;

bool death_collision();
bool sphere_colision(float sphere_position_x, float sphere_position_z);


#endif