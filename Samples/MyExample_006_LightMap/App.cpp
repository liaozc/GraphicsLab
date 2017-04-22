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

struct DrawVert2
{
	vec3 pos;
	vec3 tex;
};


BaseApp *app = new LightMapApp();

bool LightMapApp::init()
{
	initWorkDir();
	return true;
}

void LightMapApp::exit()
{
	delete m_cube;
	delete m_sphere;
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
	m_cube0_world_normal = identity4();
	m_cube1_world = translate(-10, 3, -7)* rotateY(0.15f);
	m_cube1_world_normal = rotateY(0.15f);
	m_cube2_world = translate(9, 3, -6)* rotateY(0.65f);
	m_cube2_world_normal = rotateY(0.65f);

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
	m_width = float(rect.right - rect.left);

#if 0
	preComputeLightMap();
#endif
	m_lm0_id = renderer->addTexture(ShaderDir("/LightMap0.dds"), 1, SS_NONE);
	m_lm1_id = renderer->addTexture(ShaderDir("/LightMap1.dds"), 1, SS_NONE);
	m_lm2_id = renderer->addTexture(ShaderDir("/LightMap2.dds"), 1, SS_NONE);
	m_lm_ground_id = renderer->addTexture(ShaderDir("/LightMap_ground.dds"), 1, SS_NONE);
	
	m_sphere = new Model();
	m_sphere->createSphere(2);
	m_sphere->makeDrawable(renderer,true,m_plain_light_shd);
	
	m_light_blend = renderer->addBlendState(ONE, ONE);


	m_shadow_map_ssid = renderer->addSamplerState(LINEAR, CLAMP, CLAMP, CLAMP, 0.0f, 1, LESS);
	m_shadow_map_id = renderer->addRenderDepth(256, 256, 1, FORMAT_D32F, 1, m_shadow_map_ssid, CUBEMAP | SAMPLE_DEPTH);
	m_shadow_map_gs_shd = renderer->addShader(ShaderDir("/shadow_gs.shd"));

	//for show cube map
	m_plain_texture_shd = renderer->addShader(ShaderDir("/plain_texture.shd"));
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
	m_cube_vb_for_showSM = renderer->addVertexBuffer(sizeof(verts2), STATIC, verts2);
	m_cube_ib_for_showSM = renderer->addIndexBuffer(sizeof(indices2) / sizeof(ushort), sizeof(ushort), STATIC, indices2);
	FormatDesc vfdesc2[] = {
		{ 0,TYPE_VERTEX,FORMAT_FLOAT,3 },
		{ 0,TYPE_TEXCOORD,FORMAT_FLOAT,3 }
	};
	m_cube_vf_for_showSM = renderer->addVertexFormat(vfdesc2, 2, m_plain_texture_shd);
	
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


	return true;
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
				m_bsp.pushSphere(samplePos, 0.1f);
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
		//m_bsp.addTriangle(v0, v1, v2);
		//m_bsp.addTriangle(v0, v2, v3);
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

void LightMapApp::updateFrame()
{
	//m_cube0_world = translate(-5, 10, 5)* rotateZXY(0.65f * time, 0.55f * time, 1.25f * time);
	vec3 eye(0, 25, 0);
	vec3 lookAt(0, 0, 0);
	vec3 up(0, 0, 1);
	mat4 view = makeViewMatrixD3D(eye, lookAt, up);
	mat4 world = translate(-400, -100, 250) * scale(10, 10, 10) * rotateZXY(0, 0, PI);
	mat4 proj = orthoMatrixX(-m_width * 0.5f, m_width * 0.5f, m_width * 0.5f * m_ratio, -m_width * 0.5f * m_ratio, -200, 200);
	m_cube_mvp = proj * view * world;


	m_move_light = rotateY(/*PI * 1.18f*/time) * translate(20, 15, 0);
}

void LightMapApp::drawFrame()
{
	updateFrame();

	//generate shadow map first.
	float zNear = 1.f;
	float zFar = 60.0f;
	float3 vLightPos(m_move_light.rows[0].w, m_move_light.rows[1].w, m_move_light.rows[2].w);
	float3 vLightColor(1, 1, 1);
	renderer->changeRenderTargets(NULL,0, m_shadow_map_id);
	{
		renderer->clear(false, true, NULL, 1.0f);
		// Compute view projection matrices for the faces of the cubemap    
		float4x4 viewProjArray[6];
		float4x4 proj = cubeProjectionMatrixD3D(zNear, zFar);
		for (uint k = 0; k < 6; k++) {
			viewProjArray[k] = proj * cubeViewMatrix(k) * translate(-vLightPos);
		}
		renderer->reset();
		renderer->setRasterizerState(cullBack);
		renderer->setShader(m_shadow_map_gs_shd);
		renderer->setShaderConstantArray4x4f("viewProjArray", viewProjArray, 6);
		renderer->setShaderConstant4x4f("worldMatrix", m_cube0_world);
		renderer->apply();
		m_cube->draw(renderer);
		
		renderer->setShaderConstant4x4f("worldMatrix", m_cube1_world);
		renderer->apply();
		m_cube->draw(renderer);

		renderer->setShaderConstant4x4f("worldMatrix", m_cube2_world);
		renderer->apply();
		m_cube->draw(renderer);
	}
	renderer->changeToMainFramebuffer();

	float col[4] = { 0.5f,0.5f,0.5f,1 };
	renderer->clear(true, true, col, 1.0f);

	vec3 eye(0, 200,0);
	vec3 lookAt(0,0,0);
	vec3 up(0, 0, 1);
	mat4 view = makeViewMatrixD3D(eye, lookAt, up);
	mat4 proj = perspectiveMatrix(PI/12, m_ratio, 1, 250);
	mat4 viewProj = proj * view;


	renderer->reset();
	renderer->setShader(m_light_lm_shd);
	renderer->setShaderConstant4x4f("viewProj", viewProj);
	renderer->setTexture("shadowMap", m_shadow_map_id);
	renderer->setSamplerState("shadowMapfliter", m_shadow_map_ssid);;
	renderer->setShaderConstant4f("vMoveLightColor", vec4(vLightColor,1));
	renderer->setShaderConstant4f("vMoveLightPos", vec4(vLightPos,1));
	renderer->setShaderConstant2f("nf", float2(zFar * zNear / (zNear - zFar), zFar / (zFar - zNear)));
	renderer->setShaderConstant1f("denisty", 50);
	
	renderer->setShaderConstant4x4f("worldMatrix", m_cube0_world);
	renderer->setShaderConstant4x4f("worldMatrixNormal", m_cube0_world_normal);
	renderer->setTexture("LightMap", m_lm0_id);
	renderer->apply();
	m_cube->draw(renderer);


	renderer->setShaderConstant4x4f("worldMatrix", m_cube1_world); 
	renderer->setShaderConstant4x4f("worldMatrixNormal", m_cube1_world_normal);
	renderer->setTexture("LightMap", m_lm1_id);
	renderer->apply();
	m_cube->draw(renderer);
	
	renderer->setShaderConstant4x4f("worldMatrix", m_cube2_world);
	renderer->setShaderConstant4x4f("worldMatrixNormal", m_cube2_world_normal);
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
	//draw move light
	mat4 m_light_mvp = viewProj * translate(vLightPos);
	renderer->setShaderConstant4x4f("worldViewProj", m_light_mvp);
	renderer->setShaderConstant4f("vLightColor", vec4(vLightColor, 1));
	renderer->apply();
	m_sphere->draw(renderer);


	//show shadow map
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

}
