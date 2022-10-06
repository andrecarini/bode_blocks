#ifndef COLLISIONS_H // To make sure you don't declare the function more than once by including the header multiple times.
#define COLLISIONS_H

extern int block_position;
extern float g_PositionX;
extern float g_PositionZ;
extern float g_PositionY;

bool death_collision(float x, float y, float z);

#endif