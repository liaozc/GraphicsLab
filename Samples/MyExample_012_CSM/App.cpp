#include "App.h"

#define ProjectDir	"/Samples/MyExample_012_CSM"
#include "InitResDir.inl"

#include "Util/Model.h"
#include "Math/Noise.h"


BaseApp *app = new CSMApp();

CSMApp::CSMApp() : EmptyWithCameraApp(DFov,DNear,DFar * 5)
{
	m_shadow_type = ST_NORMAL;
	mb_show_sm = false;
	mb_use_ligth_as_camera = false;
	mb_use_wire = false;
	mb_use_pcf = true;
	mi_ss_type = 0;
}

bool CSMApp::init()
{
	initNoise();
	int xoffset = 0;
	for (int i = 0; i < 50; ++i){
		mat4 r = identity4();
		mat4 s = scale( 0.5f + (rand()%2), 3.5f + (rand() % 20), 1.5 + (rand() % 2));
		mat4 t = translate(xoffset + s.rows[0].x - 130 , s.rows[1].y, 0);
		mat4 m = t * r * s;
		m_cubeMats[i] = m;
		xoffset += s.rows[0].x + 20;
	}
	return true;
}

void CSMApp::exit()
{
	delete m_cube;
	delete m_panel;
	delete m_show_sm_panel;
	delete m_obj;
}


void CSMApp::calNormalLightView(vec3 lightDir)
{
	vec3 aabb_verts[] = {
		vec3(m_aabb.bMin),
		vec3(m_aabb.bMin.x,m_aabb.bMin.y,m_aabb.bMax.z),
		vec3(m_aabb.bMax.x,m_aabb.bMin.y,m_aabb.bMax.z),
		vec3(m_aabb.bMax.x,m_aabb.bMin.y,m_aabb.bMin.z),

		vec3(m_aabb.bMin.x,m_aabb.bMax.y,m_aabb.bMin.z),
		vec3(m_aabb.bMin.x,m_aabb.bMax.y,m_aabb.bMax.z),
		vec3(m_aabb.bMax),
		vec3(m_aabb.bMax.x,m_aabb.bMax.y,m_aabb.bMin.z),
	};
	//将AABB转换得到光照空间，得到在光照空间中的aabb，然后移动光源位置到aabb的xy中心，计算出矩阵
	vec3 eye = (m_aabb.bMin + m_aabb.bMax) * 0.5f;
	vec3 lookAtDir = lightDir;
	vec3 up(0, 1, 0);
	if ((abs(dot(normalize(lookAtDir), vec3(0, 1, 0)))) > 0.95f) {//too close.
		up = vec3(1, 0, 0);
	}
	mat4 lightViewMatrix = makeViewMatrixD3D(eye, eye + lookAtDir, up);
	AABBox lightAABB;
	for (int i = 0; i < 8; ++i) {
		vec3 posLightSpace;
		posLightSpace.x = dot(lightViewMatrix.rows[0], vec4(aabb_verts[i], 1));
		posLightSpace.y = dot(lightViewMatrix.rows[1], vec4(aabb_verts[i], 1));
		posLightSpace.z = dot(lightViewMatrix.rows[2], vec4(aabb_verts[i], 1));
		lightAABB.addVert(posLightSpace);
	}
	vec3 neye = (lightAABB.bMin + lightAABB.bMax) * 0.5f;
	neye.z = lightAABB.bMin.z - 1;
	mat4 inverLightViewMatrix = !lightViewMatrix;
	eye.x = dot(inverLightViewMatrix.rows[0], vec4(neye, 1));
	eye.y = dot(inverLightViewMatrix.rows[1], vec4(neye, 1));
	eye.z = dot(inverLightViewMatrix.rows[2], vec4(neye, 1));

	m_light_view = makeViewMatrixD3D(eye, eye + lookAtDir, up);
	int width = lightAABB.bMax.x - lightAABB.bMin.x;
	int height = lightAABB.bMax.y - lightAABB.bMin.y;
	int depth = lightAABB.bMax.z - lightAABB.bMin.z;
	int hw = (width >> 1) + 1;
	int hh = (height >> 1) + 1;
	mat4 lightProj = orthoMatrixX(-hw, hw, hh, -hh, 1, depth + 1);
	//mat4 lightProj = orthoMatrixX(-512, 512, -512, 512, 1, depth + 2);
	m_light_mat = lightProj * m_light_view;
}

bool CSMApp::load()
{
	initWorkDir(renderer);
	
	m_camera_shd = renderer->addShader(ShaderDir("/cameraTest.shd"));
	m_show_sm_shd = renderer->addShader(ShaderDir("/show_sm.shd"));
	m_line_shd = renderer->addShader(ShaderDir("/line.shd"));
	shadow_shd = renderer->addShader(ShaderDir("/normal_shadow.shd"));


	m_cube = new Model();
	m_cube->createCube(1, false);
	m_cube->computeNormals(true);
	m_cube->makeDrawable(renderer, true, m_camera_shd);
	
	m_panel = new Model();
	m_panel->createPanel(vec2(2000, 800), false);
	m_panel->computeNormals(true);
	m_panel->makeDrawable(renderer, true, m_camera_shd);

	m_obj = new Model();
	m_obj->loadObj(ResDir("/Models/Castle.oobj"));
	m_obj->computeTangentSpace(false);
	m_obj->makeDrawable(renderer, true, m_camera_shd);
	m_obj->cleanUp();

	m_show_sm_panel = new Model();
	m_show_sm_panel->createPanel(vec2(500, 500), true);
	m_show_sm_panel->makeDrawable(renderer, true, m_show_sm_shd);
	m_show_sm_ss = renderer->addSamplerState(BILINEAR_ANISO, CLAMP, CLAMP, CLAMP, 0.0f, 1, LESS);

	//创建渲染深度buffer
	m_rtd_sm_normal = renderer->addRenderDepth(2048, 2048, 1, FORMAT_D16, 1, SS_NONE, SAMPLE_DEPTH);
	m_rt_sm_normal = renderer->addRenderTarget(2048, 2048,1,8,1, FORMAT_RGBA32F,1, SS_NONE);
	m_ss_sm = renderer->addSamplerState(BILINEAR, CLAMP, CLAMP, CLAMP, 0.0f, 1, LESS);
	m_ss_asio_sm = renderer->addSamplerState(BILINEAR_ANISO, CLAMP, CLAMP, CLAMP, 0.0f, 16, 0);
	//计算场景AABB
	uint cube_vert_size = m_cube->getStream(0).nVertices;
	float* cube_verts = (float*)m_cube->getStream(0).vertices;
	int unitOffset = m_cube->getStream(0).nComponents;
	for (int i = 0; i < 50; ++i) {
		for (int j = 0; j < cube_vert_size; ++j) {
			vec3 worldPos;
			vec3 pos = *(vec3*)(cube_verts + unitOffset * j);
			worldPos.x = dot(m_cubeMats[i].rows[0], vec4(pos, 1));
			worldPos.y = dot(m_cubeMats[i].rows[1], vec4(pos, 1));
			worldPos.z = dot(m_cubeMats[i].rows[2], vec4(pos, 1));
			m_aabb.addVert(worldPos);
		}
	}
	uint panel_vert_size = m_panel->getStream(0).nVertices;
	float* panel_verts = (float*)m_panel->getStream(0).vertices;
	for(int i = 0; i < panel_vert_size; ++ i){
		vec3 pos = *(vec3*)(panel_verts + unitOffset * i);
		m_aabb.addVert(pos);
	}
	uint obj_vert_size = m_obj->getStream(0).nVertices;
	float* obj_verts = (float*)m_obj->getStream(0).vertices;
	int objOffset = m_obj->getStream(0).nComponents;
	for (int i = 0; i < obj_vert_size; ++i) {
		vec3 pos = *(vec3*)(obj_verts + objOffset * i);
		m_aabb.addVert(pos * 0.1f - vec3(300,0,0));
	}


	m_rs_wire = renderer->addRasterizerState(CULL_BACK, WIREFRAME);
	vec3 aabb_verts[] = {
		vec3(m_aabb.bMin),
		vec3(m_aabb.bMin.x,m_aabb.bMin.y,m_aabb.bMax.z),
		vec3(m_aabb.bMax.x,m_aabb.bMin.y,m_aabb.bMax.z),
		vec3(m_aabb.bMax.x,m_aabb.bMin.y,m_aabb.bMin.z),

		vec3(m_aabb.bMin.x,m_aabb.bMax.y,m_aabb.bMin.z),
		vec3(m_aabb.bMin.x,m_aabb.bMax.y,m_aabb.bMax.z),
		vec3(m_aabb.bMax),
		vec3(m_aabb.bMax.x,m_aabb.bMax.y,m_aabb.bMin.z),
		
	};
	ushort inds[] = {
		0,1,
		1,2,
		2,3,
		3,0,
		4,5,
		5,6,
		6,7,
		7,4,
		0,4,
		1,5,
		2,6,
		3,7
	};
	m_aabb_vb = renderer->addVertexBuffer(sizeof(aabb_verts), STATIC, aabb_verts);
	m_aabb_ib = renderer->addIndexBuffer(sizeof(inds) / sizeof(ushort), sizeof(ushort), STATIC, inds);
	FormatDesc vfd[] = {
		{0,TYPE_VERTEX,FORMAT_FLOAT,3},
	};
	m_aabb_vf = renderer->addVertexFormat(vfd, sizeof(vfd) / sizeof(FormatDesc), m_line_shd);
	calNormalLightView(vec3(-1,-0.5, -1));

	RECT rect;
	GetWindowRect(hwnd, &rect);
	float ratio = float(rect.bottom - rect.top) / float(rect.right - rect.left);
	m_cameraProj = perspectiveMatrix(m_fov, ratio, m_near, m_far);
	
	int hSW = (rect.right - rect.left)>>1;
	int hSH = (rect.bottom - rect.top) >> 1;
	vec3 show_sm_eye = vec3(hSW - 250, 50, -hSH + 250);
	mat4 show_sm_view = makeViewMatrixD3D(show_sm_eye, show_sm_eye + vec3(0, -1, 0), vec3(0, 0, 1));
	m_show_sm_mat = orthoMatrixX( -hSW, hSW, hSH, -hSH, -100, 100) * show_sm_view;
	

	return true;
}

bool CSMApp::onKey(const uint key, const bool pressed)
{
	mb_show_sm = (key == KEY_F) && pressed ? !mb_show_sm : mb_show_sm;
	mb_use_wire = (key == KEY_V) && pressed ? !mb_use_wire : mb_use_wire;
	mi_ss_type = (key == KEY_1) && pressed ? 0 : mi_ss_type;
	mi_ss_type = (key == KEY_2) && pressed ? 1 : mi_ss_type;
	mb_use_pcf = (key == KEY_H) && pressed ? !mb_use_pcf : mb_use_pcf;
	bool old_ulac = mb_use_ligth_as_camera;
	mb_use_ligth_as_camera = (key == KEY_G) && pressed ? !mb_use_ligth_as_camera : mb_use_ligth_as_camera;
	if (old_ulac != mb_use_ligth_as_camera) {
		if (mb_use_ligth_as_camera) {
			m_old_eye = m_eye;
			m_old_lookAt = m_lookAt;
			m_old_right = m_right;
			m_old_up = m_up;
			mat4 inve_light_view = !m_light_view;
			m_eye = vec3(inve_light_view.rows[0].w, inve_light_view.rows[1].w, inve_light_view.rows[2].w);
			m_lookAt = m_eye + m_light_view.rows[2].xyz();
			m_up = m_light_view.rows[1].xyz();
			m_right = m_light_view.rows[0].xyz();
			updateView();
		}
		else {
			m_eye = m_old_eye;
			m_lookAt = m_old_lookAt;
			m_right = m_old_right;
			m_up = m_old_up;
			updateView();
		}
	}
	return EmptyWithCameraApp::onKey(key, pressed);
}


void CSMApp::drawFrame()
{
	float col[4] = { 0.5f,0.5f,0.5f,1 };
	renderer->clear(true, true, col, 1.0f);

	//normal-shadow
	if (m_shadow_type == ST_NORMAL) {
		renderer->changeRenderTargets(&m_rt_sm_normal,1, m_rtd_sm_normal);
		float col[4] = { 1,1,0,1};
		renderer->clear(true, true, (float*)col, 1.0f);
		renderer->reset();
		renderer->setRasterizerState(cullBack);
		renderer->setShader(shadow_shd);
		for (int i = 0; i < 50; ++i) {
			renderer->setShaderConstant4x4f("worldViewProj", m_light_mat *  m_cubeMats[i]);
			renderer->apply();
			m_cube->draw(renderer);
		}
		renderer->setShaderConstant4x4f("worldViewProj", m_light_mat);
		renderer->apply();
		m_panel->draw(renderer);
		renderer->setShaderConstant4x4f("worldViewProj", m_light_mat * translate(-300, 0, 0) * scale(0.1, 0.1, 0.1));
		renderer->apply();
		for (uint i = 0; i < m_obj->getBatchCount(); i++) {
			m_obj->drawBatch(renderer, i);
		}
	}

	renderer->changeToMainFramebuffer();
	mat4 camera = m_cameraProj * m_cameraView;
	camera = mb_use_ligth_as_camera ? m_light_mat : camera;
	renderer->reset();
	if(mb_use_wire)
		renderer->setRasterizerState(m_rs_wire);
	else
		renderer->setRasterizerState(cullBack);
	renderer->setShader(m_camera_shd);
	renderer->setShaderConstant4x4f("lightViewMatrix", m_light_mat);
	renderer->setTexture("shadowMap", m_rt_sm_normal);
	renderer->setSamplerState("tempSS", mi_ss_type == 0 ? m_ss_sm : m_ss_asio_sm);
	renderer->setSamplerState("smFliter", m_ss_sm);
	renderer->setShaderConstant1i("fliterType", mb_use_pcf ? 0 : 1);
	for (int i = 0; i < 50; ++i) {
		renderer->setShaderConstant4x4f("world", m_cubeMats[i]);
		renderer->setShaderConstant4x4f("worldViewProj", camera * m_cubeMats[i]);
		renderer->apply();
		m_cube->draw(renderer);
	}
	renderer->setShaderConstant4x4f("world", identity4());
	renderer->setShaderConstant4x4f("worldViewProj", camera);
	renderer->apply();
	m_panel->draw(renderer);
	renderer->setShaderConstant4x4f("worldViewProj", camera * translate(-300, 0, 0) * scale(0.1,0.1,0.1));
	renderer->setShaderConstant4x4f("world", translate(-300, 0, 0) * scale(0.1, 0.1, 0.1));
	renderer->apply();
	for (uint i = 0; i < m_obj->getBatchCount(); i++) {
		m_obj->drawBatch(renderer, i);
	}

#if 1
	{//draw scene aabb
		renderer->reset();
		renderer->setShader(m_line_shd);
		renderer->setShaderConstant4x4f("worldViewProj", camera);
		renderer->setShaderConstant3f("lineCol", vec3(0, 0, 0));
		renderer->apply();
		renderer->changeVertexFormat(m_aabb_vf);
		renderer->changeVertexBuffer(0, m_aabb_vb);
		renderer->changeIndexBuffer(m_aabb_ib);
		renderer->drawElements(PRIM_LINES, 0, 24, 0, 8);
	}
#endif
	if (mb_show_sm) {
		renderer->reset();
		renderer->setRasterizerState(cullNone);
		renderer->setDepthState(noDepthTest);
		renderer->setShader(m_show_sm_shd);
		renderer->setShaderConstant4x4f("worldViewProj", m_show_sm_mat);
		renderer->setTexture("shadowMap", m_rt_sm_normal);
		renderer->setSamplerState("smFliter", linearClamp);
		renderer->apply();
		m_show_sm_panel->draw(renderer);
	}

}