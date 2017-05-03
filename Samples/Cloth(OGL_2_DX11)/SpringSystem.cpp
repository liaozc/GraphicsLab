#include "SpringSystem.h"

SpringSystem::SpringSystem(){
	gravity = vec3(0, -70, 0);
}

void SpringSystem::addRectField(unsigned int width, unsigned int height, void *pos, void *norm, unsigned int stride){
	unsigned int i, j, k;
	char *p = (char *) pos;
	char *n = (char *) norm;

	for (j = 0; j < height; j++){
		for (i = 0; i < width; i++){
			nodes.add(new SNode((vec3 *) p, (vec3 *) n));
			p += stride;
			n += stride;
		}
	}

	static int dx[] = { 1, 1, 0, -1, -1, -1,  0,  1 };
	static int dy[] = { 0, 1, 1,  1,  0, -1, -1, -1 };

	for (j = 0; j < height; j++){
		for (i = 0; i < width; i++){
			SNode *node = nodes[j * width + i];

			unsigned int start, num;
			if (i == 0){
				if (j == 0){
					start = 0;
					num = 3;
				} else if (j == height - 1){
					start = 6;
					num = 3;
				} else {
					start = 6;
					num = 5;
				}
			} else if (i == width - 1){
				if (j == 0){
					start = 2;
					num = 3;
				} else if (j == height - 1){
					start = 4;
					num = 3;
				} else {
					start = 2;
					num = 5;
				}
			} else if (j == 0){
				start = 0;
				num = 5;
			} else if (j == height - 1){
				start = 4;
				num = 5;
			} else {
				start = 0;
				num = 8;
			}

			node->nNormal = num;

			num += start;
			for (k = start; k < num; k++){
				node->addSpring(nodes[(j + dy[k & 7]) * width + i + dx[k & 7]]);				
			}

			if (i < 2){
				if (j < 2){
					start = 0;
					num = 3;
				} else if (j > height - 3){
					start = 6;
					num = 3;
				} else {
					start = 6;
					num = 5;
				}
			} else if (i > width - 3){
				if (j < 2){
					start = 2;
					num = 3;
				} else if (j > height - 3){
					start = 4;
					num = 3;
				} else {
					start = 2;
					num = 5;
				}
			} else if (j < 2){
				start = 0;
				num = 5;
			} else if (j > height - 3){
				start = 4;
				num = 5;
			} else {
				start = 0;
				num = 8;
			}

			num += start;
			for (k = start; k < num; k++){
				node->addSpring(nodes[(j + 2 * dy[k & 7]) * width + i + 2 * dx[k & 7]]);				
			}
		/*
			if (i > 0) node->addSpring(nodes[j * width + i - 1]);
			if (i > 0 && j > 0) node->addSpring(nodes[(j - 1) * width + i - 1]);
			if (j > 0) node->addSpring(nodes[(j - 1) * width + i]);
			if (j > 0 && i < width - 1) node->addSpring(nodes[(j - 1) * width + i + 1]);
			if (i < width - 1) node->addSpring(nodes[j * width + i + 1]);
			if (i < width - 1 && j < height - 1) node->addSpring(nodes[(j + 1) * width + i + 1]);
			if (j < height - 1) node->addSpring(nodes[(j + 1) * width + i]);
			if (j < height - 1 && i > 0) node->addSpring(nodes[(j + 1) * width + i - 1]);
			
			if (i > 1) node->addSpring(nodes[j * width + i - 2]);
			if (i > 1 && j > 1) node->addSpring(nodes[(j - 2) * width + i - 2]);
			if (j > 1) node->addSpring(nodes[(j - 2) * width + i]);
			if (j > 1 && i < width - 2) node->addSpring(nodes[(j - 2) * width + i + 2]);
			if (i < width - 2) node->addSpring(nodes[j * width + i + 2]);
			if (i < width - 2 && j < height - 2) node->addSpring(nodes[(j + 2) * width + i + 2]);
			if (j < height - 2) node->addSpring(nodes[(j + 2) * width + i]);
			if (j < height - 2 && i > 1) node->addSpring(nodes[(j + 2) * width + i - 2]);
			*/
		}
	}
/*	for (j = 0; j < height; j++){
		for (i = 0; i < width; i++){
			if (i > 0) nodes[j * width + i]->addSpring(nodes[j * width + i - 1]);
			if (j > 0) nodes[j * width + i]->addSpring(nodes[(j - 1) * width + i]);
			if (i < width  - 1) nodes[j * width + i]->addSpring(nodes[j * width + i + 1]);
			if (j < height - 1) nodes[j * width + i]->addSpring(nodes[(j + 1) * width + i]);
		}
	}
	// Fix this particular corner that normally gets inverted
	//nodes[width - 1]->springs[1] = nodes[width - 2];
	//nodes[width - 1]->springs[0] = nodes[2 * width - 1];
	*/
}

void SpringSystem::update(float dTime, ProcessNodeFunc process, float *attribs){
	unsigned int i, j;

	for (i = 0; i < nodes.getCount(); i++){
		if (!nodes[i]->locked){
			nodes[i]->dir *= powf(0.5f, dTime);
			for (j = 0; j < nodes[i]->springs.getCount(); j++){
				vec3 d = (*nodes[i]->springs[j].node->pos - *nodes[i]->pos);
				float len = length(d);

				float t = (len - nodes[i]->springs[j].naturalLength) / len;
				
				//float s = sqrtf(fabsf(t) + 1.0f) - 1.0f;
				//t = (t < 0)? -s : s;

				vec3 f = 3 * d * t;//normalize(d) * len;

				nodes[i]->dir += 350 * dTime * f;
			}
			nodes[i]->dir += dTime * gravity;

			if (process != NULL) process(nodes[i], attribs);
		}
	}

	for (i = 0; i < nodes.getCount(); i++){
		if (!nodes[i]->locked){
			*nodes[i]->pos += dTime * nodes[i]->dir;
		}
	}
}

void SpringSystem::evaluateNormals(){
	for (unsigned int i = 0; i < nodes.getCount(); i++){
		SNode *node = nodes[i];

		vec3 normal = vec3(0,0,0);
		vec3 v0, v1 = (*node->springs[0].node->pos - *node->pos);

		for (unsigned int j = 1; j < node->nNormal; j++){
			v0 = v1;
			v1 = (*node->springs[j].node->pos - *node->pos);
			normal += cross(v0, v1) * rsqrtf(dot(v0, v0) * dot(v1, v1));
		}
		*node->normal = fastNormalize(normal);
	}
}
