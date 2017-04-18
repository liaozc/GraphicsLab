#include "App.h"
#include "Util/Model.h"

#define ProjectDir	"/Samples/MyExample_003_ShadowMap"
#include "InitResDir.inl"

//#define USING_GEOMETRY_SHADER_AND_DEPTH 

#include "Util/Model.h"

BaseApp *app = new ShadowMapApp();


struct DrawVert
{
	vec3 pos;
	vec3 normal;
};

struct DrawVert2
{
	vec3 pos;
	vec2 tex;
};


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
	m_light_shdadow_shd = renderer->addShader(ShaderDir("/light_shadow.shd"));
	m_plain_texture_shd = renderer->addShader(ShaderDir("/plain_texture.shd"));

	m_sphere = new Model();
	m_sphere->createSphere(3);
	m_sphere->computeNormals(false);
	m_sphere->makeDrawable(renderer, true, m_plain_normal_shd);

	DrawVert verts[] = {
		{ vec3(-10,-5,10),vec3(0,1,0) },
		{ vec3(-10,-5,-10),vec3(0,1,0) },
		{ vec3(10,-5,-10),vec3(0,1,0) },
		{ vec3(10,-5,10),vec3(0,1,0) }
	};

	DrawVert2 verts2[] = {
		{ vec3(-15,-5,10),vec2(0,0) },
		{ vec3(-15,-5,5),vec2(0,1) },
		{ vec3(-10,-5,5),vec2(1,1) },
		{ vec3(-10,-5,10),vec2(1,0) }
	};

	ushort indices[] = {
		0,3,2,
		0,2,1
	};
	m_quad_vb = renderer->addVertexBuffer(sizeof(verts), STATIC, verts);
	m_quad_ib = renderer->addIndexBuffer(sizeof(indices) / sizeof(ushort), sizeof(ushort), STATIC, indices);
	FormatDesc vfdesc[] = {
		{ 0,TYPE_VERTEX,FORMAT_FLOAT,3 },
		{ 0,TYPE_NORMAL,FORMAT_FLOAT,3 }
	};
	m_quad_vf = renderer->addVertexFormat(vfdesc, 2, m_light_shdadow_shd);


	m_quad_vb_for_showSM = renderer->addVertexBuffer(sizeof(verts2), STATIC, verts2);
	FormatDesc vfdesc2[] = {
		{ 0,TYPE_VERTEX,FORMAT_FLOAT,3 },
		{ 0,TYPE_TEXCOORD,FORMAT_FLOAT,2 }
	};
	m_quad_vf_for_showSM = renderer->addVertexFormat(vfdesc2, 2, m_plain_texture_shd);
	

	m_shadow_map_ssid = renderer->addSamplerState(BILINEAR, WRAP, WRAP, WRAP);
	//two shadow map
	m_shadow_cubemap_id = renderer->addRenderTarget(256, 256, FORMAT_R16F, m_shadow_map_ssid,CUBEMAP|SAMPLE_DEPTH);
	m_shadow_map_id = renderer->addRenderTarget(256, 256, FORMAT_R16F, m_shadow_map_ssid);
	m_shadow_map_gs_shd = renderer->addShader(ShaderDir("/shadow_gs.shd"));
	m_shadow_map_ps_shd = renderer->addShader(ShaderDir("/shadow_ps.shd"));
	m_texture_id = renderer->addTexture(ResDir("./Textures/seafloor.dds"), false, m_shadow_map_ssid);

	RECT rect;
	GetClientRect(hwnd, &rect);
	m_ratio = float(rect.bottom - rect.top) / float(rect.right - rect.left);

	return true;
}


void ShadowMapApp::updateFrame()
{
	m_world_for_nomal = rotateY(0); 
	float4x4 world = m_world_for_nomal * scale(2, 2, 2);
	//vec3 eye(10.0f * sinf(time), 1, 10.0f * cosf(time));
	vec3 eye(0, 25, 0);
	vec3 lookAt(0, 0, 0);
	vec3 up(0, 0, 1);
	mat4 view = makeViewMatrixD3D(eye, lookAt, up);
	mat4 proj = perspectiveMatrix(PI / 2, m_ratio, 1, 100);
	mat4 mvp = proj * view * world;
	m_sphere_mvp = mvp;
	//m_world_for_nomal = toD3DProjection(m_world_for_nomal);
	

	m_light_wrold = rotateZXY(time, time *0.5f, time * 1.5f) * translate(4,4,0) * scale(0.25f, 0.25f, 0.25f);
	mvp = proj * view * m_light_wrold;
	m_light_mvp = mvp;
	
}

void ShadowMapApp::drawFrame()
{
	updateFrame();

	vec4 vLightPos(m_light_wrold.rows[0].w, m_light_wrold.rows[1].w, m_light_wrold.rows[2].w, 0);

	float zNear = 0.01f;
	float zFar = 100;
#ifdef USING_GEOMETRY_SHADER_AND_DEPTH
	//prepare shadow map
	renderer->changeRenderTargets(NULL, 0, m_shadow_cubemap_id);
		renderer->clear(false, true, NULL, 1.0f);

		// Compute view projection matrices for the faces of the cubemap
		float4x4 viewProjArray[6];
		float4x4 proj = cubeProjectionMatrixD3D(zNear, zFar);
		for (uint k = 0; k < 6; k++) {
			viewProjArray[k] = proj * cubeViewMatrix(k) * translate(-vLightPos.xyz());
		}

		renderer->reset();
		renderer->setRasterizerState(cullBack);
		renderer->setShader(m_shadow_map_gs_shd);
		renderer->setShaderConstantArray4x4f("viewProjArray", viewProjArray, 6);
		renderer->apply();
		
		m_sphere->draw(renderer);
	renderer->changeToMainFramebuffer();
#else // US PS Method
	renderer->changeRenderTargets(&m_shadow_map_id,1,-1,0);
		float col_depth[] = {1,0,0,1};
		renderer->clear(true, true, col_depth, 1.0f); //renderer to zFar
		vec3 eye = vLightPos.xyz();
		vec3 lookAt(0, 0, 0);
		vec3 up(0, 1, 0);
		mat4 view = makeViewMatrixD3D(eye, lookAt, up);
		mat4 proj = perspectiveMatrix(PI / 5, 1, 1, 10);
		mat4 mvp = proj * view;
		
		renderer->reset();
		renderer->setRasterizerState(cullBack);
		renderer->setShader(m_shadow_map_ps_shd);
		renderer->setShaderConstant4x4f("worldViewProj", mvp);
		renderer->apply();
		m_sphere->draw(renderer);

	renderer->changeToMainFramebuffer();
#endif

	float col[4] = { 0.5f,0.5f,0.5f,1 };
	renderer->clear(true, true, col, 1.0f);

	float4x4 invViewProj = (!m_sphere_mvp);

	vec4 posT = m_sphere_mvp * vec4(1, 1, 1, 1);
	posT /= posT.w;
	posT = invViewProj * posT;
	posT /= posT.w;

	float4x4 identity = invViewProj * m_sphere_mvp;
	
	vec4 vLightColor(1, 1, 1, 1);
	renderer->reset();
	renderer->setRasterizerState(cullBack);

	renderer->setShader(m_light_shdadow_shd);
	renderer->setTexture("shadowMap", m_shadow_map_id);
	renderer->setShaderConstant4f("vLightPos", vLightPos);
	renderer->setShaderConstant4f("vLightColor", vLightColor);
	renderer->setShaderConstant4x4f("worldViewProj", m_sphere_mvp);
	renderer->setShaderConstant4x4f("invViewProj", invViewProj);
	renderer->setShaderConstant4x4f("worldMatrix", m_world_for_nomal);
	renderer->setShaderConstant4x4f("lightMVP", mvp);
	renderer->setShaderConstant1i("needShadow", 1);
	renderer->apply();
	m_sphere->draw(renderer);

	renderer->changeVertexFormat(m_quad_vf);
	renderer->changeVertexBuffer(0, m_quad_vb);
	renderer->changeIndexBuffer(m_quad_ib);
	renderer->drawElements(PRIM_TRIANGLES, 0, 6, 0, 4);


	renderer->reset();
	renderer->setRasterizerState(cullBack);
	renderer->setShader(m_plain_texture_shd);
	renderer->setTexture("tex", m_shadow_map_id);
	renderer->setShaderConstant4x4f("worldViewProj", m_sphere_mvp);
	renderer->apply();
	renderer->changeVertexFormat(m_quad_vf_for_showSM);
	renderer->changeVertexBuffer(0, m_quad_vb_for_showSM);
	renderer->changeIndexBuffer(m_quad_ib);
	renderer->drawElements(PRIM_TRIANGLES, 0, 6, 0, 4);

	//draw light
	renderer->setShader(m_plain_light_shd);
	renderer->setShaderConstant4f("vLightColor", vLightColor);
	renderer->setShaderConstant4x4f("worldViewProj", m_light_mvp);
	renderer->apply();
	m_sphere->draw(renderer);

}




