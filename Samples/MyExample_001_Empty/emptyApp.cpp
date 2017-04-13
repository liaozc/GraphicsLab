#include "emptyApp.h"

BaseApp *app = new EmptyApp();

void EmptyApp::drawFrame()
{
	float col[4] = { 0.5f,0.5f,0.5f,1 };
	renderer->clear(true, true, col, 1.0f);
}
