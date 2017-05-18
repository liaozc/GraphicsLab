#include "App.h"
#include "FBX/FBXManager.h"
#include "FBX/FbxModel.h"
#include "Util/Model.h"

#define ProjectDir	"/Samples/MyExample_011_FBX"
#include "InitResDir.inl"


BaseApp *app = new FBXApp();

bool FBXApp::init()
{
	m_camera_dist = 100;
	m_camera_pos = vec3(0, 0, m_camera_dist);
	m_camera_angel = 0;

	AllocConsole();
	freopen("CONOUT$", "w+t", stdout);

	return true;
}

void FBXApp::exit()
{
	delete m_model;
	delete m_model_panel;
	FBXSimpleManager::destory();

	FreeConsole();
}

bool FBXApp::load()
{
	initWorkDir(renderer);

	m_basic_shd = renderer->addShader(ShaderDir("/basic.shd"));
	m_model = new FbxModel();
	m_model->loadFbx(ResDir("/Models/Fbx/human2_fbx6binary.fbx"));
	m_model->makeDrawable(renderer, true, m_basic_shd);

	m_model_panel = new FbxModel();
	m_model_panel->loadFbx(ResDir("/Models/Fbx/sadface.FBX"));
	m_model_panel->makeDrawable(renderer, true, m_basic_shd);

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
		m_camera_pos += 0.1f * m_view.rows[0].xyz();
	}
	else if (key == KEY_RIGHT) {
		m_camera_pos -= 0.1f * m_view.rows[0].xyz();
	}
	else if (key == KEY_UP) {
		m_camera_pos -= 5 * m_view.rows[2].xyz();
	}
	else if (key == KEY_DOWN) {
		m_camera_pos += 5 * m_view.rows[2].xyz();
	}
	return true;
}

void FBXApp::updateFrame()
{
	float cosX = cosf(wx), sinX = sinf(wx), cosY = cosf(wy), sinY = sinf(wy);
	vec3 dx(cosY, 0, sinY);
	vec3 dy(-sinX * sinY,  cosX, sinX * cosY);
	vec3 dz(-cosX * sinY, -sinX, cosX * cosY);
	mat4 viewRot(vec4(dx,0), vec4(dy, 0), vec4(dz, 0), vec4(0,0,0,1));
	vec3 eye = - m_camera_pos;
	mat4 viewTran(vec4(1,0,0, -eye.x), vec4(0, 1, 0, -eye.y), vec4(0, 0, 1, -eye.z), vec4(0, 0, 0, 1));
	m_view = viewRot * viewTran;
}


void FBXApp::drawFrame()
{
	updateFrame();
	float col[4] = { 0.5f,0.5f,0.5f,1 };
	renderer->clear(true, true, col, 1.0f);
	
	mat4 proj = perspectiveMatrix(0.15f, m_ratio, 0.1f, 1000);
	mat4 worldViewProj = proj * m_view;
/*
	renderer->reset();
	renderer->setRasterizerState(cullBack);
	renderer->setShader(m_basic_shd);
	renderer->setShaderConstant4x4f("worldMatrix", identity4());
	renderer->setShaderConstant4x4f("viewProj", worldViewProj);
	renderer->setTexture("tex0", m_test_id);
	renderer->setSamplerState("filter0", linearClamp);
	renderer->apply();
*/
	renderer->setViewProj(worldViewProj);
	m_model->setWorld(scale(0.1f, 0.1f, 0.1f));
	m_model->draw(renderer);
	m_model_panel->setWorld(rotateX(PI*0.5f));
	m_model_panel->draw(renderer);
	
	
}
