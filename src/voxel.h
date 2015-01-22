#ifndef VOXEL_H
#define VOXEL_H

#include <glm/glm.hpp>

struct VoxelStruct
{
    unsigned int position; // 2 bits unused, 10/10/10 bits pos.x/y/z
    unsigned int color; // 11/11/10
    unsigned int normal;
    unsigned int padding;
};

struct OctreeNodeStruct
{
    unsigned int id;
};

struct OctreeNodeColorStruct
{
    glm::vec4 color;
    glm::vec4 normal;
};

struct BrickStruct
{
	unsigned int radiance_center;
	unsigned int radiance_center_back;
	unsigned int radiance_center_front;
	unsigned int radiance_left;
	unsigned int radiance_left_back;
	unsigned int radiance_left_front;
	unsigned int radiance_right;
	unsigned int radiance_right_back;
	unsigned int radiance_right_front;
	unsigned int radiance_up;
	unsigned int radiance_up_back;
	unsigned int radiance_up_front;
	unsigned int radiance_down;
	unsigned int radiance_down_back;
	unsigned int radiance_down_front;
	unsigned int radiance_left_up;
	unsigned int radiance_left_up_back;
	unsigned int radiance_left_up_front;
	unsigned int radiance_left_down;
	unsigned int radiance_left_down_back;
	unsigned int radiance_left_down_front;
	unsigned int radiance_right_up;
	unsigned int radiance_right_up_back;
	unsigned int radiance_right_up_front;
	unsigned int radiance_right_down;
	unsigned int radiance_right_down_back;
	unsigned int radiance_right_down_front;
};

#endif // VOXEL_H
