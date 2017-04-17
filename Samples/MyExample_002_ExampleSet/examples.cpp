#include "examples.h"

#define ProjectDir	"/Samples/MyExample_002_ExampleSet"
#include "InitResDir.inl"

#include "Util/Model.h"
//#include "Math/Noise.h"

BaseApp *app = new SmallExampleApp();

struct DrawVert
{
	vec3 pos;
	vec4 color;
};

bool SmallExampleApp::init()
{
	initWorkDir();
	m_example_id = 0;
	
	m_plain3d_shd = SHADER_NONE;
	m_plain3d_vb = VB_NONE;
	m_plain3d_ib = IB_NONE;

	m_light_shd = SHADER_NONE;
		
	m_texture_shd = SHADER_NONE;
	return true;
}

void SmallExampleApp::exit()
{
	delete m_sphere;
	delete m_cube;
}

bool SmallExampleApp::load()
{
	//exmaple 2 and 3
	DrawVert quad[] = {
		{vec3(-1,1,-1),vec4(0.5f,1,1,1)},
		{vec3(-1,-1,-1),vec4(0.5f,0.5f,1,1)},
		{vec3(1,-1,-1),vec4(0.5f,1,0.5f,1)},
		{vec3(1,1,-1), vec4(1, 0.5f, 1, 1)},

		{vec3(-1,1,1),vec4(1, 0.5f, 1, 1)},
		{vec3(-1,-1,1),vec4(1,0, 1, 1)},
		{vec3(1,-1,1), vec4(0, 0.5f, 1, 1)},
		{vec3(1,1,1), vec4(1, 0.5f, 1, 1)},
	};
	ushort indices[] = {
		0,1,2,
		0,2,3,
		4,5,6,
		4,6,7,
		0,4,5,
		0,5,1,
		2,6,7,
		2,7,3,
		0,3,7,
		0,7,4,
		1,2,6,
		1,6,5,
	};
	m_plain3d_vb = renderer->addVertexBuffer(sizeof(quad), STATIC, (void*)quad);
	m_plain3d_ib = renderer->addIndexBuffer(sizeof(indices) / sizeof(ushort), sizeof(ushort), STATIC, indices);
	m_plain3d_shd = renderer->addShader(ShaderDir("/plain3d.shd"));
	FormatDesc format[] = { 
		{0, TYPE_VERTEX, FORMAT_FLOAT, 3 },
		{0, TYPE_TEXCOORD, FORMAT_FLOAT, 4 }
	};
	m_plain3d_vf = renderer->addVertexFormat(format, elementsOf(format), m_plain3d_shd);
	
	//example 4

	m_light_shd = renderer->addShader(ShaderDir("/simple_lit.shd"));
	m_sphere = new Model();
	m_sphere->createSphere(3);
	m_sphere->computeNormals(false);
	m_sphere->makeDrawable(renderer, false, m_light_shd);
	
	RECT rect;
	GetClientRect(hwnd, &rect);
	m_ratio = float(rect.bottom - rect.top) / float(rect.right - rect.left);


	m_texture_shd = renderer->addShader(ShaderDir("/texture.shd"));
	m_texture_sample_id = renderer->addSamplerState(TRILINEAR_ANISO, WRAP, WRAP, WRAP);
	m_texture_id = renderer->addTexture(ResDir("./Textures/seafloor.dds"), m_texture_sample_id);
	m_texture_raster_id = renderer->addRasterizerState(CULL_BACK);

	m_cube = new Model();
	m_cube->createCube(2,true);
	m_cube->computeNormals(false);
	m_cube->makeDrawable(renderer, false, m_texture_shd);

	return true;
}

void SmallExampleApp::updateTime()
{
	D3D11App::updateTime();
	if (m_example_id == 1 || m_example_id == 2) {
		mat4 world = rotateY(time);
		vec3 eye(0, 1, -10);
		vec3 lookAt(0, 0, 0);
		vec3 up(0, 1, 0);
		mat4 view = makeViewMatrixD3D(eye, lookAt, up);
		mat4 proj = perspectiveMatrix(PI / 2, 2, 100);
		proj.rows[0] *= m_ratio;
		mat4 mvp = proj * view * world;
		m_plain3d_mvp = toD3DProjection(mvp);

		if (m_example_id == 2) {
			mat4 world = translate(0, 0, 4);
			world = rotateZXY(time * 2, time * 1.5f, time * 1.6f) * world * scale(0.4f, 0.4f, 0.4f);
			mat4 mvp = proj * view * world;
			m_plain3d_mvp_asteroid = toD3DProjection(mvp);
		}
	}
	
	if (m_example_id == 3 || m_example_id == 4) {
		m_sphere_big_world = rotateY(time) * scale(2,2,2);
		vec3 eye(0, 1, -10);
		vec3 lookAt(0, 0, 0);
		vec3 up(0, 1, 0);
		mat4 view = makeViewMatrixD3D(eye, lookAt, up);
		mat4 proj = perspectiveMatrix(PI / 2, 2, 100);
		proj.rows[0] *= m_ratio;
		mat4 mvp = proj * view * m_sphere_big_world;
		m_sphere_big = toD3DProjection(mvp);
		m_sphere_big_world = toD3DProjection(m_sphere_big_world);
		
		m_sphere_small_world_rotaty = rotateZXY(0,time,1.5f * time) *translate(5, 0, 0) * scale(0.25f, 0.25f, 0.25f);
		mvp = proj * view * m_sphere_small_world_rotaty;
		m_sphere_small_rotaty = toD3DProjection(mvp);

		m_sphere_small_world_static = rotateZXY(time * 0.5f, time * 2.0f, 1.5f * time) * translate(-3, 3, -3) * scale(0.25f, 0.25f, 0.25f);
		mvp = proj * view * m_sphere_small_world_static;
		m_sphere_small_static = toD3DProjection(mvp);

	}

	if (m_example_id == 4) {
	
		m_cube_world = rotateY(time);
		vec3 eye(0, 3, -10);
		vec3 lookAt(0, 0, 0);
		vec3 up(0, 1, 0);
		mat4 view = makeViewMatrixD3D(eye, lookAt, up);
		mat4 proj = perspectiveMatrix(PI / 2, 2, 100);
		proj.rows[0] *= m_ratio;
		mat4 mvp = proj * view * m_cube_world;
		m_cube_world = toD3DProjection(m_cube_world);
		m_cube_mvp = toD3DProjection(mvp);

	}

}

void SmallExampleApp::drawFrame()
{
	float col[4] = { 0.5f,0.5f,0.5f,1 };
	renderer->clear(true, true, col, 1.0f);

	RECT rect;
	GetClientRect(hwnd, &rect);

	updateTime();

	if(m_example_id == 0) {//flat triangle
		vec2 tri[3] = {
		vec2((rect.left + rect.right) * 0.5f,(rect.top * 0.75f + rect.bottom * 0.25f)),
		vec2((rect.left * 0.75f + rect.right * 0.25f) ,(rect.top * 0.25f + rect.bottom * 0.75f)),
		vec2((rect.left * 0.25f + rect.right * 0.75f) ,(rect.top * 0.25f + rect.bottom * 0.75f)),
		};
		vec4 tcol = vec4(1,0.5f, 0.5f,1);
		renderer->setup2DMode((float)rect.left, (float)rect.right, (float)rect.top, (float)rect.bottom);
		renderer->drawPlain(PRIM_TRIANGLES, tri, 3, BS_NONE,DS_NONE,&tcol);
	}
	else if(m_example_id == 1 || m_example_id == 2) {//3D space
		renderer->reset();
		renderer->setShader(m_plain3d_shd);
		renderer->setShaderConstant4x4f("worldViewProj", m_plain3d_mvp);
		renderer->setVertexFormat(m_plain3d_vf);
		renderer->setVertexBuffer(0, m_plain3d_vb, 0);
		renderer->setIndexBuffer(m_plain3d_ib);
		renderer->apply();
		renderer->drawElements(PRIM_TRIANGLES, 0, 36, 0, 8);
		
		if (m_example_id == 2) {
			renderer->setShaderConstant4x4f("worldViewProj", m_plain3d_mvp_asteroid);
			renderer->apply();
			renderer->drawElements(PRIM_TRIANGLES, 0, 36, 0, 8);
		}
		
	}
	if (m_example_id == 3 || m_example_id == 4)	{
		vec4 vLightDir[2] = { 
			normalize(vec4(m_sphere_small_world_static.rows[0].w,m_sphere_small_world_static.rows[1].w,m_sphere_small_world_static.rows[2].w,0)),
			normalize(vec4(m_sphere_small_world_rotaty.rows[0].w,m_sphere_small_world_rotaty.rows[1].w,m_sphere_small_world_rotaty.rows[2].w,0))
		};
		vec4 vLightColor[2] = {
			vec4(1,1,0,1),
			vec4(0,0,1,1)
		};
		renderer->reset();
		if (m_example_id == 3) {
			renderer->setShader(m_light_shd);
			renderer->setShaderConstant1i("lightIndex", 0);
			renderer->setShaderConstantArray4f("vLightDir", vLightDir, 2);
			renderer->setShaderConstantArray4f("vLightColor", vLightColor, 2);
			renderer->setShaderConstant4x4f("worldViewProj", m_sphere_big);
			renderer->setShaderConstant4x4f("worldMatrix", m_sphere_big_world);
			renderer->apply();
			m_sphere->draw(renderer);
		}
		if (m_example_id == 4) {
			renderer->setRasterizerState(m_texture_raster_id);
			renderer->setShader(m_texture_shd);
			renderer->setShaderConstant4x4f("worldViewProj", m_cube_mvp);
			renderer->setShaderConstant4x4f("worldMatrix", m_cube_world);
			renderer->setShaderConstant1i("lightIndex", 0);
			renderer->setShaderConstantArray4f("vLightDir", vLightDir, 2);
			renderer->setShaderConstantArray4f("vLightColor", vLightColor, 2);
			renderer->setTexture("texDiffuse", m_texture_id);
			renderer->apply();
			m_cube->draw(renderer);
		}
		renderer->setShaderConstant1i("lightIndex", 1);
		renderer->setShaderConstant4x4f("worldViewProj", m_sphere_small_static);
		renderer->apply();
		m_sphere->draw(renderer);

		renderer->setShaderConstant1i("lightIndex", 2);
		renderer->setShaderConstant4x4f("worldViewProj", m_sphere_small_rotaty);
		renderer->apply();
		m_sphere->draw(renderer);
	}

}

bool SmallExampleApp::onKey(const uint key, const bool pressed)
{
	if (D3D11App::onKey(key, pressed))
		return true;
	if (key == KEY_1) {
		m_example_id = 0;
	}
	else if (key == KEY_2) {
		m_example_id = 1;
	}
	else if (key == KEY_3) {
		m_example_id = 2;
	}
	else if (key == KEY_4) { 
		m_example_id = 3;
	}
	else if (key == KEY_5) {
		m_example_id = 4;
	}
	else if (key == KEY_6) {
		m_example_id = 5;
	}

	return true;
}

