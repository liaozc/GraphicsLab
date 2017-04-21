#ifndef LIGHT_MAP_APP_H
#define LIGHT_MAP_APP_H

#include "../Framework3/Direct3D11/D3D11App.h"
#include "Util/BSP.h"


struct Light {
	float3 Position;
	float  Intensity;
	float3 Color;
	float  NotUsed;
};


class Model;

class LightMapApp : public D3D11App
{
public:
	char *getTitle() const { return "light-map scene"; }
	void drawFrame();
	bool init();
	bool load();
	void updateFrame();
protected:
	Model* createCube();
	void preComputeLightMap();
	void computFace(const vec3& v0, const vec3& v1, const vec3& v2,const vec2& t0, const vec2& t2,uint8* lm, int width);
	//the scene objs
	VertexBufferID m_quad_vb;
	IndexBufferID m_quad_ib;
	VertexFormatID m_quad_vf;
	 //-- since i don't know how to assign texture co-coordinate to sphere, i just skip it with cube.
	Model* m_cube;
	float4x4 m_cube0_world;
	float4x4 m_cube1_world;
	float4x4 m_cube2_world;
	
	ShaderID m_light_lm_shd;

	ShaderID m_plain_light_shd;

	Model* m_sphere;
	

	Light m_lights[6];


	uint m_lm_width;
	uint m_lm_height;
	TextureID m_lm0_id;
	TextureID m_lm1_id;
	TextureID m_lm2_id;
	TextureID m_lm_ground_id;

	BSP m_bsp;

	BlendStateID m_light_blend;
	
	float m_ratio;
	
};


#endif