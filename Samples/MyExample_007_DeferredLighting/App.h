#ifndef DEFERRED_LIGHTING_APP_H
#define DEFERRED_LIGHTING_APP_H

#include "../Framework3/Direct3D11/D3D11App.h"

class Model;

class DeferredLightingApp : public D3D11App
{
public:
	char *getTitle() const { return "deferred lighting scene"; }
	void drawFrame();
protected:
	//scene desc,just like the pre example(lightmap),we've got 3 cube and a Big Box ,
	//which contains the cubes.and we has 100 animating lights,which will lit the scene.
	Model* m_cube;

};


#endif