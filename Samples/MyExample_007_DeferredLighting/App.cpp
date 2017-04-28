#include "App.h"

#define ProjectDir	"/Samples/MyExample_007_DeferredLighting"
#include "InitResDir.inl"


struct DrawVert 
{
	vec3 pos;
	vec3 normal;
	vec3 binormal;
	vec3 tangent;
	vec2 texcoord;
};

BaseApp *app = new DeferredLightingApp();

bool DeferredLightingApp::init()
{
	initWorkDir();
	return true;
}

bool DeferredLightingApp::load()
{

	m_fill_buff_shd = renderer->addShader(ShaderDir("/fill_buffers.shd"));
	m_ambient_shd = renderer->addShader(ShaderDir("/ambient.shd"));

	DrawVert quad[] = {
		{vec3(-20,0,20),vec3(0,1,0),vec3(1,0,0),vec3(0,0,1),vec2(0,0)},
		{ vec3(20,0,20),vec3(0,1,0),vec3(1,0,0),vec3(0,0,1),vec2(1,0) },
		{ vec3(20,0,-20),vec3(0,1,0),vec3(1,0,0),vec3(0,0,1),vec2(1,1) },
		{ vec3(-20,0,-20),vec3(0,1,0),vec3(1,0,0),vec3(0,0,1),vec2(0,1) },
	};
	ushort indices[] = {
		0,1,2,
		0,2,3
	};
	m_ground_vb = renderer->addVertexBuffer(sizeof(quad), STATIC, quad);
	m_ground_ib = renderer->addIndexBuffer(sizeof(indices) / sizeof(ushort), sizeof(ushort), STATIC, indices);
	FormatDesc vfdesc[] = {
		{ 0,TYPE_VERTEX,FORMAT_FLOAT,3 },
		{ 0,TYPE_NORMAL,FORMAT_FLOAT,3 },
		{ 0,TYPE_BINORMAL,FORMAT_FLOAT,3 },
		{ 0,TYPE_TANGENT,FORMAT_FLOAT,3 },
		{ 0,TYPE_TEXCOORD,FORMAT_FLOAT,2 },
	};
	m_ground_vf = renderer->addVertexFormat(vfdesc, sizeof(vfdesc) / sizeof(FormatDesc), m_fill_buff_shd);

	RECT rect;
	GetClientRect(hwnd, &rect);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;
	m_ratio = float(height) / float(width);
	//create a ambient buffer, a specular buffer, a normal buffer
	m_ambient_buff = renderer->addRenderTarget(width, height, FORMAT_RGB10A2, linearClamp);
	m_specular_buff = renderer->addRenderTarget(width, height, FORMAT_R8, linearClamp);
	m_normal_buff = renderer->addRenderTarget(width, height, FORMAT_RGB10A2, linearClamp);
	m_depth_buff = renderer->addRenderDepth(width, height, 1, FORMAT_D16, 1, SS_NONE, SAMPLE_DEPTH);
	
	m_diffuse_id = renderer->addTexture(ResDir("/Textures/FieldStone.dds"), false, SS_NONE);
	m_normal_id = renderer->addTexture(ResDir("/Textures/FieldStoneBump.dds"), false, SS_NONE);
	m_specular_id = renderer->addTexture(ResDir("/Textures/floor_wood_3.dds"), false, SS_NONE);
	//TEST RESOURCE
	m_test_tex = renderer->addTexture(ResDir("/Textures/FieldStone.dds"),false,SS_NONE);

	return true;
}

void DeferredLightingApp::drawFrame()
{

	vec3 eye(0, 55, 0);
	vec3 lookAt(0, 0, 0);
	vec3 up(0, 0, 1);
	mat4 view = makeViewMatrixD3D(eye, lookAt, up);
	mat4 proj = perspectiveMatrix(PI / 4, m_ratio, 1, 100);
	mat4 mvp = proj * view;

	//fill buffer with geometry info
	TextureID buffers[] = { m_ambient_buff,m_specular_buff,m_normal_buff};
	renderer->changeRenderTargets(buffers, 3, m_depth_buff);
	{
		float col[4] = { 0.5f,0.5f,0.5f,1 };
		renderer->clear(true, true, col, 1.0f);
		
		renderer->reset();
		renderer->setShader(m_fill_buff_shd);
		renderer->setShaderConstant4x4f("mvp", mvp);//a world matrix for normal maybe needed.
		renderer->setTexture("diffuse", m_diffuse_id);
		renderer->setTexture("normal", m_normal_id);
		renderer->setTexture("specular", m_specular_id);
		renderer->apply();

		renderer->changeVertexFormat(m_ground_vf);
		renderer->changeVertexBuffer(0, m_ground_vb);
		renderer->changeIndexBuffer(m_ground_ib);
		renderer->drawElements(PRIM_TRIANGLES, 0, 6, 0, 4);
	}

	renderer->changeToMainFramebuffer();
	float col[4] = { 0.5f,0.5f,0.5f,1 };
	renderer->clear(true, true, col, 1.0f);

	renderer->reset();
	renderer->setDepthState(noDepthTest);
	renderer->setShader(m_ambient_shd);
	renderer->setTexture("ambient", m_ambient_buff);
	renderer->setTexture("normal", m_normal_buff);
	renderer->setTexture("specular", m_specular_buff);
	renderer->setTexture("depth", m_depth_buff);
	renderer->setSamplerState("fliter", linearClamp);
	renderer->setShaderConstant3f("camPos", vec3(0, 55, 0));
	renderer->setShaderConstant4x4f("invMvp", !(mvp));
	renderer->setShaderConstant3f("sunLightDir", normalize(vec3(1, 0.6f, 1)));
	renderer->apply();

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(3, 0);

}
