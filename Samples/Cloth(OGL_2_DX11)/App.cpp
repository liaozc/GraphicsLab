#include "App.h"
#include "Math/Noise.h"
#include "CPU.h"
#include "Util/Model.h"
#include "Util/String.h"

#define ProjectDir	"/Samples/Cloth(OGL_2_DX11)"
#include "InitResDir.inl"

struct DrawBillboardVert {
	vec3 position;
	vec2 texcoord;
};

BaseApp* app = new ClothApp();

bool ClothApp::init()
{
	initWorkDir();
	initNoise();
	initCPU();
	nextTime = 0;
	srand((unsigned)getHz());
	return true;
}

void ClothApp::exit()
{
	delete m_sphere;
}

bool ClothApp::load()
{
	m_lighting_shd = renderer->addShader(ShaderDir("/lighting.shd"));
	m_sphere_shd = renderer->addShader(ShaderDir("/sphere.shd"));
	m_billboard_shd = renderer->addShader(ShaderDir("/billboard.shd"));
	
	m_sphere = new Model();
	m_sphere->createSphere(3);
	m_sphere->computeNormals(false);
	m_sphere->makeDrawable(renderer, true, m_sphere_shd);
	//light
	m_light_id = renderer->addTexture(ResDir("/Textures/Particle.tga"), true, SS_NONE);
	m_light_blend = renderer->addBlendState(ONE, ONE);
	//flags
	int result;
	WIN32_FIND_DATA findFileData;
	HANDLE handle = FindFirstFile(ResDir("/Textures/Flags/*.*"), &findFileData);
	if (handle != INVALID_HANDLE_VALUE) {
		do {
			if (findFileData.cFileName[0] != '.') {
				TextureID tex = renderer->addTexture(String(ResDir("/Textures/Flags/")) + findFileData.cFileName, true,SS_NONE);
				if (tex != TEXTURE_NONE) m_flags.add(tex);
			}
			result = FindNextFile(handle, &findFileData);
		} while (result > 0);
		FindClose(handle);
	}
	if (m_flags.getCount() == 0) return false;

	DrawBillboardVert verts[] = {
		{ vec3(-10,0,10),vec2(0,0) },
		{ vec3(10,0,10),vec2(1,0) },
		{ vec3(10,0,-10),vec2(1,1) },
		{ vec3(-10,0,-10),vec2(0,1) },
	};
	ushort inds[] = {
		0,1,2,
		0,2,3
	};
	FormatDesc vfDes[] = {
		{0,TYPE_VERTEX,FORMAT_FLOAT,3},
		{ 0,TYPE_TEXCOORD,FORMAT_FLOAT,2 }
	};
	m_light_vb = renderer->addVertexBuffer(sizeof(verts), DYNAMIC, verts);
	m_light_ib = renderer->addIndexBuffer(sizeof(inds) / sizeof(ushort), sizeof(ushort), STATIC, inds);
	m_light_vf = renderer->addVertexFormat(vfDes, 2, m_billboard_shd);


	m_flag_vb = renderer->addVertexBuffer(sizeof(m_flag_vertexs), DYNAMIC, m_flag_vertexs);
	FormatDesc vfDes2[] = {
		{ 0,TYPE_VERTEX,FORMAT_FLOAT,3 },
		{ 0,TYPE_NORMAL,FORMAT_FLOAT,3 },
		{ 0,TYPE_TEXCOORD,FORMAT_FLOAT,2 }
	};
	m_flag_vf = renderer->addVertexFormat(vfDes2, 3, m_lighting_shd);
	unsigned short flag_inds[(CLOTH_SIZE_X - 1) * (CLOTH_SIZE_Y - 1) * 6] ;
	unsigned index = 0;
	for (unsigned int j = 0; j < CLOTH_SIZE_Y - 1; j++) {
		for (unsigned int i = 0; i < CLOTH_SIZE_X - 1; i++){
			flag_inds[index++] = j * (CLOTH_SIZE_X) + i; //0
			flag_inds[index++] = j * (CLOTH_SIZE_X)+ i + 1; // 1
			flag_inds[index++] = j * (CLOTH_SIZE_X)+ i + 1 + CLOTH_SIZE_X; //2
			flag_inds[index++] = j * (CLOTH_SIZE_X)+ i; //0
			flag_inds[index++] = j * (CLOTH_SIZE_X)+ i + 1 + CLOTH_SIZE_X; // 2
			flag_inds[index++] = j * (CLOTH_SIZE_X)+ i + CLOTH_SIZE_X; //3
		}
	}
	m_flag_ib = renderer->addIndexBuffer(sizeof(flag_inds) / sizeof(ushort), sizeof(ushort), STATIC, flag_inds);

	RECT rect;
	GetClientRect(hwnd, &rect);
	m_ratio = float(rect.bottom - rect.top) / float(rect.right - rect.left);

	return true;
}

void ClothApp::drawLight(const vec3 & lightPos,float size,const vec3& camPos, const vec3& up,const mat4& viewProj)
{
	vec3 viewDir = normalize(lightPos - camPos);
	vec3 right = normalize(cross(up, viewDir));
	vec3 rup = cross(viewDir, right);
	DrawBillboardVert verts[] = {
		{ lightPos - right * size * 0.5f + rup * size * 0.5f,vec2(0,0)},
		{ lightPos + right * size * 0.5f + rup * size * 0.5f,vec2(1,0) },
		{ lightPos + right * size * 0.5f - rup * size * 0.5f,vec2(1,1) },
		{ lightPos - right * size * 0.5f - rup * size * 0.5f,vec2(0,1) },
	};
	bool ret = renderer->updateVertexBuffer(m_light_vb, sizeof(verts), verts);
	if (!ret) return;

	renderer->reset();
	renderer->setBlendState(m_light_blend);
	renderer->setShader(m_billboard_shd);
	renderer->setTexture("base",m_light_id);
	renderer->setShaderConstant4x4f("viewProj",viewProj);
	renderer->apply();
	renderer->changeVertexFormat(m_light_vf);
	renderer->changeVertexBuffer(0, m_light_vb);
	renderer->changeIndexBuffer(m_light_ib);
	renderer->drawElements(PRIM_TRIANGLES, 0, 6, 0, 4);
}

void ClothApp::drawSphere(const vec3 & lightPos, const vec3 & spherePos, const float size, const vec3 & color, const mat4& viewProj, const vec3& camPos)
{
	mat4 world = scale(size, size, size);
	world = translate(spherePos) * world;
	renderer->reset();
	renderer->setShader(m_sphere_shd);
	renderer->setShaderConstant4x4f("viewProj", viewProj);
	renderer->setShaderConstant4x4f("world", world);
	renderer->setShaderConstant4f("color", vec4(color, 0.0f));
	renderer->setShaderConstant3f("lightPos", lightPos);
	renderer->setShaderConstant3f("camPos", camPos);
	renderer->apply();
	m_sphere->draw(renderer);
}

void ClothApp::drawCloth(const vec3 & lightPos, const mat4& viewProj, const vec3& camPos)
{
	renderer->reset();
	renderer->setRasterizerState(cullNone);
	renderer->setShader(m_lighting_shd);
	renderer->setTexture("Base", m_base_id);
	renderer->setSamplerState("Filter", linearClamp);
	renderer->setShaderConstant4x4f("world", identity4());
	renderer->setShaderConstant4x4f("viewProj", viewProj);
	renderer->setShaderConstant3f("lightPos", lightPos);
	renderer->setShaderConstant3f("camPos", camPos);
	renderer->apply();

	bool ret = renderer->updateVertexBuffer(m_flag_vb, sizeof(m_flag_vertexs), m_flag_vertexs);
	if (!ret) return;

	renderer->changeVertexFormat(m_flag_vf);
	renderer->changeVertexBuffer(0, m_flag_vb);
	renderer->changeIndexBuffer(m_flag_ib);
	renderer->drawElements(PRIM_TRIANGLES, 0, (CLOTH_SIZE_X - 1) * (CLOTH_SIZE_Y - 1) * 6, 0, CLOTH_SIZE_Y * CLOTH_SIZE_X);
}

void ClothApp::drawFrame()
{
	float col[4] = { 0.5f,0.5f,0.5f,1 };
	renderer->clear(true, true, col, 1.0f);

	static int cornerInds[] = { 0, CLOTH_SIZE_X - 1, CLOTH_SIZE_X * CLOTH_SIZE_Y - 1, (CLOTH_SIZE_Y - 1) * CLOTH_SIZE_X };
	unsigned int i, j;
	static float sum = 0;

	//vec3 eye(0, 400, -250);
	//vec3 lookAt(0, 0, 0);
	//vec3 up(0, 1, 0);
	//mat4 view = makeViewMatrixD3D(eye, lookAt, up);
	//mat4 proj = perspectiveMatrix(PI / 2, m_ratio, 1, 6000);
	//mat4 vp = proj * view;

	vec3 eye(120, 80, -190);
	mat4 proj = perspectiveMatrix(1.5f, m_ratio, 0.1f, 600);
	mat4 view = rotateZXY(-wx, -wy, -0);
	view.translate(-eye);
	vec3 up = view.rows[1].xyz();
	mat4 vp = proj * view;


	vec3 lightPos = vec3(C_SIZE * CLOTH_SIZE_X * noise1(0.5f * time), 80 + 80 * noise1(0.3f * time + 23.37f), C_SIZE * CLOTH_SIZE_X * noise1(12.31f - 0.5f * time));
	
#if 1
	{//cloth simulation core

		if (time >= nextTime || ((nextTime - time < 41) && sum < 5) || sum > 9800) {
			nextTime = time + 45;

			for (j = 0; j < CLOTH_SIZE_Y; j++) {
				for (i = 0; i < CLOTH_SIZE_X; i++) {
					m_flag_vertexs[j][i].pos = vec3(C_SIZE * (i - 0.5f * (CLOTH_SIZE_X - 1)), 0, C_SIZE * (0.5f * (CLOTH_SIZE_Y - 1) - j));
					m_flag_vertexs[j][i].texCoord = vec2(float(i) / (CLOTH_SIZE_X - 1), float(j) / (CLOTH_SIZE_Y - 1));
				}
			}

			static bool first = true;
			if (first) {
				spring.addRectField(CLOTH_SIZE_X, CLOTH_SIZE_Y, m_flag_vertexs, ((char *)m_flag_vertexs) + sizeof(vec3), sizeof(DrawVert));
				first = false;
			}

			for (i = 0; i < spring.getNodeCount(); i++) {
				spring.getNode(i)->dir = vec3(0, 0, 0);
			}

			nSpheres = rand() & 3;
			for (i = 0; i < nSpheres; i++) {
				m_sphereSize[i] = 60 + 40 * float(rand()) / RAND_MAX;
				m_spherePos[i] = vec3(0.8f * C_SIZE * CLOTH_SIZE_X * (float(rand()) / RAND_MAX - 0.5f), -m_sphereSize[i] - 50 * float(rand()) / RAND_MAX, 0.8f * C_SIZE * CLOTH_SIZE_Y * (float(rand()) / RAND_MAX - 0.5f));
				m_sphereColor[i] = vec3(float(rand()) / RAND_MAX, float(rand()) / RAND_MAX, float(rand()) / RAND_MAX);
			}

			for (i = 0; i < 4; i++) spring.getNode(cornerInds[i])->locked = false;
			if (nSpheres == 0 || (rand() > RAND_MAX / 2)) {
				unsigned int pos = rand() & 3;
				unsigned int n = 2 + (rand() % 3);

				for (i = 0; i < n; i++) {
					spring.getNode(cornerInds[(pos + i) & 3])->locked = true;
				}
			}
			m_base_id = m_flags[rand() % m_flags.getCount()];
		}

		spring.update(min(frameTime, 0.0125f));

		sum = 0;
		for (i = 0; i < spring.getNodeCount(); i++) {
			SNode *node = spring.getNode(i);
			bool include = true;
			for (j = 0; j < nSpheres; j++) {
				vec3 v = *node->pos - m_spherePos[j];
				float dSqr = dot(v, v);

				if (dSqr < m_sphereSize[j] * m_sphereSize[j]) {
					*node->pos = m_spherePos[j] + m_sphereSize[j] * normalize(v);//rsqrtf(dSqr) * v;
					node->dir *= powf(0.015f, frameTime);
					include = false;
				}
			}
			if (include) sum += dot(node->dir, node->dir);
		}
		sum /= (CLOTH_SIZE_X * CLOTH_SIZE_Y);
		spring.evaluateNormals();

	}
	//draw sphere
	for (j = 0; j < nSpheres; j++) {
		drawSphere(lightPos, m_spherePos[j], m_sphereSize[j] - 1, m_sphereColor[j],vp,eye);
	}
	for (j = 0; j < 4; j++) {
		SNode *node = spring.getNode(cornerInds[j]);
		if (node->locked) drawSphere(lightPos, *node->pos, 5.0f, vec3(0.3f, 0.3f, 1.0f), vp, eye);
	}
	
	drawCloth(lightPos,vp,eye);

#endif
	drawLight(lightPos,15, eye, up, vp);

}
