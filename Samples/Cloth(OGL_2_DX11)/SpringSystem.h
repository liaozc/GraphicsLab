
#ifndef _SPRINGSYSTEM_H_
#define _SPRINGSYSTEM_H_

#include "Util/Array.h"

#include "../Math/Vector.h"

struct SNode;

struct Spring {
	SNode *node;
	float naturalLength;
};

struct SNode {
	SNode(vec3 *position, vec3 *norm){
		pos = position;
		normal = norm;
		dir = vec3(0, 0, 0);
		locked = false;
		nNormal = 0;
	}
	void addSpring(SNode *node){
		Spring spring;

		spring.node = node;
		spring.naturalLength = distance(*pos, *node->pos);

		springs.add(spring);
	}

	vec3 *pos;
	vec3 *normal;
	vec3 dir;

	Array <Spring> springs;
	unsigned int nNormal;

	bool locked;
};

typedef void (*ProcessNodeFunc)(SNode *node, float *attribs);

class SpringSystem {
public:
	SpringSystem();
	~SpringSystem();
	void addRectField(unsigned int width, unsigned int height, void *pos, void *norm, unsigned int stride);

	void update(float dTime, ProcessNodeFunc process = NULL, float *attribs = NULL);
	void evaluateNormals();

	SNode *getNode(unsigned int index) const { return nodes[index]; }
	unsigned int getNodeCount() const { return nodes.getCount(); }

	void setGravity(const vec3 &grav){ gravity = grav; }

protected:
	vec3 gravity;
	Array <SNode *> nodes;
};


#endif // _SPRINGSYSTEM_H_
