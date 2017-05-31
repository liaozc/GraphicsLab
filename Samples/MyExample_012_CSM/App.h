#ifndef CSM_APP_H
#define CSM_APP_H

#include "CameraApp.h"

#include "math_helper.h"

enum ShadowType {
	ST_NORMAL,
	ST_CSM_PSSM,
	ST_CSM_SDSM,
};

enum FilterType {
	FT_PCF,
	FT_EVSM,
};

class CSMApp : public EmptyWithCameraApp
{
public:
	CSMApp();
	char *getTitle() const { return "cascaded shadow maps scene"; }
	void drawFrame();
	bool init();
	void exit();
	bool load();
	bool onKey(const uint key, const bool pressed);
	
	void calNormalLightView(vec3 lightDir);
protected:
	ShaderID m_camera_shd;
	//scene desc
	Model* m_cube;
	Model* m_panel;
	mat4 m_cubeMats[50];
	Model* m_obj;
	
	//test line drawing
	AABBox m_aabb;
	RasterizerStateID m_rs_wire;
	VertexBufferID m_aabb_vb;
	IndexBufferID m_aabb_ib;
	VertexFormatID m_aabb_vf;
	ShaderID m_line_shd;
	
	//control light
	mat4 m_light_mat;
	mat4 m_light_view;
	vec3 m_old_eye;
	vec3 m_old_lookAt;
	vec3 m_old_up;
	vec3 m_old_right;

	ShadowType m_shadow_type;
	ShaderID shadow_shd;

	bool mb_show_sm;
	bool mb_use_ligth_as_camera;
	bool mb_use_pcf;
	
	bool mb_use_wire;
	ShaderID m_show_sm_shd;
	mat4 m_show_sm_mat;
	Model* m_show_sm_panel;
	SamplerStateID m_show_sm_ss;

	//for noraml shadow map
	TextureID m_rtd_sm_normal;
	TextureID m_rt_sm_normal;

	SamplerStateID m_ss_sm;
	SamplerStateID m_ss_asio_sm;
	int mi_ss_type;

};


#endif