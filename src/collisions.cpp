#include "collisions.h"
extern int block_position;
extern float g_PositionX;
extern float g_PositionZ;
extern float g_PositionY;
float sphere_position_x;
float sphere_position_y;
float sphere_position_z;

bool death_collision()
{
    if (block_position == 1) {
        // Bloco está de pé
        if (g_PositionZ == 0 && g_PositionX >= 0 && g_PositionX <= 2) return false;
        if (g_PositionZ == 1 && g_PositionX >= 0 && g_PositionX <= 5) return false;
        if (g_PositionZ == 2 && g_PositionX >= 0 && g_PositionX <= 8) return false;
        if (g_PositionZ == 3 && g_PositionX >= 1 && g_PositionX <= 9) return false;
        if (g_PositionZ == 4 && g_PositionX >= 5 && g_PositionX <= 9) return false;
        if (g_PositionZ == 5 && g_PositionX >= 6 && g_PositionX <= 8) return false;
    } 
    
    if (block_position == 2) {
        // Bloco está deitado paralelo ao eixo Z
        if (g_PositionZ == 0.5 && g_PositionX >= 0 && g_PositionX <= 2) return false;
        if (g_PositionZ == 1.5 && g_PositionX >= 0 && g_PositionX <= 5) return false;
        if (g_PositionZ == 2.5 && g_PositionX >= 1 && g_PositionX <= 8) return false;
        if (g_PositionZ == 3.5 && g_PositionX >= 5 && g_PositionX <= 9) return false;
        if (g_PositionZ == 4.5 && g_PositionX >= 6 && g_PositionX <= 8) return false;
    }
    
    if (block_position == 3) {
        // Bloco está deitado paralelo ao eixo X
        if (g_PositionZ == 0 && g_PositionX >= 0.5 && g_PositionX <= 1.5) return false;
        if (g_PositionZ == 1 && g_PositionX >= 0.5 && g_PositionX <= 4.5) return false;
        if (g_PositionZ == 2 && g_PositionX >= 0.5 && g_PositionX <= 7.5) return false;
        if (g_PositionZ == 3 && g_PositionX >= 1.5 && g_PositionX <= 8.5) return false;
        if (g_PositionZ == 4 && g_PositionX >= 5.5 && g_PositionX <= 8.5) return false;
        if (g_PositionZ == 5 && g_PositionX >= 6.5 && g_PositionX <= 7.5) return false;
    } 
    return true;
}


bool sphere_colision(float sphere_position_x, float sphere_position_z){

    if((abs(sphere_position_x - g_PositionX) <= 1.5) && (abs(sphere_position_z - g_PositionZ)<= 1.5))
        return true;

    else return false;
}


