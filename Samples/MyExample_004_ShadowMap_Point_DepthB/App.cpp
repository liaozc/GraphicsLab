#include "App.h"
#include "Util/Model.h"

#define ProjectDir	"/Samples/MyExample_004_ShadowMap_Point_DepthB"
#include "InitResDir.inl"

#include "Util/Model.h"

BaseApp *app = new ShadowMapApp_PointDepthB();


struct DrawVert
{
	vec3 pos;
	vec3 normal;
};

struct DrawVert2
{
	vec3 pos;
	vec3 tex;
};


bool ShadowMapApp_PointDepthB::init()
{
	initWorkDir();
	m_sphere = 0;

	return true;
}

void ShadowMapApp_PointDepthB::exit()
{
	delete m_sphere;
}


bool ShadowMapApp_PointDepthB::load()
{

	m_plain_light_shd = renderer->addShader(ShaderDir("/plain_light.shd"));
	m_light_shdadow_shd = renderer->addShader(ShaderDir("/light_shadow.shd"));
	m_plain_texture_shd = renderer->addShader(ShaderDir("/plain_texture.shd"));

	m_sphere = new Model();
	m_sphere->createSphere(3);
	m_sphere->computeNormals(false);
	m_sphere->makeDrawable(renderer, true, m_light_shdadow_shd);

	DrawVert verts[] = {
		{ vec3(-15,-7,15),vec3(0,1,0) },
		{ vec3(-15,-7,-15),vec3(0,1,0) },
		{ vec3(15,-7,-15),vec3(0,1,0) },
		{ vec3(15,-7,15),vec3(0,1,0) }
	};

	ushort indices[] = {
		0,3,2,
		0,2,1
	};


	DrawVert2 verts2[] = {
		{ vec3(-5,5,-5),vec3(-1,1,-1) },
		{ vec3(-5,-5,-5),vec3(-1,-1,-1) },
		{ vec3(5,-5,-5),vec3(1,-1,-1) },
		{ vec3(5,5,-5),vec3(1,1,-1) },

		{ vec3(-5,5,5),vec3(-1,1,1) },
		{ vec3(-5,-5,5),vec3(-1,-1,1) },
		{ vec3(5,-5,5),vec3(1,-1,1) },
		{ vec3(5,5,5),vec3(1,1,1) }
	};

	ushort indices2[] = {
		0,2,1,
		0,3,2,
		4,6,7,
		4,5,6,
		0,7,3,
		0,4,7,
		1,2,6,
		1,6,5,
		0,5,4,
		0,1,5,
		3,7,6,
		3,6,2
	};
	
	m_quad_vb = renderer->addVertexBuffer(sizeof(verts), STATIC, verts);
	m_quad_ib = renderer->addIndexBuffer(sizeof(indices) / sizeof(ushort), sizeof(ushort), STATIC, indices);
	FormatDesc vfdesc[] = {
		{ 0,TYPE_VERTEX,FORMAT_FLOAT,3 },
		{ 0,TYPE_NORMAL,FORMAT_FLOAT,3 }
	};
	m_quad_vf = renderer->addVertexFormat(vfdesc, 2, m_light_shdadow_shd);
	
	m_cube_vb_for_showSM = renderer->addVertexBuffer(sizeof(verts2), STATIC, verts2);
	m_cube_ib_for_showSM = renderer->addIndexBuffer(sizeof(indices2) / sizeof(ushort), sizeof(ushort), STATIC, indices2);
	FormatDesc vfdesc2[] = {
		{ 0,TYPE_VERTEX,FORMAT_FLOAT,3 },
		{ 0,TYPE_TEXCOORD,FORMAT_FLOAT,3 }
	};
	m_cube_vf_for_showSM = renderer->addVertexFormat(vfdesc2, 2, m_plain_texture_shd);

	m_shadow_map_ssid = renderer->addSamplerState(BILINEAR, WRAP, WRAP, WRAP);
	m_shadow_map_id = renderer->addRenderDepth(256, 256, 1, FORMAT_DEPTH16, 1, m_shadow_map_ssid, CUBEMAP | SAMPLE_DEPTH);
	m_shadow_map_gs_shd = renderer->addShader(ShaderDir("/shadow_gs.shd"));
	

	m_triClamp_ssid = renderer->addSamplerState(TRILINEAR, CLAMP, CLAMP, CLAMP);
	const char *fileNames[6];
	std::string tempStr[6];
	tempStr[0] = ResDir("/Textures/CubeMaps/UnionSquare/posx.jpg");
	fileNames[0] = tempStr[0].c_str();
	tempStr[1] = ResDir("/Textures/CubeMaps/UnionSquare/negx.jpg");
	fileNames[1] = tempStr[1].c_str();
	tempStr[2] = ResDir("/Textures/CubeMaps/UnionSquare/posy.jpg");
	fileNames[2] = tempStr[2].c_str();
	tempStr[3] = ResDir("/Textures/CubeMaps/UnionSquare/negy.jpg");
	fileNames[3] = tempStr[3].c_str();
	tempStr[4] = ResDir("/Textures/CubeMaps/UnionSquare/posz.jpg");
	fileNames[4] = tempStr[4].c_str();
	tempStr[5] = ResDir("/Textures/CubeMaps/UnionSquare/negx.jpg");
	fileNames[5] = tempStr[5].c_str();

	m_cube_tex = renderer->addCubemap(fileNames, true, m_triClamp_ssid);


	RECT rect;
	GetClientRect(hwnd, &rect);
	m_ratio = float(rect.bottom - rect.top) / float(rect.right - rect.left);
	m_width = float(rect.right - rect.left);

	return true;
}


void ShadowMapApp_PointDepthB::updateFrame()
{
	m_world_for_nomal = rotateY(0); 
	float4x4 world = m_world_for_nomal * scale(2, 2, 2);
	//vec3 eye(10.0f * sinf(time), 1, 10.0f * cosf(time));
	vec3 eye(0, 25, 0);
	vec3 lookAt(0, 0, 0);
	vec3 up(0, 0, 1);
	mat4 view = makeViewMatrixD3D(eye, lookAt, up);
	mat4 proj = perspectiveMatrix(PI / 2, m_ratio, 1, 100);
	m_sphere_mvp = proj * view * world;

	//m_world_for_nomal = toD3DProjection(m_world_for_nomal);
	
	m_light_wrold = rotateZXY(time, time *0.5f, time * 1.5f) * translate(3,5,0) * scale(0.25f, 0.25f, 0.25f);
	m_light_mvp = proj * view * m_light_wrold;
	
	world = translate(-350, -100, 250) * scale(10, 10, 10) * rotateZXY(0,0,PI);
	proj = orthoMatrixX(-m_width * 0.5f, m_width * 0.5f, m_width * 0.5f * m_ratio, -m_width * 0.5f * m_ratio, -200, 200);
	m_cube_mvp = proj * view * world;

}

void ShadowMapApp_PointDepthB::drawFrame()
{
	updateFrame();

	vec4 vLightPos(m_light_wrold.rows[0].w, m_light_wrold.rows[1].w, m_light_wrold.rows[2].w, 0);
	vec4 vLightColor(1, 1, 1, 1);

	float zNear = 1;
	float zFar = 50;
	//prepare shadow map
	renderer->changeRenderTargets(NULL, 0, m_shadow_map_id);
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

	float col[4] = { 0.5f,0.5f,0.5f,1 };
	renderer->clear(true, true, col, 1.0f);

	float4x4 invViewProj = (!m_sphere_mvp);
	renderer->reset();
	renderer->setRasterizerState(cullBack);

	renderer->setShader(m_light_shdadow_shd);
	renderer->setTexture("cube", m_shadow_map_id);
	renderer->setShaderConstant4f("vLightPos", vLightPos);
	renderer->setShaderConstant4f("vLightColor", vLightColor);
	renderer->setShaderConstant4x4f("worldViewProj", m_sphere_mvp);
	renderer->setShaderConstant4x4f("worldMatrix", m_world_for_nomal);
	renderer->setShaderConstant2f("nf", float2(zFar * zNear / (zNear - zFar), zFar / (zFar - zNear)));
	renderer->setShaderConstant1i("needShadow", 0);
	renderer->apply();
	m_sphere->draw(renderer);


	renderer->setShaderConstant1i("needShadow", 1);
	renderer->apply();

	renderer->changeVertexFormat(m_quad_vf);
	renderer->changeVertexBuffer(0, m_quad_vb);
	renderer->changeIndexBuffer(m_quad_ib);
	renderer->drawElements(PRIM_TRIANGLES, 0, 6, 0, 4);


	renderer->reset();
	renderer->setRasterizerState(cullBack);
	renderer->setShader(m_plain_texture_shd);
	renderer->setTexture("cube", m_shadow_map_id);
	renderer->setShaderConstant4x4f("worldViewProj", m_cube_mvp);
	renderer->apply();
	renderer->changeVertexFormat(m_cube_vf_for_showSM);
	renderer->changeVertexBuffer(0, m_cube_vb_for_showSM);
	renderer->changeIndexBuffer(m_cube_ib_for_showSM);
	renderer->drawElements(PRIM_TRIANGLES, 0, 36, 0, 8);

	//draw light
	renderer->setShader(m_plain_light_shd);
	renderer->setShaderConstant4f("vLightColor", vLightColor);
	renderer->setShaderConstant4x4f("worldViewProj", m_light_mvp);
	renderer->apply();
	m_sphere->draw(renderer);
	
}




