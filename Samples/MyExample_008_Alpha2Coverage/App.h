#ifndef ALPHA_COVERAGE_APP_H
#define ALPHA_COVERAGE_APP_H

#include "Direct3D11/D3D11App.h"

class Alpha2CoverageApp : public D3D11App
{
public:
	char *getTitle() const { return "alpha2coverage scene"; }
	void drawFrame();
	bool init();
	bool initAPI();
	bool load();
	bool onKey(const uint key, const bool pressed);
protected:
	TextureID m_tex_id;
	SamplerStateID m_tex_ss;
	ShaderID m_tex_shd;

	//quad
	VertexBufferID m_quad_vb;
	IndexBufferID m_quad_ib;
	VertexFormatID m_quad_vf;
	
	float m_ratio;
	float m_width;

	BlendStateID m_alpha2coverage_not_blend;
	BlendStateID m_alpha2coverage_blend;

	DropDownList*  BlendType;
};


#endif