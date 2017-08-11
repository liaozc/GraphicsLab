#include "App.h"
#include <time.h>
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
	
	srand(time);
	float randomAry[10];
	for (int i = 0; i < 10; i+= 2)
	{
		/*
		float r = rand() % 100000;
		r = r / 100000;
		r = r * 2 - 1;
		r = (r + 1) * 2 - 1;
		randomAry[i] = r;
		*/
		randomAry[i] = 0.02f * i;
		randomAry[i + 1] = 0;
	}

	renderer->reset();
	renderer->setRasterizerState(cullNone);
	renderer->setDepthState(noDepthTest);
	renderer->setShader(gs_shd);
	renderer->setShaderConstantArray1f("randomAry", randomAry, 6);
	renderer->apply();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	context->Draw(1, 0);
}