#ifndef CSM_APP_H
#define CSM_APP_H

#include "CameraApp.h"

class CSMApp : public EmptyWithCameraApp
{
public:
	CSMApp();
	char *getTitle() const { return "cascaded shadow maps scene"; }
	void drawFrame();
	bool init();
	void exit();
	bool load();
protected:
	ShaderID m_camera_shd;
	Model* m_cube;
	Model* m_panel;
	mat4 m_cubeMats[50];

};


#endif