#include "math_helper.h"

#define G_MAX 1000000
#define G_MIN -G_MAX

#define minv(v,v0,v1) \
{\
	v.x = min(v0.x,v1.x);		\
	v.y = min(v0.y,v1.y);		\
	v.z = min(v0.z,v1.z);		\
}\

#define maxv(v,v0,v1) \
{\
	v.x = max(v0.x,v1.x);		\
	v.y = max(v0.y,v1.y);		\
	v.z = max(v0.z,v1.z);		\
}\


AABBox::AABBox()
{
	bMin = vec3(G_MAX, G_MAX, G_MAX);
	bMax = vec3(G_MIN, G_MIN, G_MIN);
}

void AABBox::generateFrom(vec3 * verts, int size)
{
	for (int i = 0; i < size; ++i) {
		minv(bMin, bMin, verts[i]);
		maxv(bMax, bMax, verts[i]);
	}
}

void AABBox::addVert(vec3 pt)
{
	minv(bMin, bMin, pt);
	maxv(bMax, bMax, pt);
}
