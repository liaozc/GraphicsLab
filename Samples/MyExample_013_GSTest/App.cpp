#include "App.h"

#define ProjectDir	"/Samples/MyExample_013_GSTest"
#include "InitResDir.inl"

BaseApp *app = new GSTestApp();


bool GSTestApp::init()
{
	return true;
}

bool GSTestApp::load()
{
	initWorkDir(renderer);
	gs_shd = renderer->addShader(ShaderDir("/gs.shd"));
	return true;
}

void GSTestApp::drawFrame()
{
	float col[4] = { 0.5f,0.5f,0.5f,1 };
	renderer->clear(true, true, col, 1.0f);
	renderer->reset();
	renderer->setShader(gs_shd);
	renderer->apply();

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	context->Draw(3, 0);
}