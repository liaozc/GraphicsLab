#include "App.h"

BaseApp *app = new DeferredLightingApp();

void DeferredLightingApp::drawFrame()
{
	float col[4] = { 0.5f,0.5f,0.5f,1 };
	renderer->clear(true, true, col, 1.0f);
}
