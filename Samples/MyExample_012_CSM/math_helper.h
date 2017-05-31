#ifndef MATH_HELPER_H
#define MATH_HELPER_H

#include "Math/Vector.h"

struct AABBox
{
	AABBox();
	vec3 bMin;
	vec3 bMax;
	void generateFrom(vec3* verts,int size);
	void addVert(vec3 pt);
};

#endif
