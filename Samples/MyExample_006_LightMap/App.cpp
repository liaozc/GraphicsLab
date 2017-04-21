#include "App.h"
#include "Util/Model.h"
#include "Util/TexturePacker.h"

#define ProjectDir	"/Samples/MyExample_006_LightMap"
#include "InitResDir.inl"

struct DrawVert
{
	vec3 pos;
	vec3 normal;
	vec2 tex;
};

BaseApp *app = new LightMapApp();

bool LightMapApp::init()
{
	initWorkDir();
	return true;
}

bool LightMapApp::load()
{
	m_light_lm_shd = renderer->addShader(ShaderDir("/light_lm.shd"));
	m_plain_light_shd = renderer->addShader(ShaderDir("/plain_light.shd"));

	//create the ground
	DrawVert quad[] = {
		{vec3(-30,0,30),vec3(0,1,0),vec2(0,0)},
		{ vec3(-30,0,-30),vec3(0,1,0),vec2(0,1) },
		{ vec3(30,0,-30),vec3(0,1,0) ,vec2(1,1) },
		{ vec3(30,0,30),vec3(0,1,0) ,vec2(1,0) },
	};
	ushort indices[] = {
		0,2,1,
		0,3,2
	};

	m_quad_vb = renderer->addVertexBuffer(sizeof(quad), STATIC, quad);
	m_quad_ib = renderer->addIndexBuffer(sizeof(indices) / sizeof(ushort), sizeof(ushort), STATIC, indices);
	FormatDesc vfdesc[] = {
		{ 0,TYPE_VERTEX,FORMAT_FLOAT,3 },
		{ 0,TYPE_NORMAL,FORMAT_FLOAT,3 },
		{ 0,TYPE_TEXCOORD,FORMAT_FLOAT,2 },
	};
	m_quad_vf = renderer->addVertexFormat(vfdesc, sizeof(vfdesc)/sizeof(FormatDesc), m_light_lm_shd);

	m_cube = createCube();
	
	m_cube0_world = translate(-5, 3, 5);
	m_cube1_world = translate(-10, 3, -7)* rotateY(0.15f);
	m_cube2_world = translate(9, 3, -6)* rotateY(0.65f);

	float dist = 20;
	m_lights[0].Position = vec3(-dist, 10, dist);
	m_lights[0].Intensity = 50.0f;
	m_lights[0].Color = vec3(1, 0, 0);

	m_lights[1].Position = vec3(dist, 10, dist);
	m_lights[1].Intensity = 50.0f;
	m_lights[1].Color = vec3(1, 1, 0);

	m_lights[2].Position = vec3(dist, 10, -dist);
	m_lights[2].Intensity = 50.0f;
	m_lights[2].Color = vec3(0, 1, 1);

	m_lights[3].Position = vec3(-dist, 10, -dist);
	m_lights[3].Intensity = 50.0f;
	m_lights[3].Color = vec3(1, 0, 1);

	m_lights[4].Position = vec3(0, 5, 0);
	m_lights[4].Intensity = 50.0f;
	m_lights[4].Color = vec3(0.5f, 0.5f, 0.5f);

	m_lights[5].Position = vec3(0, 20, 0);
	m_lights[5].Intensity = 50.0f;
	m_lights[5].Color = vec3(0.8f, 1, 0.8f);

	RECT rect;
	GetClientRect(hwnd, &rect);
	m_ratio = float(rect.bottom - rect.top) / float(rect.right - rect.left);
	
	preComputeLightMap();
	m_lm0_id = renderer->addTexture(ShaderDir("/LightMap0.dds"), 1, SS_NONE);
	m_lm1_id = renderer->addTexture(ShaderDir("/LightMap1.dds"), 1, SS_NONE);
	m_lm2_id = renderer->addTexture(ShaderDir("/LightMap2.dds"), 1, SS_NONE);
	m_lm_ground_id = renderer->addTexture(ShaderDir("/LightMap_ground.dds"), 1, SS_NONE);

	m_sphere = new Model();
	m_sphere->createSphere(2);
	m_sphere->makeDrawable(renderer,true,m_plain_light_shd);
	
	m_light_blend = renderer->addBlendState(ONE, ONE);

	return true;
}

void LightMapApp::updateFrame()
{
	//m_cube0_world = translate(-5, 10, 5)* rotateZXY(0.65f * time, 0.55f * time, 1.25f * time);
}


Model* LightMapApp::createCube()
{
	Model* cube = new Model();
	cube->createCube(3, false);
	cube->computeNormals(true);
	
	uint* inds = cube->getStream(0).indices;
	float3* src = (float3*)cube->getStream(0).vertices;
	uint nIndices = cube->getIndexCount();

	TexturePacker texPacker;
	for (uint i = 0; i < 36; i += 6) {

		float3 v0 = src[inds[i]];
		float3 v1 = src[inds[i + 1]];
		float3 v3 = src[inds[i + 5]];

		float w = length(v1 - v0) * 10;
		float h = length(v3 - v0) * 10;

		if (w < 8) 
			w = 8;
		if (h < 8)
			h = 8;

		uint tw = (int)w;
		uint th = (int)h;

		tw = (tw + 4) & ~7;
		th = (th + 4) & ~7;
		
		texPacker.addRectangle(tw, th);
	}

	m_lm_width = 512;
	m_lm_height = 512;

	if (!texPacker.assignCoords(&m_lm_width, &m_lm_height, widthComp)) {
		ErrorMsg("Lightmap too small");
		return false;
	}

	uint nLmCoords = (nIndices / 6) * 4;
	float2* lmCoords = new float2[nLmCoords];
	uint* lmIndices = new uint[nIndices];

	uint index = 0;
	for (uint i = 0; i < nIndices; i += 6)
	{
		TextureRectangle* rect = texPacker.getRectangle(index);

		float x0 = float(rect->x + 1 + 0.5f) / m_lm_width;
		float y0 = float(rect->y + 1 + 0.5f) / m_lm_height;
		float x1 = float(rect->x + rect->width - 1 - 0.5f) / m_lm_width;
		float y1 = float(rect->y + rect->height - 1 - 0.5f) / m_lm_height;

		lmCoords[4 * index + 0] = float2(x0, y0);
		lmCoords[4 * index + 1] = float2(x1, y0);
		lmCoords[4 * index + 2] = float2(x1, y1);
		lmCoords[4 * index + 3] = float2(x0, y1);

		lmIndices[i + 0] = 4 * index;
		lmIndices[i + 1] = 4 * index + 1;
		lmIndices[i + 2] = 4 * index + 2;
		lmIndices[i + 3] = 4 * index;
		lmIndices[i + 4] = 4 * index + 2;
		lmIndices[i + 5] = 4 * index + 3;

		index++;
	}

	cube->addStream(TYPE_TEXCOORD, 2, nLmCoords, (float *)lmCoords, lmIndices, true);
	cube->makeDrawable(renderer, true,m_light_lm_shd);

	return cube;
}


void LightMapApp::computFace(const vec3& v0, const vec3& v1, const vec3& v2, const vec2& t0, const vec2& t2, uint8* lm,int width)
{
	int left = (int)t0.x;
	int right = (int)t2.x;
	int top = (int)t0.y;
	int bottom = (int)t2.y;
	float divX = 1.0f / (right - left);
	float divY = 1.0f / (bottom - top);
	float3 dirS = v1 - v0;
	float3 dirT = v2 - v1;
	float3 normal = cross(dirS, dirT);

	for (int k = 0; k < 6; ++ k) {
		int lightIndex = k;
		float index = 0;
		float dotVal = dot(normal, m_lights[lightIndex].Position - v0);
		if (dotVal <= 0)
			continue;
		for (int j = top; j < bottom; ++j) {
			for (int i = left; i < right; ++i) {
				float3 samplePos = v0 + (j - top) * dirT * divY + (i - left) * dirS * divX;
				if (m_bsp.intersects(m_lights[lightIndex].Position, samplePos)) {
					continue;
				}
				float3 vDist = samplePos - m_lights[lightIndex].Position;
				float dist = dot(vDist, vDist);
				float sample = m_lights[lightIndex].Intensity / dist;
				sample = sample > 1 ? 1 : sample;
				float3 col = (sample * 255.0f) * m_lights[lightIndex].Color + float3(lm[3 * (j * width + i)], lm[3 * (j * width + i) + 1], lm[3 * (j * width + i) + 2]);
				lm[3 * (j * width + i)] = (uint8)(col.x > 255 ? 255 : col.x);
				lm[3 * (j * width + i) + 1] = (uint8)(col.y > 255 ? 255 : col.y);
				lm[3 * (j * width + i) + 2] = (uint8)(col.z > 255 ? 255 : col.z);
			}
		}
	}
}


void LightMapApp::preComputeLightMap()
{

	float3* verts = (float3*)m_cube->getStream(0).vertices;
	uint*  indices = m_cube->getStream(0).indices;
	float2* texcoords = (float2*)m_cube->getStream(2).vertices;
	uint* texIndices = m_cube->getStream(2).indices;

	int index = 0;
	{
		//create BSP tree to detect insect infomation
		for (int k = 0; k < 3; k++) {
			index = 0;
			mat4 world = k == 0 ? m_cube0_world : k == 1 ? m_cube1_world : m_cube2_world;
			for (int i = 0; i < 6; ++i) {
				float3 v0 = verts[indices[index]];
				float3 v1 = verts[indices[index + 1]];
				float3 v2 = verts[indices[index + 2]];
				float3 v3 = verts[indices[index + 5]];
				v0 = (world * vec4(v0, 1)).xyz();
				v1 = (world * vec4(v1, 1)).xyz();
				v2 = (world * vec4(v2, 1)).xyz();
				v3 = (world * vec4(v3, 1)).xyz();
				index += 6;
				m_bsp.addTriangle(v0, v1, v2);
				m_bsp.addTriangle(v0, v2, v3);
			}
		}
		float3 v0 = vec3(-30, 0, 30);
		float3 v1 = vec3(30, 0, 30);
		float3 v2 = vec3(30, 0, -30);
		float3 v3 = vec3(-30, 0, -30);
		m_bsp.addTriangle(v0, v1, v2);
		m_bsp.addTriangle(v0, v2, v3);
	}
	m_bsp.build();

	uint8* lm = new uint8[m_lm_width* m_lm_height * 3];
	memset(lm, 0, sizeof(uint8) * m_lm_width* m_lm_height * 3);
	float3 v0, v1,v2;
	float2 t0, t2;
	for (int k = 0; k < 3; k++) {
		memset(lm, 0, sizeof(uint8) * m_lm_width* m_lm_height * 3);
		index = 0;
		mat4 world = k == 0 ? m_cube0_world : k == 1 ? m_cube1_world : m_cube2_world;
		for (int i = 0; i < 6; ++i) {
			v0 = verts[indices[index]];
			v1 = verts[indices[index + 1]];
			v2 = verts[indices[index + 2]];
			v0 = (world * vec4(v0, 1)).xyz();
			v1 = (world * vec4(v1, 1)).xyz();
			v2 = (world * vec4(v2, 1)).xyz();
			t0 = texcoords[texIndices[index]] * float2(m_lm_width, m_lm_height) + float2(-1, -1);
			t2 = texcoords[texIndices[index + 2]] * float2(m_lm_width, m_lm_height) + float2(1, 1);
			computFace(v0, v1, v2, t0, t2, lm,m_lm_width);
			index += 6;
		}
		Image image;
		image.loadFromMemory(lm, FORMAT_RGB8, m_lm_width, m_lm_height, 1, 1, false);
		char fileName[256];
		sprintf(fileName, ShaderDir("/LightMap%d.dds"), k);
		image.saveImage(fileName);
	}

	delete[] lm;
	lm = nullptr;

	uint8* lm2 = new uint8[512 * 512 * 3];
	memset(lm2, 0, sizeof(uint8) * 512 * 512 * 3);
	//create light map of ground
	v0 = vec3(-30, 0, 30);
	v1 = vec3(30, 0, 30);
	v2 = vec3(30, 0, -30);
	t0 = float2(0,0);
	t2 = float2(512, 512);
	computFace(v0, v1, v2, t0, t2, lm2,512);
	Image image;
	image.loadFromMemory(lm2, FORMAT_RGB8, 512, 512, 1, 1, true);
	char fileName[256];
	sprintf(fileName, ShaderDir("/LightMap_ground.dds"));
	image.saveImage(fileName);
}


void LightMapApp::drawFrame()
{
	updateFrame();

	float col[4] = { 0.5f,0.5f,0.5f,1 };
	renderer->clear(true, true, col, 1.0f);

	vec3 eye(0, 180, -120);
	vec3 lookAt(0,0,0);
	vec3 up(0, 0, 1);
	mat4 view = makeViewMatrixD3D(eye, lookAt, up);
	mat4 proj = perspectiveMatrix(PI/12, m_ratio, 1, 250);
	mat4 viewProj = proj * view;
	
	renderer->reset();
	renderer->setShader(m_light_lm_shd);
	renderer->setShaderConstant4x4f("viewProj", viewProj);
	renderer->setShaderConstant4x4f("worldMatrix", m_cube0_world);
	renderer->setShaderConstantArray4f("Lights", (const float4 *)m_lights, 2 * (sizeof(Light) / 16));
	renderer->setTexture("LightMap", m_lm0_id);
	renderer->apply();
	m_cube->draw(renderer);


	renderer->setShaderConstant4x4f("worldMatrix", m_cube1_world); 
	renderer->setTexture("LightMap", m_lm1_id);
	renderer->apply();
	m_cube->draw(renderer);
	
	renderer->setShaderConstant4x4f("worldMatrix", m_cube2_world);
	renderer->setTexture("LightMap", m_lm2_id);
	renderer->apply();
	m_cube->draw(renderer);
	
	renderer->setShaderConstant4x4f("worldMatrix", identity4());
	renderer->setTexture("LightMap", m_lm_ground_id);
	renderer->apply();
	renderer->changeVertexFormat(m_quad_vf);
	renderer->changeVertexBuffer(0, m_quad_vb);
	renderer->changeIndexBuffer(m_quad_ib);
	renderer->drawElements(PRIM_TRIANGLES, 0, 6, 0, 4);

	renderer->reset();
	renderer->setBlendState(m_light_blend);
	renderer->setShader(m_plain_light_shd);
	for (int k = 0; k < 6; ++k) {
		mat4 m_light_mvp = viewProj * translate(m_lights[k].Position);
		renderer->setShaderConstant4x4f("worldViewProj", m_light_mvp);
		renderer->setShaderConstant4f("vLightColor", vec4(m_lights[k].Color, 1));
		renderer->apply();
		m_sphere->draw(renderer);
	}
	
}
