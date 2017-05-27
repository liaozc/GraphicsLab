#include "App.h"

#define ProjectDir	"/Samples/MyExample_012_CSM"
#include "InitResDir.inl"

#include "Util/Model.h"
#include "Math/Noise.h"

BaseApp *app = new CSMApp();

CSMApp::CSMApp() : EmptyWithCameraApp(DFov,DNear,DFar * 2)
{

}


bool CSMApp::init()
{
	initNoise();
	int xoffset = 0;
	for (int i = 0; i < 50; ++i){
		mat4 r = identity4();
		mat4 s = scale( 0.5f + (rand()%2), 1.2f + (rand() % 4), 1.5 + (rand() % 2));
		mat4 t = translate(xoffset + s.rows[0].x - 110 , s.rows[1].y, 0);
		mat4 m = t * r * s;
		m_cubeMats[i] = m;
		xoffset += s.rows[0].x + 3;
	}
	return true;
}

void CSMApp::exit()
{
	delete m_cube;
	delete m_panel;
}

bool CSMApp::load()
{
	initWorkDir(renderer);
	
	m_camera_shd = renderer->addShader(ShaderDir("/cameraTest.shd"));
	
	m_cube = new Model();
	m_cube->createCube(1, false);
	m_cube->computeNormals(true);
	m_cube->makeDrawable(renderer, true, m_camera_shd);
	
	m_panel = new Model();
	m_panel->createPanel(vec2(300, 20), false);
	m_panel->computeNormals(true);
	m_panel->makeDrawable(renderer, true, m_camera_shd);


	EmptyWithCameraApp::load();

	return true;
}

void CSMApp::drawFrame()
{
	float col[4] = { 0.5f,0.5f,0.5f,1 };
	renderer->clear(true, true, col, 1.0f);

	renderer->reset();
	renderer->setRasterizerState(cullBack);
	renderer->setShader(m_camera_shd);
	renderer->setShaderConstant4x4f("world", identity4());
	for (int i = 0; i < 50; ++i) {
		renderer->setShaderConstant4x4f("worldViewProj", m_cameraProj * m_cameraView * m_cubeMats[i]);
		renderer->apply();
		m_cube->draw(renderer);
	}
	renderer->setShaderConstant4x4f("worldViewProj", m_cameraProj * m_cameraView);
	renderer->apply();
	m_panel->draw(renderer);


}