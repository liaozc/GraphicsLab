#ifndef EMPTY_APP_H
#define EMPTY_APP_H

#include "../Framework3/Direct3D11/D3D11App.h"

class GSTestApp : public D3D11App
{
public:
	virtual char *getTitle() const { return "gs test scene"; }
	virtual void drawFrame();
	virtual bool init();
	virtual bool load();
protected:
	ShaderID gs_shd;

};


#endif