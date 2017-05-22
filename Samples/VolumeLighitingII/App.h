#ifndef VOLUME_LIGHTTING_II_H
#define VOLUME_LIGHTTING_II_H

#include "Direct3D11/D3D11App.h"

class Model;

struct Light {
	vec4 color;
	vec3 position;
	Light(const vec3 &pos, const vec4 &col) {
		position = pos;
		color = col;
	}
};

class VolumeLighttingApp : public D3D11App
{
public:
	char *getTitle() const { return "Volume Lightting Scene"; }
	void drawFrame();
	bool init();
	void exit();
	bool load();
	void resetCamera();
protected:
	Model* m_model;
	Array <Light> m_lights;
	Array <TextureID> m_texes;
	ShaderID m_volume_shd;
	BlendStateID m_blend_one;
	float m_ratio;
};

#endif