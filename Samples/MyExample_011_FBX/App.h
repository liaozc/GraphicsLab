#ifndef FBX_APP_H
#define FBX_APP_H

#include "Direct3D11/D3D11App.h"

class FbxModel;
class Model; //for test

class FBXApp : public D3D11App
{
public:
	char *getTitle() const { return "fbx scene"; }
	void drawFrame();
	bool init();
	void exit();
	bool load();
	bool onKey(const uint key, const bool pressed);
	void updateFrame();
protected:
	FbxModel* m_model;
	FbxModel* m_model_panel;
	ShaderID m_basic_shd;
	
	float m_ratio;
	float m_camera_angel;
	float m_camera_dist;
	vec3 m_camera_pos;
	mat4 m_view;

};


#endif