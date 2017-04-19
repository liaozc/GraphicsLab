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
	//for shadow map
	SamplerStateID m_shadow_map_ssid;
	TextureID m_shadow_map_id;
	ShaderID m_shadow_map_ps_shd;
	//for using shadow map	
	ShaderID m_light_shdadow_shd;
	//for 
	float4x4 m_world_for_nomal;
	float4x4 m_sphere_mvp;
	//
	float m_ratio;
	//for draw light
	ShaderID m_plain_light_shd;
	float4x4 m_light_mvp;
	float4x4 m_light_wrold;

	//for the ground also can be done by using a cube. 
	VertexBufferID m_quad_vb;
	IndexBufferID m_quad_ib;
	VertexFormatID m_quad_vf;
	
	//for  show the shadow texture
	VertexBufferID m_quad_vb_for_showSM;
	VertexFormatID m_quad_vf_for_showSM;
	ShaderID	m_plain_texture_shd;
};

#endif
