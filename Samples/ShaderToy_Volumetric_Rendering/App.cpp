#include "App.h"

#define ProjectDir	"/Samples/ShaderToy_Volumetric_Rendering"
#include "InitResDir.inl"

BaseApp *app = new VolumetricRenderingApp();

bool VolumetricRenderingApp::init()
{
	return true;
}

bool VolumetricRenderingApp::load()
{
	initWorkDir(renderer);

	m_content_shd = renderer->addShader(ShaderDir("/content.shd"));

	m_iChannel0_id = renderer->addTexture(ResDir("/Textures/noise.png"),SS_NONE);

	RECT rect;
	GetWindowRect(hwnd, &rect);
	m_width = rect.right - rect.left;
	m_height = rect.bottom - rect.top;

	return true;
}

void VolumetricRenderingApp::drawFrame()
{
	float col[4] = { 0.5f,0.5f,0.5f,1 };
	renderer->clear(true, true, col, 1.0f);

	renderer->reset();
	renderer->setDepthState(noDepthTest);
	renderer->setShader(m_content_shd);
	renderer->setShaderConstant2f("iResolution", float2(m_width, m_height));
	renderer->setShaderConstant2f("iGlobalTime", time);
	renderer->setShaderConstant2f("iMouse", float2(wx, wy));
	renderer->setTexture("iChannel0",m_iChannel0_id);
	renderer->apply();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(3, 0);
}

/*
float     iChannelTime[4];       // channel playback time (in seconds)  @not knowing what it is
float3    iChannelResolution[4]; // channel resolution (in pixels)  @not knowing what it is

Texture2D iChannel0;          // input channel. XX = 2D/Cube
Texture2D iChannel1;
Texture2D iChannel2;
Texture2D iChannel3;

float4    iDate;                 // (year, month, day, time in seconds)
float     iSampleRate;           // sound sample rate (i.e., 44100)

*/