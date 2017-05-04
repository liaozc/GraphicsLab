#include "App.h"
#include "Util/Model.h"
#include "Util/TexturePacker.h"

#define ProjectDir	"/Samples/MyExample_010_VolumeLightting"
#include "InitResDir.inl"

struct DrawVert
{
	vec3 pos;
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
	vec2 tex;
};

struct DrawVert2
{
	vec3 pos;
	vec3 tex;
};


BaseApp *app = new VolumeLightingApp();

bool VolumeLightingApp::init()
{
	initWorkDir();
	m_camera_angel = 45;
	m_camera_dist = 350;
	return true;
}

void VolumeLightingApp::exit()
{
	delete m_cube;
	delete m_sphere;
}

bool VolumeLightingApp::load()
{
	m_light_lm_shd = renderer->addShader(ShaderDir("/light_lm_sm.shd"));
	m_plain_light_shd = renderer->addShader(ShaderDir("/plain_light.shd"));
	m_ground_shd = renderer->addShader(ShaderDir("/light_lm_sm_tex_normal.shd"));
	m_full_screen_shd = renderer->addShader(ShaderDir("/full_screen_texture.shd"));
	//create the ground
	DrawVert quad[] = {
		{vec3(-100,0,100),vec3(0,1,0),vec3(0,0,1),vec3(1,0,0),vec2(0,0)},
		{ vec3(-100,0,-100),vec3(0,1,0),vec3(0,0,1),vec3(1,0,0),vec2(0,1) },
		{ vec3(100,0,-100),vec3(0,1,0) ,vec3(0,0,1),vec3(1,0,0),vec2(1,1) },
		{ vec3(100,0,100),vec3(0,1,0) ,vec3(0,0,1),vec3(1,0,0),vec2(1,0) },
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
		{ 0,TYPE_TANGENT,FORMAT_FLOAT,3 },
		{ 0,TYPE_BINORMAL,FORMAT_FLOAT,3 },
		{ 0,TYPE_TEXCOORD,FORMAT_FLOAT,2 },
	};
	m_quad_vf = renderer->addVertexFormat(vfdesc, sizeof(vfdesc)/sizeof(FormatDesc), m_ground_shd);

	m_cube = createCube();
	
	m_cube0_world = translate(-30, 24, 0) * scale(4,8,4);
	m_cube0_world_normal = identity4();
	m_cube1_world = translate(30, 24, 0) * scale(4, 8, 4);
	m_cube1_world_normal = rotateY(0.15f);
	m_cube2_world = translate(9, 3, -6)* rotateY(0.65f) * scale(2, 2, 2);
	m_cube2_world_normal = rotateY(0.65f);

	float dist = 80;
	m_lights[0].Position = vec3(-dist, 30, dist);
	m_lights[0].Intensity = 250.0f;
	m_lights[0].Color = vec3(1, 0, 0);

	m_lights[1].Position = vec3(dist, 30, dist);
	m_lights[1].Intensity = 250.0f;
	m_lights[1].Color = vec3(1, 1, 0);

	m_lights[2].Position = vec3(dist, 30, -dist);
	m_lights[2].Intensity = 250.0f;
	m_lights[2].Color = vec3(0, 1, 1);

	m_lights[3].Position = vec3(-dist, 30, -dist);
	m_lights[3].Intensity = 250.0f;
	m_lights[3].Color = vec3(1, 0, 1);

	m_lights[4].Position = vec3(0, 25, 0);
	m_lights[4].Intensity = 250.0f;
	m_lights[4].Color = vec3(0.5f, 0.5f, 0.5f);

	m_lights[5].Position = vec3(0, 40, 0);
	m_lights[5].Intensity = 250.0f;
	m_lights[5].Color = vec3(0.8f, 1, 0.8f);


	RECT rect;
	GetClientRect(hwnd, &rect);
	m_ratio = float(rect.bottom - rect.top) / float(rect.right - rect.left);
	m_width = float(rect.right - rect.left);
	int height = rect.bottom - rect.top;

#if 0
	preComputeLightMap();
#endif
	m_light_map_ssid = renderer->addSamplerState(BILINEAR, CLAMP, CLAMP, CLAMP);
	m_lm0_id = renderer->addTexture(ShaderDir("/LightMap0.dds"), false, m_light_map_ssid);
	m_lm1_id = renderer->addTexture(ShaderDir("/LightMap1.dds"), false, m_light_map_ssid);
	m_lm2_id = renderer->addTexture(ShaderDir("/LightMap2.dds"), false, m_light_map_ssid);
	m_lm_ground_id = renderer->addTexture(ShaderDir("/LightMap_ground.dds"), false, m_light_map_ssid);
	
	m_sphere = new Model();
	m_sphere->createSphere(2);
	m_sphere->makeDrawable(renderer,true,m_plain_light_shd);
	
	m_light_blend = renderer->addBlendState(ONE, ONE);


	m_shadow_map_ssid = renderer->addSamplerState(LINEAR, CLAMP, CLAMP, CLAMP, 0.0f, 1, LESS);
	m_shadow_map_id = renderer->addRenderDepth(256, 256, 1, FORMAT_D32F, 1, m_shadow_map_ssid, CUBEMAP | SAMPLE_DEPTH);
	m_shadow_map_gs_shd = renderer->addShader(ShaderDir("/shadow_gs.shd"));

	m_ground_ssid = renderer->addSamplerState(TRILINEAR_ANISO, WRAP, WRAP, WRAP);
	m_ground_tex_id = renderer->addTexture(ResDir("/Textures/FieldStone.dds"), true, m_ground_ssid);
	 m_ground_normal_id = renderer->addNormalMap(ResDir("/Textures/FieldStoneBump.dds"), FORMAT_RGBA8, true, m_ground_ssid);

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
	
	m_color_rt = renderer->addRenderTarget(m_width, height, FORMAT_RGBA8, SS_NONE);
	m_depth_rt = renderer->addRenderTarget(m_width, height, FORMAT_R16, SS_NONE);
	m_depth_rt2 = renderer->addRenderDepth(m_width, height,1,FORMAT_D24,1,SS_NONE,SAMPLE_DEPTH);

	return true;
}



Model* VolumeLightingApp::createCube()
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

		float x0 = float(rect->x + 1) / m_lm_width;
		float y0 = float(rect->y + 1) / m_lm_height;
		float x1 = float(rect->x + rect->width - 1) / m_lm_width;
		float y1 = float(rect->y + rect->height - 1) / m_lm_height;

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

void VolumeLightingApp::computFace(const vec3& v0, const vec3& v1, const vec3& v2, const vec2& t0, const vec2& t2, uint8* lm,int width,int height)
{
	int left = (int)(t0.x);
	int right = (int)(t2.x);
	int top = (int)(t0.y);
	int bottom = (int)(t2.y);
	if (left < 0 || right > width || top < 0 || top > height) {
		ErrorMsg("computeFace: check the texcoord of light map");
		return;
	}

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

void VolumeLightingApp::fillBorder(const vec2 & t0, const vec2 & t2, uint8 * lm, int width,int height)
{
	int left = (int)(t0.x) - 1;
	int right = (int)(t2.x);
	int top = (int)(t0.y) - 1;
	int bottom = (int)(t2.y);
	if (left >= 0) {//copy left
		int row = min(bottom, height);
		for (int i = top + 1; i < row; ++i) {
			int c = 3 * (i * width + left);
			lm[c] = lm[c + 3];
			lm[c + 1] = lm[c + 3 + 1];
			lm[c + 2] = lm[c + 3 + 2];
		}
	}
	if (right < width) {//copy right
		int row = min(bottom, height);
		for (int i = top + 1; i < row; ++i) {
			int c = 3 * (i * width + right);
			lm[c] = lm[c - 3];
			lm[c + 1] = lm[c - 3 + 1];
			lm[c + 2] = lm[c - 3 + 2];
		}
	}
	if (top >= 0) {//copy top
		int DstAdd = 3 * (top * width + t0.x);
		int SrcAdd = 3 * (((int)t0.y) * width + t0.x);
		memcpy(lm + DstAdd, lm + SrcAdd, (int(t2.x) - int(t0.x)) * 3);
	}

	if (bottom < height) {//copy bottom
		int DstAdd = 3 * (bottom * width + t0.x);
		int SrcAdd = 3 * (((int)t2.y - 1) * width + t0.x);
		memcpy(lm + DstAdd, lm + SrcAdd, (int(t2.x) - int(t0.x)) * 3);
	}
	if (left >= 0 && top >= 0) {
		int c = (top * width + left) * 3;
		lm[c] = lm[c + 3];
		lm[c + 1] = lm[c + 3 + 1];
		lm[c + 2] = lm[c + 3 + 2];
	}
	if (right < width && top >= 0) {
		int c = (top * width + right) * 3;
		lm[c] = lm[c - 3];
		lm[c + 1] = lm[c - 3 + 1];
		lm[c + 2] = lm[c - 3 + 2];
	}

	if (left >= 0 && bottom < height) {
		int c = (bottom * width + left) * 3;
		lm[c] = lm[c + 3];
		lm[c + 1] = lm[c + 3 + 1];
		lm[c + 2] = lm[c + 3 + 2];
	}

	if (right < width && bottom < height) {
		int c = (bottom * width + right) * 3;
		lm[c] = lm[c - 3];
		lm[c + 1] = lm[c - 3 + 1];
		lm[c + 2] = lm[c - 3 + 2];
	}
}

void VolumeLightingApp::blurLightMap(uint8 * lm, int width, int height)
{
	for (int j = 0; j < height; ++j) {
		for (int i = 0; i < width; ++i) {
			int left = i - 1;
			left = left < 0 ? 0 : left;
			int right = i + 1;
			right = right >= width ? width - 1 : right;
			int top = j - 1;
			top = top < 0 ? 0 : top;
			int bottom = j + 1;
			bottom = bottom >= height ? height - 1 : bottom;
			for (int c = 0; c < 3; ++c) {
				int vCenter = lm[(j * width + i) * 3 + c];
				int vLeft = lm[(j * width + left) * 3 + c];
				int vRight = lm[(j * width + right) * 3 + c];
				int vTop = lm[(top * width + i) * 3 + c];
				int vBottom = lm[(bottom * width + i) * 3 + c];
				lm[(j * width + i) * 3 + c] = ((vLeft + vRight + vTop + vBottom) >> 2);
			}
		}
	}
}

void VolumeLightingApp::preComputeLightMap()
{

	float3* verts = (float3*)m_cube->getStream(0).vertices;
	uint*  indices = m_cube->getStream(0).indices;
	float2* texcoords = (float2*)m_cube->getStream(2).vertices;
	uint* texIndices = m_cube->getStream(2).indices;

	int index = 0;
	{
		//create BSP tree to detect insect infomation
		for (int k = 0; k < 2; k++) {
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
	for (int k = 0; k < 2; k++) {
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
			t0 = texcoords[texIndices[index]] * float2(m_lm_width, m_lm_height);
			t2 = texcoords[texIndices[index + 2]] * float2(m_lm_width, m_lm_height);
			computFace(v0, v1, v2, t0, t2, lm, m_lm_width, m_lm_height);
			fillBorder(t0, t2, lm, m_lm_width, m_lm_height);
			index += 6;
		}
		blurLightMap(lm, m_lm_width, m_lm_height);
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
	v0 = vec3(-100, 0, 100);
	v1 = vec3(100, 0, 100);
	v2 = vec3(100, 0, -100);
	t0 = float2(0,0);
	t2 = float2(512, 512);
	computFace(v0, v1, v2, t0, t2, lm2,512,512);
	blurLightMap(lm2, 512, 512);
	Image image;
	image.loadFromMemory(lm2, FORMAT_RGB8, 512, 512, 1, 1, true);
	char fileName[256];
	sprintf(fileName, ShaderDir("/LightMap_ground.dds"));
	image.saveImage(fileName);
}

void VolumeLightingApp::updateFrame()
{
	//m_cube0_world = translate(-5, 10, 5)* rotateZXY(0.65f * time, 0.55f * time, 1.25f * time);
	vec3 eye(0, 25, 0);
	vec3 lookAt(0, 0, 0);
	vec3 up(0, 0, 1);
	mat4 view = makeViewMatrixD3D(eye, lookAt, up);
	mat4 world = translate(-400, -100, 250) * scale(10, 10, 10) * rotateZXY(0, 0, PI);
	mat4 proj = orthoMatrixX(-m_width * 0.5f, m_width * 0.5f, m_width * 0.5f * m_ratio, -m_width * 0.5f * m_ratio, -200, 200);
	m_cube_mvp = proj * view * world;


	m_move_light = rotateY(/*PI * 1.18f*/0) * translate(0, 80, 80);
}

bool VolumeLightingApp::onKey(const uint key, const bool pressed)
{
	if (D3D11App::onKey(key, pressed))
		return true;
	if (key == KEY_LEFT) {
		m_camera_angel += 0.1f;
	}
	else if (key == KEY_RIGHT) {
		m_camera_angel -= 0.1f;
	}
	else if (key == KEY_UP) {
		m_camera_dist += 5;
	}
	else if (key == KEY_DOWN) {
		m_camera_dist -= 5;
	}

	return true;
}

void VolumeLightingApp::drawFrame()
{
	updateFrame();

	//generate shadow map first.
	float zNear = 30;
	float zFar = 250.0f;
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

	//	renderer->setShaderConstant4x4f("worldMatrix", m_cube2_world);
	//	renderer->apply();
	//	m_cube->draw(renderer);
	}
	//renderer->changeToMainFramebuffer();
	TextureID buffs[] = { m_color_rt,m_depth_rt};
	renderer->changeRenderTargets(buffs, 2, m_depth_rt2);
	((Direct3D11Renderer*)renderer)->clearRenderTarget(m_color_rt, float4(0, 0, 0, 0));
	((Direct3D11Renderer*)renderer)->clearRenderTarget(m_depth_rt, float4(1, 1, 1, 1));
	((Direct3D11Renderer*)renderer)->clearDepthTarget(m_depth_rt2, 1);


	vec3 eye(0, m_camera_dist,-m_camera_dist);
	eye = (rotateY(m_camera_angel) * vec4(eye,1)).xyz();
	vec3 lookAt(0,0,0);
	vec3 up(0, 1, 0);
	mat4 view = makeViewMatrixD3D(eye, lookAt, up);
	mat4 proj = perspectiveMatrix(PI/12, m_ratio, 1, 1000);
	mat4 viewProj = proj * view;


	renderer->reset();
	renderer->setShader(m_light_lm_shd);
	renderer->setShaderConstant4x4f("view", view);
	renderer->setShaderConstant4x4f("proj", proj);
	renderer->setTexture("shadowMap", m_shadow_map_id);
	renderer->setSamplerState("shadowFilter", m_light_map_ssid);
	renderer->setSamplerState("fliter", m_light_map_ssid);
	renderer->setShaderConstant4f("vMoveLightColor", vec4(vLightColor,1));
	renderer->setShaderConstant4f("vMoveLightPos", vec4(vLightPos,1));
	renderer->setShaderConstant2f("nf", float2(zFar * zNear / (zNear - zFar), zFar / (zFar - zNear)));
	renderer->setShaderConstant1f("denisty", 1000);
	
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

#if 0	
	renderer->setShaderConstant4x4f("worldMatrix", m_cube2_world);
	renderer->setShaderConstant4x4f("worldMatrixNormal", m_cube2_world_normal);
	renderer->setTexture("LightMap", m_lm2_id);
	renderer->apply();
	m_cube->draw(renderer);
#endif

	renderer->setShader(m_ground_shd);
	renderer->setShaderConstant4x4f("viewProj", viewProj);
	renderer->setTexture("shadowMap", m_shadow_map_id);
	renderer->setSamplerState("shadowFilter", m_light_map_ssid);
	renderer->setSamplerState("lightMapfliter", m_light_map_ssid);
	renderer->setSamplerState("fliter", m_ground_ssid);
	renderer->setShaderConstant4f("vMoveLightColor", vec4(vLightColor, 1));
	renderer->setShaderConstant4f("vMoveLightPos", vec4(vLightPos, 1));
	renderer->setShaderConstant2f("nf", float2(zFar * zNear / (zNear - zFar), zFar / (zFar - zNear)));
	renderer->setShaderConstant1f("denisty", 1000);
	renderer->setShaderConstant4x4f("worldMatrix", identity4());
	renderer->setTexture("LightMap", m_lm_ground_id);
	renderer->setTexture("tex", m_ground_tex_id);
	renderer->setTexture("normal", m_ground_normal_id);
	renderer->setShaderConstant4x4f("worldMatrixNormal", identity4());
	renderer->apply();
	renderer->changeVertexFormat(m_quad_vf);
	renderer->changeVertexBuffer(0, m_quad_vb);
	renderer->changeIndexBuffer(m_quad_ib);
	renderer->drawElements(PRIM_TRIANGLES, 0, 6, 0, 4);

	renderer->reset();
	renderer->setBlendState(m_light_blend);
	renderer->setShader(m_plain_light_shd);
#if 0
	for (int k = 0; k < 6; ++k) {
		mat4 m_light_mvp = viewProj * translate(m_lights[k].Position);
		renderer->setShaderConstant4x4f("worldViewProj", m_light_mvp);
		renderer->setShaderConstant4f("vLightColor", vec4(m_lights[k].Color, 1));
		renderer->apply();
		m_sphere->draw(renderer);
	}
#endif
	//draw move light
	mat4 m_light_mvp = viewProj * translate(vLightPos);
	renderer->setShaderConstant4x4f("worldViewProj", m_light_mvp);
	renderer->setShaderConstant4f("vLightColor", vec4(vLightColor, 1));
	renderer->apply();
	m_sphere->draw(renderer);

#if 0
	//show shadow map
	renderer->reset();
	renderer->setRasterizerState(cullBack);
	renderer->setDepthState(noDepthWrite);
	renderer->setShader(m_plain_texture_shd);
	renderer->setTexture("cube", m_shadow_map_id);
	renderer->setShaderConstant4x4f("worldViewProj", m_cube_mvp);
	renderer->apply();
	renderer->changeVertexFormat(m_cube_vf_for_showSM);
	renderer->changeVertexBuffer(0, m_cube_vb_for_showSM);
	renderer->changeIndexBuffer(m_cube_ib_for_showSM);
	renderer->drawElements(PRIM_TRIANGLES, 0, 36, 0, 8);
#endif

#if 1
	renderer->changeToMainFramebuffer();
	renderer->reset();
	renderer->setDepthState(noDepthTest);
	renderer->setShader(m_full_screen_shd);
	renderer->setTexture("base", m_depth_rt);
	renderer->setSamplerState("filter", linearClamp);
	renderer->apply();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(3, 0);

#endif
}
#if 0
/* Hi there!
* Here is a demo presenting volumetric rendering single with shadowing.
* Did it quickly so I hope I have not made any big mistakes :)
*
* I also added the improved scattering integration I propose in my SIGGRAPH'15 presentation
* about Frostbite new volumetric system I have developed. See slide 28 at http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/
* Basically it improves the scattering integration for each step with respect to extinction
* The difference is mainly visible for some participating media having a very strong scattering value.
* I have setup some pre-defined settings for you to checkout below (to present the case it improves):
* - D_DEMO_SHOW_IMPROVEMENT_xxx: shows improvement (on the right side of the screen). You can still see aliasing due to volumetric shadow and the low amount of sample we take for it.
* - D_DEMO_SHOW_IMPROVEMENT_xxx_NOVOLUMETRICSHADOW: same as above but without volumetric shadow
*
* To increase the volumetric rendering accuracy, I constrain the ray marching steps to a maximum distance.
*
* Volumetric shadows are evaluated by raymarching toward the light to evaluate transmittance for each view ray steps (ouch!)
*
* Do not hesitate to contact me to discuss about all that :)
* SebH



*/



/*
* This are predefined settings you can quickly use
*    - D_DEMO_FREE play with parameters as you would like
*    - D_DEMO_SHOW_IMPROVEMENT_FLAT show improved integration on flat surface
*    - D_DEMO_SHOW_IMPROVEMENT_NOISE show improved integration on noisy surface
*    - the two previous without volumetric shadows
*/
#define D_DEMO_FREE
//#define D_DEMO_SHOW_IMPROVEMENT_FLAT
//#define D_DEMO_SHOW_IMPROVEMENT_NOISE
//#define D_DEMO_SHOW_IMPROVEMENT_FLAT_NOVOLUMETRICSHADOW
//#define D_DEMO_SHOW_IMPROVEMENT_NOISE_NOVOLUMETRICSHADOW





#ifdef D_DEMO_FREE
// Apply noise on top of the height fog?
#define D_FOG_NOISE 1.0

// Height fog multiplier to show off improvement with new integration formula
#define D_STRONG_FOG 0.0

// Enable/disable volumetric shadow (single scattering shadow)
#define D_VOLUME_SHADOW_ENABLE 1

// Use imporved scattering?
// In this mode it is full screen and can be toggle on/off.
#define D_USE_IMPROVE_INTEGRATION 1

//
// Pre defined setup to show benefit of the new integration. Use D_DEMO_FREE to play with parameters
//
#elif defined(D_DEMO_SHOW_IMPROVEMENT_FLAT)
#define D_STRONG_FOG 10.0
#define D_FOG_NOISE 0.0
#define D_VOLUME_SHADOW_ENABLE 1
#elif defined(D_DEMO_SHOW_IMPROVEMENT_NOISE)
#define D_STRONG_FOG 5.0
#define D_FOG_NOISE 1.0
#define D_VOLUME_SHADOW_ENABLE 1
#elif defined(D_DEMO_SHOW_IMPROVEMENT_FLAT_NOVOLUMETRICSHADOW)
#define D_STRONG_FOG 10.0
#define D_FOG_NOISE 0.0
#define D_VOLUME_SHADOW_ENABLE 0
#elif defined(D_DEMO_SHOW_IMPROVEMENT_NOISE_NOVOLUMETRICSHADOW)
#define D_STRONG_FOG 3.0
#define D_FOG_NOISE 1.0
#define D_VOLUME_SHADOW_ENABLE 0
#endif



/*
* Other options you can tweak
*/

// Used to control wether transmittance is updated before or after scattering (when not using improved integration)
// If 0 strongly scattering participating media will not be energy conservative
// If 1 participating media will look too dark especially for strong extinction (as compared to what it should be)
// Toggle only visible zhen not using the improved scattering integration.
#define D_UPDATE_TRANS_FIRST 0

// Apply bump mapping on walls
#define D_DETAILED_WALLS 0

// Use to restrict ray marching length. Needed for volumetric evaluation.
#define D_MAX_STEP_LENGTH_ENABLE 1

// Light position and color
#define LPOS vec3( 20.0+15.0*sin(iGlobalTime), 15.0+12.0*cos(iGlobalTime),-20.0)
#define LCOL (600.0*vec3( 1.0, 0.9, 0.5))


float displacementSimple(vec2 p)
{
	float f;
	f = 0.5000* textureLod(iChannel0, p, 0.0).x; p = p*2.0;
	f += 0.2500* textureLod(iChannel0, p, 0.0).x; p = p*2.0;
	f += 0.1250* textureLod(iChannel0, p, 0.0).x; p = p*2.0;
	f += 0.0625* textureLod(iChannel0, p, 0.0).x; p = p*2.0;

	return f;
}


vec3 getSceneColor(vec3 p, float material)
{
	if (material == 1.0)
	{
		return vec3(1.0, 0.5, 0.5);
	}
	else if (material == 2.0)
	{
		return vec3(0.5, 1.0, 0.5);
	}
	else if (material == 3.0)
	{
		return vec3(0.5, 0.5, 1.0);
	}

	return vec3(0.0, 0.0, 0.0);
}


float getClosestDistance(vec3 p, out float material)
{
	float d = 0.0;
#if D_MAX_STEP_LENGTH_ENABLE
	float minD = 1.0; // restrict max step for better scattering evaluation
#else
	float minD = 10000000.0;
#endif
	material = 0.0;

	float yNoise = 0.0;
	float xNoise = 0.0;
	float zNoise = 0.0;
#if D_DETAILED_WALLS
	yNoise = 1.0*clamp(displacementSimple(p.xz*0.005), 0.0, 1.0);
	xNoise = 2.0*clamp(displacementSimple(p.zy*0.005), 0.0, 1.0);
	zNoise = 0.5*clamp(displacementSimple(p.xy*0.01), 0.0, 1.0);
#endif

	d = max(0.0, p.y - yNoise);
	if (d<minD)
	{
		minD = d;
		material = 2.0;
	}

	d = max(0.0, p.x - xNoise);
	if (d<minD)
	{
		minD = d;
		material = 1.0;
	}

	d = max(0.0, 40.0 - p.x - xNoise);
	if (d<minD)
	{
		minD = d;
		material = 1.0;
	}

	d = max(0.0, -p.z - zNoise);
	if (d<minD)
	{
		minD = d;
		material = 3.0;
	}

	return minD;
}


vec3 calcNormal(in vec3 pos)
{
	float material = 0.0;
	vec3 eps = vec3(0.3, 0.0, 0.0);
	return normalize(vec3(
		getClosestDistance(pos + eps.xyy, material) - getClosestDistance(pos - eps.xyy, material),
		getClosestDistance(pos + eps.yxy, material) - getClosestDistance(pos - eps.yxy, material),
		getClosestDistance(pos + eps.yyx, material) - getClosestDistance(pos - eps.yyx, material)));

}

vec3 evaluateLight(in vec3 pos)
{
	vec3 lightPos = LPOS;
	vec3 lightCol = LCOL;
	vec3 L = lightPos - pos;
	float distanceToL = length(L);
	return lightCol * 1.0 / (distanceToL*distanceToL);
}

vec3 evaluateLight(in vec3 pos, in vec3 normal)
{
	vec3 lightPos = LPOS;
	vec3 L = lightPos - pos;
	float distanceToL = length(L);
	vec3 Lnorm = L / distanceToL;
	return max(0.0, dot(normal, Lnorm)) * evaluateLight(pos);
}

// To simplify: wavelength independent scattering and extinction
void getParticipatingMedia(out float muS, out float muE, in vec3 pos)
{
	float heightFog = 7.0 + D_FOG_NOISE*3.0*clamp(displacementSimple(pos.xz*0.005 + iGlobalTime*0.01), 0.0, 1.0);
	heightFog = 0.3*clamp((heightFog - pos.y)*1.0, 0.0, 1.0);

	const float fogFactor = 1.0 + D_STRONG_FOG * 5.0;

	const float sphereRadius = 5.0;
	float sphereFog = clamp((sphereRadius - length(pos - vec3(20.0, 19.0, -17.0))) / sphereRadius, 0.0, 1.0);

	const float constantFog = 0.02;

	muS = constantFog + heightFog*fogFactor + sphereFog;

	const float muA = 0.0;
	muE = max(0.000000001, muA + muS); // to avoid division by zero extinction
}

float phaseFunction()
{
	return 1.0 / (4.0*3.14);
}

float volumetricShadow(in vec3 from, in vec3 to)
{
#if D_VOLUME_SHADOW_ENABLE
	const float numStep = 16.0; // quality control. Bump to avoid shadow alisaing
	float shadow = 1.0;
	float muS = 0.0;
	float muE = 0.0;
	float dd = length(to - from) / numStep;
	for (float s = 0.5; s<(numStep - 0.1); s += 1.0)// start at 0.5 to sample at center of integral part
	{
		vec3 pos = from + (to - from)*(s / (numStep));
		getParticipatingMedia(muS, muE, pos);
		shadow *= exp(-muE * dd);
	}
	return shadow;
#else
	return 1.0;
#endif
}

void traceScene(bool improvedScattering, vec3 rO, vec3 rD, inout vec3 finalPos, inout vec3 normal, inout vec3 albedo, inout vec4 scatTrans)
{
	const int numIter = 100;

	float muS = 0.0;
	float muE = 0.0;

	vec3 lightPos = LPOS;

	// Initialise volumetric scattering integration (to view)
	float transmittance = 1.0;
	vec3 scatteredLight = vec3(0.0, 0.0, 0.0);

	float d = 1.0; // hack: always have a first step of 1 unit to go further
	float material = 0.0;
	vec3 p = vec3(0.0, 0.0, 0.0);
	float dd = 0.0;
	for (int i = 0; i<numIter; ++i)
	{
		vec3 p = rO + d*rD;


		getParticipatingMedia(muS, muE, p);

#ifdef D_DEMO_FREE
		if (D_USE_IMPROVE_INTEGRATION>0) // freedom/tweakable version
#else
		if (improvedScattering)
#endif
		{
			// See slide 28 at http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/
			vec3 S = evaluateLight(p) * muS * phaseFunction()* volumetricShadow(p, lightPos);// incoming light
			vec3 Sint = (S - S * exp(-muE * dd)) / muE; // integrate along the current step segment
			scatteredLight += transmittance * Sint; // accumulate and also take into account the transmittance from previous steps

													// Evaluate transmittance to view independentely
			transmittance *= exp(-muE * dd);
		}
		else
		{
			// Basic scatering/transmittance integration
#if D_UPDATE_TRANS_FIRST
			transmittance *= exp(-muE * dd);
#endif
			scatteredLight += muS * evaluateLight(p) * phaseFunction() * volumetricShadow(p, lightPos) * transmittance * dd;
#if !D_UPDATE_TRANS_FIRST
			transmittance *= exp(-muE * dd);
#endif
		}


		dd = getClosestDistance(p, material);
		if (dd<0.2)
			break; // give back a lot of performance without too much visual loss
		d += dd;
	}

	albedo = getSceneColor(p, material);

	finalPos = rO + d*rD;

	normal = calcNormal(finalPos);

	scatTrans = vec4(scatteredLight, transmittance);
}


void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
	//iGlobalTime
	//iMouse
	//iResolution

	vec2 uv = fragCoord.xy / iResolution.xy;
	vec2 uv2 = 2.0*fragCoord.xy / iResolution.xy - 1.0;

	vec3 camPos = vec3(20.0, 18.0, -50.0);
	if (iMouse.x + iMouse.y > 0.0) // to handle first loading and see somthing on screen
		camPos += vec3(0.05, 0.12, 0.0)*(vec3(iMouse.x, iMouse.y, 0.0) - vec3(iResolution.xy*0.5, 0.0));
	vec3 camX = vec3(1.0, 0.0, 0.0) *0.75;
	vec3 camY = vec3(0.0, 1.0, 0.0) *0.5;
	vec3 camZ = vec3(0.0, 0.0, 1.0);

	vec3 rO = camPos;
	vec3 rD = normalize(uv2.x*camX + uv2.y*camY + camZ);
	vec3 finalPos = rO;
	vec3 albedo = vec3(0.0, 0.0, 0.0);
	vec3 normal = vec3(0.0, 0.0, 0.0);
	vec4 scatTrans = vec4(0.0, 0.0, 0.0, 0.0);
	traceScene(fragCoord.x>(iResolution.x / 2.0),
		rO, rD, finalPos, normal, albedo, scatTrans);


	//lighting
	vec3 color = (albedo / 3.14) * evaluateLight(finalPos, normal) * volumetricShadow(finalPos, LPOS);
	// Apply scattering/transmittance
	color = color * scatTrans.w + scatTrans.xyz;

	// Gamma correction
	color = pow(color, vec3(1.0 / 2.2)); // simple linear to gamma, exposure of 1.0

#ifndef D_DEMO_FREE
										 // Separation line
	if (abs(fragCoord.x - (iResolution.x*0.5))<0.6)
		color.r = 0.5;
#endif

	fragColor = vec4(color, 1.0);
}




#endif