#ifndef EMPTY_WITH_CAMERA_APP_H
#define EMPTY_WITH_CAMERA_APP_H

#include "Direct3D11/D3D11App.h"

class Model;

#define DFov  PI/8.0f
#define DNear 0.1f
#define DFar  1000.0f

class EmptyWithCameraApp : public D3D11App
{
public:
	EmptyWithCameraApp(float fov = DFov,float n = DNear, float f = DFar);
public:
	char *getTitle() const { return "empty with camera scene"; }
	void drawFrame();
	bool init();
	void exit();
	bool load();
	void resetCamera();
	bool onMouseMove(const int x, const int y, const int deltaX, const int deltaY);
	bool onMouseButton(const int x, const int y, const MouseButton button, const bool pressed);
	bool onMouseWheel(const int x, const int y, const int scroll);
	bool onKey(const uint key, const bool pressed);
protected:
#ifdef TEST_MODEL
	Model* m_sphere;
	ShaderID m_cameraTest;
#endif
	void updateView();
	mat4 m_cameraView;
	vec3 m_eye;
	vec3 m_lookAt;
	vec3 m_up;
	vec3 m_right;
	
	mat4 m_cameraProj;
	float m_fov;
	float m_near;
	float m_far;

	bool m_leftPress;
	vec2 m_leftPoint;
	
	bool m_midPress;
	vec2 m_midPoint;

	bool m_rightPress;
	vec2 m_rightPoint;

	bool m_shift;


};


#endif