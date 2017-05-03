#include "App.h"

#define ProjectDir	"/Samples/MyExample_008_Alpha2Coverage"
#include "InitResDir.inl"


struct DrawVert {
	vec3 pos;
	vec2 texcoord;
};

BaseApp *app = new Alpha2CoverageApp();

bool Alpha2CoverageApp::init()
{
	initWorkDir();
	return true;
}

bool Alpha2CoverageApp::initAPI()
{
	//why i don't open MSAA, i still has a alpha2coverage ?
	// the following is my guess
	//it create the coverage mask table,whether there is a msaa or not.
	//however, it caculate n color samples every pixel when msaa is on and caculate one only sample when it is off.
	//and n samples on coverage mask table in both as long as the Alpha2Coverage is on.So it can still get advantage from the 
	//alpha2coverage but no msaa when msaa is off and alpha2coverage is on.
	return D3D11App::initAPI(D3D11,DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_UNKNOWN, 4, NO_SETTING_CHANGE);
	
}

bool Alpha2CoverageApp::load()
{
	m_tex_ss = renderer->addSamplerState(BILINEAR, WRAP, WRAP, WRAP);
	if((m_tex_id = renderer->addTexture(ResDir("/Textures/fence.dds"),false, m_tex_ss)) == TEXTURE_NONE) return false;
	if ((m_tex_shd = renderer->addShader(ShaderDir("/plain_texture.shd"))) == SHADER_NONE) return false;

	DrawVert quad[] = {
		{vec3(-200,200,0),vec2(0,0)},
		{ vec3(200,200,0),vec2(1,0) },
		{ vec3(200,-200,0),vec2(1,1) },
		{ vec3(-200,-200,0),vec2(0,1) },
	};
	ushort indices[] = {
		0,2,1,
		0,3,2
	};
	m_quad_vb = renderer->addVertexBuffer(sizeof(quad), STATIC, quad);
	m_quad_ib = renderer->addIndexBuffer(sizeof(indices) / sizeof(ushort), sizeof(ushort), STATIC, indices);
	FormatDesc vfdesc[] = {
		{ 0,TYPE_VERTEX,FORMAT_FLOAT,3 },
		{ 0,TYPE_TEXCOORD,FORMAT_FLOAT,2 },
	};
	m_quad_vf = renderer->addVertexFormat(vfdesc, sizeof(vfdesc) / sizeof(FormatDesc), m_tex_shd);

	m_alpha2coverage_not_blend = renderer->addBlendState(ONE, ZERO, BM_ADD, 15, true);
	m_alpha2coverage_blend = renderer->addBlendState(SRC_ALPHA, ONE_MINUS_SRC_ALPHA, BM_ADD, 15, true);
	
	RECT rect;
	GetClientRect(hwnd, &rect);
	m_ratio = float(rect.bottom - rect.top) / float(rect.right - rect.left);
	m_width = float(rect.right - rect.left);

	//init ui
	int tab = configDialog->addTab("Alpha2Coverage");

	configDialog->addWidget(tab, new Label(0, 0, 128, 36, "BlendType"));
	
	BlendType = new DropDownList(0, 50, 360, 30);
	BlendType->addItem("alphaTest");
	BlendType->addItem("alphaBlend");
	BlendType->addItem("alpha2coverage(nb)");
	BlendType->addItem("alpha2coverage(b)");
	BlendType->selectItem(0);
	configDialog->addWidget(tab, BlendType);

	return true;
}

bool Alpha2CoverageApp::onKey(const uint key, const bool pressed)
{
	if (key >= KEY_1 && key <= KEY_4) {
		BlendType->selectItem(key - KEY_1);
	}
	return D3D11App::onKey(key, pressed);
}

void Alpha2CoverageApp::drawFrame()
{
	float col[4] = { 0.5f,0.5f,0.5f,1 };
	renderer->clear(true, true, col, 1.0f);

	vec3 eye(0, 0, -25);
	vec3 lookAt(0, 0, 0);
	vec3 up(0, 1, 0);
	mat4 view = makeViewMatrixD3D(eye, lookAt, up);
	mat4 proj = orthoMatrixX(-m_width * 0.5f, m_width * 0.5f, m_width * 0.5f * m_ratio, -m_width * 0.5f * m_ratio, -200, 200);
	mat4 mvp = proj * view;

	int sel = BlendType->getSelectedItem();
	renderer->reset();
	if(sel == 1)
		renderer->setBlendState(blendSrcAlpha);
	else if(sel == 2)
		renderer->setBlendState(m_alpha2coverage_not_blend);
	else if(sel == 3)
		renderer->setBlendState(m_alpha2coverage_blend);

	renderer->setShader(m_tex_shd);
	renderer->setTexture("image", m_tex_id);
	renderer->setSamplerState("fliter", m_tex_ss);
	renderer->setShaderConstant4x4f("worldViewProj", mvp);
	renderer->setShaderConstant1i("alphaTest", sel == 0 ? 1 : 0);
	renderer->apply();

	renderer->changeVertexFormat(m_quad_vf);
	renderer->changeVertexBuffer(0, m_quad_vb);
	renderer->changeIndexBuffer(m_quad_ib);
	renderer->drawElements(PRIM_TRIANGLES, 0, 6, 0, 4);


}
 