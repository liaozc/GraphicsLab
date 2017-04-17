#ifndef SMALL_EXAMPLE_APP_H
#define SMALL_EXAMPLE_APP_H

#include "../Framework3/Direct3D11/D3D11App.h"

class Model;

class SmallExampleApp : public D3D11App
{
public:
	bool init();
	void exit();
	char *getTitle() const { return "flat triangle scene"; }
	void drawFrame();
	bool onKey(const uint key, const bool pressed);
	bool load();
	void updateTime();
protected:
	int m_example_id;
	float m_ratio;
	//example- plain 3d
	ShaderID m_plain3d_shd;
	VertexBufferID m_plain3d_vb;
	IndexBufferID m_plain3d_ib;
	VertexFormatID m_plain3d_vf;
	float4x4 m_plain3d_mvp;
	//example- transformation
	float4x4 m_plain3d_mvp_asteroid;
	//example - lighting
	Model* m_sphere;
	float4x4 m_sphere_big;
	float4x4 m_sphere_big_world;
	float4x4 m_sphere_small_static;
	float4x4 m_sphere_small_world_static;
	float4x4 m_sphere_small_rotaty;
	float4x4 m_sphere_small_world_rotaty;

	ShaderID m_light_shd;

	//example - texture
	SamplerStateID m_texture_shd;
	SamplerStateID m_texture_sample_id;
	TextureID m_texture_id;
	Model* m_cube;
	float4x4 m_cube_mvp;
	float4x4 m_cube_world;

	RasterizerStateID m_texture_raster_id;

};


#endif