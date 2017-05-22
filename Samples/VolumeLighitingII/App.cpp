#include "App.h"

#include "Util/Model.h"

#define ProjectDir	"/Samples/VolumeLighitingII"
#include "InitResDir.inl"

BaseApp *app = new VolumeLighttingApp();

bool VolumeLighttingApp::init()
{
	m_lights.add(Light(vec3(-800, -120, 190), vec4(0, 0, 1,1)));
	//	lights.add(Light(Vector(-800, -120, -190), Color(1, 0.7f, 0.1f)));
	//	lights.add(Light(Vector(800, -120,  190), Color(1, 0.7f, 0.1f)));
	m_lights.add(Light(vec3(800, -120, -190), vec4(0, 0, 1,1)));
	m_lights.add(Light(vec3(160, -40, 70), vec4(1, 0.7f, 0.1f,1)));
	m_lights.add(Light(vec3(-160, -40, -70), vec4(1, 0.7f, 0.1f,1)));

	return true;
}

void VolumeLighttingApp::exit()
{
	if (m_model)
		delete m_model;
}

bool VolumeLighttingApp::load()
{
	initWorkDir(renderer);

	m_texes.add(renderer->addTexture(ResDir("/Textures/BrownFloor.dds"),true,SS_NONE));
	m_texes.add(renderer->addTexture(ResDir("/Textures/Demwall.dds"), true, SS_NONE));
	m_texes.add(renderer->addTexture(ResDir("/Textures/Demwall2.dds"), true, SS_NONE));
	m_texes.add(renderer->addTexture(ResDir("/Textures/BrownBase.dds"), true, SS_NONE));
	m_texes.add(renderer->addTexture(ResDir("/Textures/Angle1.dds"), true, SS_NONE));
	m_texes.add(renderer->addTexture(ResDir("/Textures/Angle2.dds"), true, SS_NONE));
	m_texes.add(renderer->addTexture(ResDir("/Textures/Demon1.dds"), true, SS_NONE));
	m_texes.add(renderer->addTexture(ResDir("/Textures/Demon2.dds"), true, SS_NONE));
	m_texes.add(renderer->addTexture(ResDir("/Textures/DemWin6.dds"), true, SS_NONE));
	m_texes.add(renderer->addTexture(ResDir("/Textures/Lampon2.dds"), true, SS_NONE));

	m_volume_shd = renderer->addShader(ShaderDir("/volumLight.shd"));
	
	m_model = new Model();
	m_model->loadObj(ResDir("/Models/Map.hml"));
	m_model->scale(0, float3(1, 1, -1));
	m_model->computeNormals();
	m_model->makeDrawable(renderer, true, m_volume_shd);

	m_blend_one = renderer->addBlendState(ONE, ONE);

	RECT rect;
	GetClientRect(hwnd, &rect);
	m_ratio = float(rect.bottom - rect.top) / float(rect.right - rect.left);

	return true;
}

void VolumeLighttingApp::resetCamera()
{
	wx = wy = 0;
	camPos = vec3(-730, 20, 2010);
}


void VolumeLighttingApp::drawFrame()
{
	float col[4] = { 0.5f,0.5f,0.5f,1 };
	renderer->clear(true, true, col, 1.0f);

	const float near_plane = 20.0f;
	const float far_plane = 4000.0f;

	// Reversed depth
	float4x4 projection = toD3DProjection(perspectiveMatrixY(1.2f, width, height, far_plane, near_plane));
	float4x4 view = rotateXY(-wx, -wy);
	view.translate(-camPos);
	float4x4 viewProj = projection * view;


	renderer->reset();
	renderer->setRasterizerState(cullBack);
	renderer->setShader(m_volume_shd);
	renderer->setShaderConstant4x4f("mvp", viewProj);

	renderer->setShaderConstant3f("camPos", camPos);
	for (int i = 0; i < m_lights.getCount(); ++i) {
		for (int j = 0; j < m_model->getBatchCount(); ++j) {
			if (i > 0) renderer->setBlendState(m_blend_one);
			renderer->setTexture("texBase", m_texes[j]);
			renderer->setSamplerState("texFilter", linearClamp);
			vec3 lightPos = m_lights[j].position;
			renderer->setShaderConstant3f("lightPos", lightPos);
			vec3 lPos = lightPos - camPos;
			renderer->setShaderConstant3f("lPos", lPos);
			renderer->setShaderConstant3f("lightColor",m_lights[i].color.xyz());
			renderer->apply();
			m_model->drawBatch(renderer, j);
		}
	}

}