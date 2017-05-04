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

class ClothApp : public D3D11App
{
public:
	char *getTitle() const { return "cloth app scene"; }

	bool init();
	void exit();

	bool load();

	void drawLight(const vec3 &lightPos,float size, const vec3& camPos, const vec3& up, const mat4& viewProj);
	void drawSphere(const vec3 &lightPos, const vec3 &spherePos, const float size, const vec3 &color, const mat4& viewProj, const vec3& camPos);
	void drawCloth(const vec3 &lightPos, const mat4& viewProj, const vec3& camPos);

	void drawFrame();

protected:
	SpringSystem spring;
	Model* m_sphere;

	DrawVert m_flag_vertexs[CLOTH_SIZE_Y][CLOTH_SIZE_X];
	
	VertexBufferID m_flag_vb;
	VertexFormatID m_flag_vf;
	IndexBufferID m_flag_ib;

	float nextTime;
	unsigned int nSpheres;
	vec3 m_spherePos[3];
	float m_sphereSize[3];
	vec3 m_sphereColor[3];

	ShaderID m_lighting_shd;
	ShaderID m_sphere_shd;
	ShaderID m_billboard_shd;

	TextureID m_base_id;
	TextureID m_light_id;
	BlendStateID m_light_blend;
	VertexBufferID m_light_vb;
	VertexFormatID m_light_vf;
	IndexBufferID m_light_ib;

	Array <TextureID> m_flags;

	float m_ratio;
};

#endif