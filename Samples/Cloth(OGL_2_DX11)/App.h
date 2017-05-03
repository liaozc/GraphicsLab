#ifndef CLOTH_APP_H
#define CLOTH_APP_H

#include "Direct3D11/D3D11App.h"
#include "SpringSystem.h"

#define CLOTH_SIZE_X 54
#define CLOTH_SIZE_Y 42
#define C_SIZE (320.0f / CLOTH_SIZE_X)

#define SPHERE_LEVEL 4
#define POW4(x) (1 << (2 * (x)))
#define SPHERE_SIZE (8 * 3 * POW4(SPHERE_LEVEL))

class Model;

struct DrawVert
{
	vec3 pos;
	vec3 normal;
	vec2 texCoord;
};

class ClothApp : D3D11App
{
public:
	void resetCamera();
	bool init();
	void exit();

	bool load();

	void drawLight(const vec3 &lightPos);
	void drawSphere(const vec3 &lightPos, const vec3 &spherePos, const float size, const vec3 &color);
	void drawCloth(const vec3 &lightPos);

	void drawFrame();

protected:
	SpringSystem spring;
	Model* m_sphere;
	//i should use a vb,but the framework don't surpport to dynamic change it.
	DrawVert m_flag_vertexs[CLOTH_SIZE_Y][CLOTH_SIZE_X];
	unsigned short m_flag_inds[CLOTH_SIZE_X * 2];
	
	float nextTime;
	unsigned int nSpheres;
	vec3 m_spherePos[3];
	float m_sphereSize[3];
	vec3 m_sphereColor[3];

};

#endif