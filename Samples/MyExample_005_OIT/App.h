#ifndef OIT_APP_H
#define OIT_APP_H

#include "../Framework3/Direct3D11/D3D11App.h"

class OITApp : public D3D11App
{
public:
	char *getTitle() const { return "oit scene"; }
	void drawFrame();
	bool initAPI();
	void exitAPI();
	bool init();
	bool load();
	void updateFrame();
protected:
	TextureID m_unsorted_color_id;
	TextureID m_unsorted_depth_id;
	TextureID m_unsorted_ds_id;

	ShaderID m_stencil_clear_shd;
	ShaderID m_render_shd;
	ShaderID m_sort_shd;
	
	RasterizerStateID m_no_msaa;
	DepthStateID m_stencil_route;
	DepthStateID m_stencil_set;

	float4x4 m_quad0_mvp;
	float4x4 m_quad1_mvp;
	float4x4 m_quad2_mvp;
	float4x4 m_quad3_mvp;

	VertexBufferID m_quad_vb;
	IndexBufferID m_quad_ib;
	VertexFormatID m_quad_vf;
	
	float m_ratio;

	

};


#endif