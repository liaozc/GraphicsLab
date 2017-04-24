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
	void exit();
	bool load();
	void updateFrame();
	bool onKey(const uint key, const bool pressed);
protected:
	Model* createCube();
	void preComputeLightMap();
	void computFace(const vec3& v0, const vec3& v1, const vec3& v2,const vec2& t0, const vec2& t2,uint8* lm, int width,int height);
	void fillBorder(const vec2& t0, const vec2& t2, uint8* lm, int width,int height);
	void blurLightMap(uint8* lm, int width, int height);
	//the scene objs
	VertexBufferID m_quad_vb;
	IndexBufferID m_quad_ib;
	VertexFormatID m_quad_vf;
	 //-- since i don't know how to assign texture co-coordinate to sphere, i just skip it with cube.
	Model* m_cube;
	float4x4 m_cube0_world;
	float4x4 m_cube0_world_normal;
	float4x4 m_cube1_world;
	float4x4 m_cube1_world_normal;
	float4x4 m_cube2_world;
	float4x4 m_cube2_world_normal;
	
	ShaderID m_light_lm_shd;

	ShaderID m_plain_light_shd;

	Model* m_sphere;
	
	Light m_lights[6];

	mat4 m_move_light;
	//for shadow map
	SamplerStateID m_shadow_map_ssid;
	TextureID m_shadow_map_id;
	ShaderID m_shadow_map_gs_shd;

	uint m_lm_width;
	uint m_lm_height;
	SamplerStateID m_light_map_ssid;
	TextureID m_lm0_id;
	TextureID m_lm1_id;
	TextureID m_lm2_id;
	TextureID m_lm_ground_id;

	TextureID m_ground_normal_id;
	TextureID m_ground_tex_id;
	ShaderID m_ground_shd;
	SamplerStateID m_ground_ssid;

	BSP m_bsp;

	BlendStateID m_light_blend;

	//for  show the shadow texture
	ShaderID	m_plain_texture_shd;
	VertexBufferID m_cube_vb_for_showSM;
	VertexFormatID m_cube_vf_for_showSM;
	IndexBufferID m_cube_ib_for_showSM;
	float4x4 m_cube_mvp;
	TextureID m_cube_tex; //for testing cube texture renderer.
	SamplerStateID m_triClamp_ssid;
	
	float m_ratio;
	float m_width;

	float m_camera_angel;
	float m_camera_dist;
};


#endif