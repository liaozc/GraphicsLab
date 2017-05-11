#include "App.h"
#include "FBX/FBXManager.h"
#include "FBX/FbxModel.h"
#include "Util/Model.h"

#define ProjectDir	"/Samples/MyExample_011_FBX"
#include "InitResDir.inl"


BaseApp *app = new FBXApp();

bool FBXApp::init()
{
	initWorkDir();
	m_camera_dist = 200;
	m_camera_angel = 0;
	return true;
}

void FBXApp::exit()
{
	delete m_model;
	delete m_model_panel;
	FBXSimpleManager::destory();
}

bool FBXApp::load()
{
	m_basic_shd = renderer->addShader(ShaderDir("/basic.shd"));
	m_model = new FbxModel();
	m_model->loadFbx(ResDir("/Models/Fbx/human2.fbx"),0.05f);
	m_model->makeDrawable(renderer, true, m_basic_shd);

	m_model_panel = new FbxModel();
	m_model_panel->loadFbx(ResDir("/Models/Fbx/sadface.FBX"));
	m_model_panel->makeDrawable(renderer, true, m_basic_shd);

	m_test_id = renderer->addTexture(ResDir("/Textures/sadface.jpg"), SS_NONE);

	RECT rect;
	GetClientRect(hwnd, &rect);
	m_ratio = float(rect.bottom - rect.top) / float(rect.right - rect.left);

	return true;
}

bool FBXApp::onKey(const uint key, const bool pressed)
{
	if (D3D11App::onKey(key, pressed))
		return true;
	if (key == KEY_LEFT) {
		m_camera_angel -= 0.1f;
	}
	else if (key == KEY_RIGHT) {
		m_camera_angel += 0.1f;
	}
	else if (key == KEY_UP) {
		m_camera_dist -= 5;
	}
	else if (key == KEY_DOWN) {
		m_camera_dist += 5;
	}
	return true;
}

void FBXApp::updateFrame()
{
	vec3 eye;
	eye.y = m_camera_dist * sinf(wx);
	eye.x = m_camera_dist * cosf(wx) * cosf(wy);
	eye.z = m_camera_dist * cosf(wx) * sinf(wy);
	vec3 up(0, 1, 0);
	if (eye.x == eye.z &&  eye.z == 0)
		up = vec3(0, 0, 1);
	vec3 lookAt(0, 0, 0);
	m_view = makeViewMatrixD3D(eye, lookAt, up);
}


void FBXApp::drawFrame()
{
	updateFrame();
	float col[4] = { 0.5f,0.5f,0.5f,1 };
	renderer->clear(true, true, col, 1.0f);
	
	mat4 proj = perspectiveMatrix(0.15f, m_ratio, 0.1f, 1000);
	mat4 worldViewProj = proj * m_view;

	renderer->reset();
	renderer->setRasterizerState(cullBack);
	renderer->setShader(m_basic_shd);
	renderer->setShaderConstant4x4f("worldViewProj", worldViewProj);
	renderer->setTexture("tex0", m_test_id);
	renderer->setSamplerState("filter0", linearClamp);
	renderer->apply();
	m_model_panel->draw(renderer);
	m_model->draw(renderer);
	
}
