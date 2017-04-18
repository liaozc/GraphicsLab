#ifndef _SHADOW_MAP_APP_H
#define _SHADOW_MAP_APP_H

#include "../Framework3/Direct3D11/D3D11App.h"

class Model;

class ShadowMapApp : public D3D11App
{
public:
	char *getTitle() const { return "shadowMap scene"; }
	void drawFrame();
	bool init();
	void exit();
	bool load();
	void updateFrame();
protected:
	Model* m_sphere;
	
	TextureID m_shadow_cubemap_id;
	SamplerStateID m_shadow_map_ssid;
	ShaderID m_shadow_map_gs_shd;

	TextureID m_shadow_map_id;
	ShaderID m_shadow_map_ps_shd;
	ShaderID	m_plain_texture_shd;
	TextureID m_texture_id;

	ShaderID m_plain_normal_shd;
	ShaderID m_light_shdadow_shd;
	
	float4x4 m_world_for_nomal;
	float4x4 m_sphere_mvp;

	float m_ratio;

	ShaderID m_plain_light_shd;
	float4x4 m_light_mvp;
	float4x4 m_light_wrold;
	float m_light_dist;

	//also can be done by using a cube.
	VertexBufferID m_quad_vb;
	IndexBufferID m_quad_ib;
	VertexFormatID m_quad_vf;

	VertexBufferID m_quad_vb_for_showSM;
	VertexFormatID m_quad_vf_for_showSM;

};

#endif
