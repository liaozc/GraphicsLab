#include "App.h"
#include "Util/Model.h"

#define ProjectDir	"/Samples/MyExample_009_Bloom"
#include "InitResDir.inl"

struct DrawVert {
	vec3 pos;
	vec3 normal;
};

BaseApp *app = new BloomApp();


bool BloomApp::init()
{
	return true;
}

void BloomApp::exit()
{
	delete m_sphere;
}

bool BloomApp::load()
{
	initWorkDir(renderer);

	m_spec_shd = renderer->addShader(ShaderDir("/spec.shd"));
	m_bloom_shd = renderer->addShader(ShaderDir("/bloom.shd"));
	m_sphere = new Model();
	m_sphere->createSphere(3);
	m_sphere->computeNormals(false);
	m_sphere->makeDrawable(renderer, true, m_spec_shd);

	//create the ground
	DrawVert ground[] = {
		{vec3(-20,0,20),vec3(0,1,0) },
		{ vec3(20,0,20),vec3(0,1,0) },
		{ vec3(20,0,-20),vec3(0,1,0) },
		{ vec3(-20,0,-20),vec3(0,1,0) },
	};
	ushort inds[] = {
		0,1,2,
		0,2,3
	};
	
	m_ground_vb = renderer->addVertexBuffer(sizeof(ground), STATIC, ground);
	FormatDesc vfDesc0[] = {
		{0,TYPE_VERTEX,FORMAT_FLOAT,3},
		{ 0,TYPE_NORMAL,FORMAT_FLOAT,3 }
	};
	m_ground_vf = renderer->addVertexFormat(vfDesc0, sizeof(vfDesc0) / sizeof(FormatDesc), m_spec_shd);
	m_ground_ib = renderer->addIndexBuffer(sizeof(inds)/sizeof(ushort), sizeof(ushort), STATIC, inds);

	
	//cache the wnd info
	RECT rect;
	GetClientRect(hwnd, &rect);
	m_ratio = float(rect.bottom - rect.top) / float(rect.right - rect.left);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	m_col_RT = renderer->addRenderTarget(width, height, FORMAT_RGBA8, SS_NONE);
	m_depth_RT = renderer->addRenderDepth(width, height,16);
	
	m_test_tex = renderer->addTexture(ResDir("/Textures/FieldStone.dds"),false,SS_NONE);

	return true;
}

void BloomApp::updateFrame()
{
	m_world0 = translate(0, 1, 0) *  rotateZXY(time,time,time);
	m_world1 = translate(-5, 1, -3) * rotateZXY(time, time, time * 2);
}


void BloomApp::drawFrame()
{
	updateFrame();

	vec3 eye(0, 30, -30);
	vec3 lookAt(0, 0, 0);
	vec3 up(0, 1, 0);
	mat4 view = makeViewMatrixD3D(eye, lookAt, up);
	mat4 proj = perspectiveMatrix(PI / 8, m_ratio, 1, 100);
	mat4 vp = proj * view;
	
	renderer->changeRenderTargets(&m_col_RT, 1, m_depth_RT);
	{
		float col[4] = { 0.5f,0.5f,0.5f,1 };
		renderer->clear(true, true, col, 1.0f);

		renderer->reset();
		renderer->setShader(m_spec_shd);
		renderer->setShaderConstant4x4f("world", m_world0);
		renderer->setShaderConstant4x4f("viewProj", vp);
		renderer->setShaderConstant3f("lightPos", (vec3(5, 10, -5)));
		renderer->setShaderConstant3f("lightCol", vec3(0.8f, 0.8f, 0.8f));
		renderer->setShaderConstant1f("gross", 15.f);
		renderer->setShaderConstant3f("viewPos", eye);
		renderer->setShaderConstant1f("atten", 150);
		renderer->setShaderConstant1f("specdef", 0.8f);
		renderer->apply();
		m_sphere->draw(renderer);

		renderer->setShaderConstant4x4f("world", m_world1);
		renderer->apply();
		m_sphere->draw(renderer);

		renderer->setShaderConstant4x4f("world", identity4());
		renderer->setShaderConstant1f("specdef", 0);
		renderer->setShaderConstant1f("atten", 100);
		renderer->apply();
		renderer->changeVertexFormat(m_ground_vf);
		renderer->changeVertexBuffer(0, m_ground_vb);
		renderer->changeIndexBuffer(m_ground_ib);
		renderer->drawElements(PRIM_TRIANGLES, 0, 6, 0, 4);
	}
	
	renderer->changeToMainFramebuffer();
	renderer->reset();
	renderer->setDepthState(noDepthTest);
	renderer->setShader(m_bloom_shd);
	renderer->setTexture("bloomT", m_col_RT);
	renderer->setSamplerState("Filter", linearClamp);
	renderer->apply();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(3, 0);
}
