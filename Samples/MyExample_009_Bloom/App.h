#ifndef BLOOM_APP_H
#define BLOOM_APP_H

#include "Direct3D11/D3D11App.h"
class Model;

class BloomApp : public D3D11App
{
public:
	char *getTitle() const { return "post_effect - bloom scene"; }
	bool init();
	void exit();
	bool load();
	void updateFrame();
	void drawFrame();
protected:
	Model* m_sphere;// i use sphere instead of cube to get get the high light.

	mat4 m_world0;
	mat4 m_world1;
	mat4 m_vp;

	VertexBufferID m_ground_vb;
	VertexFormatID m_ground_vf;
	IndexBufferID m_ground_ib;

	TextureID m_col_RT, m_depth_RT;
	TextureID m_test_tex;

	ShaderID m_spec_shd;
	ShaderID m_bloom_shd;

	float m_ratio;
};


#endif