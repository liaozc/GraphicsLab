#ifndef DEFERRED_LIGHTING_APP_H
#define DEFERRED_LIGHTING_APP_H

#include "../Framework3/Direct3D11/D3D11App.h"

class Model;

class DeferredLightingApp : public D3D11App
{
public:
	char *getTitle() const { return "deferred lighting scene"; }
	void drawFrame();
	bool init();
	bool load();
protected:
	
	Model* m_cube;
	mat4 m_cube0_mvp;
	mat4 m_cube1_mvp;
	mat4 m_cube2_mvp;

	Model* m_sphere;	//i use this to be a light volume.

	VertexBufferID m_ground_vb;
	IndexBufferID m_ground_ib;
	VertexFormatID m_ground_vf;
	
	TextureID m_ambient_buff, m_specular_buff, m_normal_buff,m_depth_buff;
	
	TextureID m_diffuse_id, m_specular_id, m_normal_id;


	ShaderID m_fill_buff_shd;
	ShaderID m_ambient_shd;
	ShaderID m_light_shd;

	float m_ratio;

	//TEST RESOURCE
	TextureID m_test_tex;

};


#endif