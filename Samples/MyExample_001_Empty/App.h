#ifndef EMPTY_APP_H
#define EMPTY_APP_H

#include "../Framework3/Direct3D11/D3D11App.h"

class EmptyApp : public D3D11App
{
public:
	char *getTitle() const { return "empty scene"; }
	void drawFrame();
	bool init();

};


#endif