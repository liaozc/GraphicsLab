#include "App.h"
#include "Util/Model.h"

#define ProjectDir	"/Samples/MyExample_003_ShadowMap"
#include "InitResDir.inl"

#include "Util/Model.h"

BaseApp *app = new ShadowMapApp();

struct DrawVert
{
	vec3 pos;
	vec3 normal;
};



void ShadowMapApp::updateFrame()
{
	m_world_for_nomal = rotateY(0); 
	float4x4 world = m_world_for_nomal * scale(2, 2, 2);
	//vec3 eye(10.0f * sinf(time), 1, 10.0f * cosf(time));
	vec3 eye(0, 8, -10);
	vec3 lookAt(0, 0, 0);
	vec3 up(0, 1, 0);
	mat4 view = makeViewMatrixD3D(eye, lookAt, up);
	mat4 proj = perspectiveMatrix(PI / 3, m_ratio, 1, 100);
	mat4 mvp = proj * view * world;
	m_sphere_mvp = toD3DProjection(mvp);
	m_world_for_nomal = toD3DProjection(m_world_for_nomal);
	

	m_light_wrold = rotateY(time) * translate(3,3,0) * scale(0.25f, 0.25f, 0.25f);
	mvp = proj * view * m_light_wrold;
	m_light_mvp = toD3DProjection(mvp);
	
}

void ShadowMapApp::drawFrame()
{
	updateFrame();

	float col[4] = { 0.5f,0.5f,0.5f,1 };
	renderer->clear(true, true, col, 1.0f);

	vec4 vLightPos(m_light_wrold.rows[0].w, m_light_wrold.rows[1].w, m_light_wrold.rows[2].w, 0);
	//vLightPos = normalize(vLightDir);
	vec4 vLightColor(20, 15, 15, 10);
	renderer->reset();
	renderer->setShader(m_plain_normal_shd);
	renderer->setShaderConstant4f("vLightPos", vLightPos);
	renderer->setShaderConstant4f("vLightColor", vLightColor);
	renderer->setShaderConstant4x4f("worldViewProj", m_sphere_mvp);
	renderer->setShaderConstant4x4f("worldMatrix", m_world_for_nomal);
	renderer->apply();
	m_sphere->draw(renderer);
	
	renderer->changeVertexFormat(m_quad_vf);
	renderer->changeVertexBuffer(0, m_quad_vb);
	renderer->changeIndexBuffer(m_quad_ib);
	renderer->drawElements(PRIM_TRIANGLES, 0, 6, 0, 4);
	
	//draw light
	renderer->setShader(m_plain_light_shd);
	renderer->setShaderConstant4f("vLightColor", vLightColor);
	renderer->setShaderConstant4x4f("worldViewProj", m_light_mvp);
	renderer->apply();
	m_sphere->draw(renderer);



}

bool ShadowMapApp::init()
{
	initWorkDir();
	m_sphere = 0;

	return true;
}

void ShadowMapApp::exit()
{
	delete m_sphere;
}

bool ShadowMapApp::load()
{

	m_plain_normal_shd = renderer->addShader(ShaderDir("/plain_normal.shd"));
	m_plain_light_shd = renderer->addShader(ShaderDir("/plain_light.shd"));

	m_sphere = new Model();
	m_sphere->createSphere(3);
	m_sphere->computeNormals(false);
	m_sphere->makeDrawable(renderer, true, m_plain_normal_shd);

	DrawVert verts[] = {
		{ vec3(-10,-2,10),vec3(0,1,0) },
		{ vec3(-10,-2,-10),vec3(0,1,0) },
		{ vec3(10,-2,-10),vec3(0,1,0) },
		{ vec3(10,-2,10),vec3(0,1,0) }
	};
	ushort indices[] = {
		0,3,2,
		0,2,1
	};
	m_quad_vb = renderer->addVertexBuffer(sizeof(verts), STATIC, verts);
	m_quad_ib = renderer->addIndexBuffer(sizeof(indices)/sizeof(ushort), sizeof(ushort), STATIC, indices);
	FormatDesc vfdesc[] = {
		{0,TYPE_VERTEX,FORMAT_FLOAT,3},
		{ 0,TYPE_NORMAL,FORMAT_FLOAT,3}
	};
	m_quad_vf = renderer->addVertexFormat(vfdesc, 2, m_plain_normal_shd);
	
	m_shadow_map_ssid = renderer->addSamplerState(BILINEAR, WRAP, WRAP, WRAP);
	m_shadow_map_id = renderer->addRenderTarget(256, 256, FORMAT_R16F, m_shadow_map_ssid);


	RECT rect;
	GetClientRect(hwnd, &rect);
	m_ratio = float(rect.bottom - rect.top) / float(rect.right - rect.left);

	return true;
}


