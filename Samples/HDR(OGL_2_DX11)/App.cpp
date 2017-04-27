
#include "App.h"
#include "Util/TexturePacker.h"
#include "Util/Model.h"

#define ProjectDir	"/Samples/HDR(OGL_2_DX11)"
#include "InitResDir.inl"

BaseApp *app = new App();

void App::resetCamera(){
	camPos = vec3(610, -350, -820);
	wx = -0.23f;
	wy =  0.19f;
}

void App::moveCamera(const vec3 &dir){
	vec3 newPos = camPos + dir * (speed * frameTime);

	vec3 point;
	const BTri *tri;
	if (bsp.intersects(camPos, newPos, &point, &tri)){
		newPos = point + tri->plane.xyz();
	}
	bsp.pushSphere(newPos, 20);

	camPos = newPos;
}

bool App::onKey(const uint key, const bool pressed){
	if (pressed){
		if (key == KEY_F5){
			if (nodes.getCount()){
				freeFly->setChecked(!freeFly->isChecked());
			}
		}
/*
		else if (key == KEY_INSERT){
			Node node;
			node.pos = camPos;
			node.wx = wx;
			node.wy = wy;
			node.exposure = exposureControl->getValue();
			node.blurStrength = blurControl->getValue();
			nodes.add(node);
		} else if (key == KEY_BACKSPACE){
			if (nodes.getCount()) nodes.fastRemove(nodes.getCount() - 1);
		}
*/
	}

	if (key >= KEY_1 && key <= KEY_4) {
		HDR_mode->selectItem(key - KEY_1);
	}

	return D3D11App::onKey(key, pressed);
}

bool App::init(){

	initWorkDir();

	FILE *file = fopen(ShaderDir("/flypath.pth"), "rb");
	if (file){
		uint n = 0;
		fread(&n, sizeof(n), 1, file);
		if (n){
			nodes.setCount(n);
			fread(nodes.getArray(), sizeof(Node), n, file);
		}
		fclose(file);
	}

	lightDir = normalize(vec3(-0.13f, 0.68f, 1));

	map = new Model();
	if (!map->loadObj(ResDir("/Models/Castle.obj"))) 
		return false;

	map->computeTangentSpace(true);

	TexturePacker texPacker;

	uint nIndices = map->getIndexCount();
	vec3 *src = (vec3 *) map->getStream(0).vertices;
	uint *inds = map->getStream(0).indices;

	for (uint i = 0; i < nIndices; i += 6){
		vec3 v0 = src[inds[i]];
		vec3 v1 = src[inds[i + 1]];
		vec3 v3 = src[inds[i + 5]];

		float w = length(v1 - v0) / 32;
		float h = length(v3 - v0) / 32;

		if (w < 8) w = 8;
		if (h < 8) h = 8;

		uint tw = (int) w;
		uint th = (int) h;

		tw = (tw + 4) & ~7;
		th = (th + 4) & ~7;

		texPacker.addRectangle(tw, th);
	}

	uint tw = 256;
	uint th = 256;

	if (!texPacker.assignCoords(&tw, &th)){
		ErrorMsg("Lightmap too small");
		return false;
	}

	tw = getUpperPowerOfTwo(tw);
	th = getUpperPowerOfTwo(th);


	uint nLmCoords = (nIndices / 6) * 4;
	vec2 *lmCoords = new vec2[nLmCoords];
	uint *lmIndices = new uint[nIndices];

	uint index = 0;
	for (uint i = 0; i < nIndices; i += 6){
		TextureRectangle *rect = texPacker.getRectangle(index);

		float x0 = float(rect->x + 0.5f) / tw;
		float y0 = float(rect->y + 0.5f) / th;
		float x1 = float(rect->x + rect->width  - 0.5f) / tw;
		float y1 = float(rect->y + rect->height - 0.5f) / th;

		lmCoords[4 * index + 0] = vec2(x0, y0);
		lmCoords[4 * index + 1] = vec2(x1, y0);
		lmCoords[4 * index + 2] = vec2(x1, y1);
		lmCoords[4 * index + 3] = vec2(x0, y1);

		lmIndices[i + 0] = 4 * index;
		lmIndices[i + 1] = 4 * index + 1;
		lmIndices[i + 2] = 4 * index + 2;
		lmIndices[i + 3] = 4 * index;
		lmIndices[i + 4] = 4 * index + 2;
		lmIndices[i + 5] = 4 * index + 3;

		vec3 v0 = src[inds[i]];
		vec3 v1 = src[inds[i + 1]];
		vec3 v2 = src[inds[i + 2]];
		vec3 v3 = src[inds[i + 5]];

		bsp.addTriangle(v0, v1, v2);
		bsp.addTriangle(v0, v2, v3);

		index++;
	}
	bsp.build();

	map->addStream(TYPE_TEXCOORD, 2, nLmCoords, (float *) lmCoords, lmIndices, true);

	if (!lightMapImage.loadDDS(ShaderDir("/LightMap.dds"))){

#ifdef COMPUTE_LIGHTMAP

		/*****	Lightmap generation code *****/
		SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

		vec3 lightDirs[L_SAMPLE_COUNT];
		vec3 occlusionDirs[O_SAMPLE_COUNT];

		lightDirs[0] = lightDir;
		vec3 v;
		for (uint x = 1; x < L_SAMPLE_COUNT; x++){
			do {
				v = vec3(float(rand()), float(rand()), float(rand())) * (2.0f / RAND_MAX) - 1.0f;
			} while (dot(v, v) > 1);
			lightDirs[x] = normalize(lightDir + 0.09f * v);
		}

		for (uint x = 0; x < O_SAMPLE_COUNT; x++){
			do {
				v = vec3(float(rand()), float(rand()), float(rand())) * (2.0f / RAND_MAX) - 1.0f;
			} while (dot(v, v) > 1);
			occlusionDirs[x] = normalize(v);
		}

		
		ubyte *lMap = lightMapImage.create(FORMAT_IA8, tw, th, 1, 1);
		memset(lMap, 0x7F, 2 * tw * th);


		index = 0;
		for (uint i = 0; i < nIndices; i += 6){
			TextureRectangle *rect = texPacker.getRectangle(index);

			vec3 dirS = (src[inds[i + 1]] - src[inds[i]]);
			vec3 dirT = (src[inds[i + 5]] - src[inds[i]]);
			vec3 pos = src[inds[i]];
			vec3 normal = cross(dirS, dirT);

			for (int t = 0; t < int(rect->height); t++){
				for (int s = 0; s < int(rect->width); s++){
					vec3 samplePos = pos + saturate((float(s) / (rect->width - 1))) * dirS + saturate((float(t) / (rect->height - 1))) * dirT;
					bsp.pushSphere(samplePos, 5);

					float light = 0.0f;
					for (uint k = 0; k < L_SAMPLE_COUNT; k++){
						if (dot(lightDirs[k], normal) > 0){
							if (!bsp.intersects(samplePos, samplePos + 10000 * lightDirs[k])) light += 1.0f / L_SAMPLE_COUNT;
						}
					}

					lMap[2 * ((rect->y + t) * tw + (rect->x + s))] = (ubyte) (255.0f * light);
				}
			}
			index++;
		}

		index = 0;
		for (uint i = 0; i < nIndices; i += 6){
			TextureRectangle *rect = texPacker.getRectangle(index);

			vec3 dirS = (src[inds[i + 1]] - src[inds[i]]);
			vec3 dirT = (src[inds[i + 5]] - src[inds[i]]);
			vec3 pos = src[inds[i]];
			vec3 normal = normalize(cross(dirS, dirT));

			for (int t = 0; t < int(rect->height); t++){
				for (int s = 0; s < int(rect->width); s++){
					vec3 samplePos = pos + (saturate((float(s) / (rect->width - 1))) * 0.998f + 0.001f) * dirS + (saturate((float(t) / (rect->height - 1))) * 0.998f + 0.001f) * dirT + normal;
					bsp.pushSphere(samplePos, 3);

					float ambientOcclusion = 0.0f;
					vec3 point;
					const BTri *tri;
					for (uint k = 0; k < O_SAMPLE_COUNT; k++){
						float diffuse = dot(occlusionDirs[k], normal);
						if (diffuse <= 0) continue;

						// Quite fake ambient occlusion, but good enough
						if (!bsp.intersects(samplePos, samplePos + 10000 * occlusionDirs[k], &point, &tri)){
							float occ = 0.5f + 0.5f * dot(occlusionDirs[k], lightDir);
							occ *= occ;
							occ *= occ;
							ambientOcclusion += occ * (1.0f / O_SAMPLE_COUNT);
						}
					}

					lMap[2 * ((rect->y + t) * tw + (rect->x + s)) + 1] = (ubyte) (255.0f * saturate(5.3f * ambientOcclusion) + 0.5f);
				}
			}

			index++;
		}

		lightMapImage.saveDDS("LightMap.dds");
#else
		ErrorMsg("Couldn't open LightMap.dds");
		return false;
#endif
	}

	map->cleanUp();
	//map->changeAllGeneric(true);

	// Init GUI components
	int tab = configDialog->addTab("HDR");

	configDialog->addWidget(tab, new Label(0, 0,  128, 36, "Exposure"));
	configDialog->addWidget(tab, new Label(0, 80, 128, 36, "Blur strength"));
	configDialog->addWidget(tab, exposureControl = new Slider(0,  40, 200, 24, -1, 1, 0.4f));
	configDialog->addWidget(tab, blurControl     = new Slider(0, 120, 200, 24, 0.85f, 2, 1.0f));
	configDialog->addWidget(tab, freeFly = new CheckBox(0, 160, 200, 36, "Free fly", false));
	configDialog->addWidget(tab, new Label(0, 200, 128, 36, "HDR mode"));
	HDR_mode = new DropDownList(0, 240, 360, 30);
	HDR_mode->addItem("Reinhard");
	HDR_mode->addItem("cry engine");
	HDR_mode->addItem("filmic");
	HDR_mode->addItem("aces");
	HDR_mode->selectItem(0);
	configDialog->addWidget(tab, HDR_mode);
	
	return true;
}

void App::exit(){
/*
	FILE *file = fopen("flypath.pth", "wb");
	if (file){
		uint n = nodes.getCount();
		fwrite(&n, sizeof(n), 1, file);
		fwrite(nodes.getArray(), sizeof(Node), n, file);
		fclose(file);
	}
*/
	delete map;
}

bool App::initAPI(){
	// Disable multisampling as it's not supported for FP16
	int saveAA = antiAliasSamples;
	antiAliasSamples = 0;

	antiAlias->setEnabled(false);

	bool result = D3D11App::initAPI();

	antiAlias->selectItem(0);
	// Restore to avoid overwriting user settings
	antiAliasSamples = saveAA;

	return result;
}

bool App::load(){
	// Shaders
	const char *attribs[] = { NULL, "textureCoord", "tangent", "binormal", "normal", "lightMapCoord" };
	if ((lighting = renderer->addShader(ShaderDir("/lighting.shd"), attribs, elementsOf(attribs))) == SHADER_NONE) return false;
	if ((skyBox = renderer->addShader(ShaderDir("/skybox.shd"))) == SHADER_NONE) return false;
	if ((convert = renderer->addShader(ShaderDir("/convert.shd"))) == SHADER_NONE) return false;
	if ((blur = renderer->addShader(ShaderDir("/blur.shd"))) == SHADER_NONE) return false;
	if ((toneMapping = renderer->addShader(ShaderDir("/toneMapping.shd"))) == SHADER_NONE) return false;

	int w = width;
	int h = height;
	
	size = vec2(1.0f / w, 1.0f / h);

	if ((nearestClamp = renderer->addSamplerState(NEAREST, CLAMP, CLAMP, CLAMP)) == SS_NONE) return false;
	if ((trilinearAniso = renderer->addSamplerState(TRILINEAR_ANISO, WRAP, WRAP, WRAP)) == SS_NONE) return false;

	// Render targets
	if ((rtColor = renderer->addRenderTarget(w, h, FORMAT_RGBA16F, nearestClamp)) == TEXTURE_NONE) return false;

	if ((rtDepth = renderer->addRenderDepth(w, h, 16)) == TEXTURE_NONE) return false;

	if ((rtBlur = renderer->addRenderTarget(w / 2, h / 2, FORMAT_RGBA16, linearClamp)) == TEXTURE_NONE) return false;
	if ((rtBlur2 = renderer->addRenderTarget(w / 4, h / 4, FORMAT_RGBA16, linearClamp)) == TEXTURE_NONE) return false;
	if ((rtBlur3 = renderer->addRenderTarget(w / 4, h / 4, FORMAT_RGBA16, linearClamp)) == TEXTURE_NONE) return false;
	
	// Textures
	if ((base[0] = renderer->addTexture(ResDir("/Textures/floor_wood_3.dds"),  true, trilinearAniso)) == TEXTURE_NONE) return false;
	if ((base[1] = renderer->addTexture(ResDir("/Textures/brick01.dds"),       true, trilinearAniso)) == TEXTURE_NONE) return false;
	if ((base[2] = renderer->addTexture(ResDir("/Textures/StoneWall_2-2.dds"), true, trilinearAniso)) == TEXTURE_NONE) return false;

	if ((bump[0] = renderer->addNormalMap(ResDir("/Textures/floor_wood_3Bump.png"),  FORMAT_RGBA8, true, trilinearAniso)) == TEXTURE_NONE) return false;
	if ((bump[1] = renderer->addNormalMap(ResDir("/Textures/brick01Bump.png"),       FORMAT_RGBA8, true, trilinearAniso)) == TEXTURE_NONE) return false;
	if ((bump[2] = renderer->addNormalMap(ResDir("/Textures/StoneWall_2-2Bump.png"), FORMAT_RGBA8, true, trilinearAniso)) == TEXTURE_NONE) return false;


	if ((lightMap = renderer->addTexture(lightMapImage, false, linearClamp)) == TEXTURE_NONE) return false;


	// HDR texture in RGBE format, with RGB in DXT1 and E as L16
	if ((envRGB = renderer->addTexture(ResDir("/Textures/CubeMaps/MoonLight/RGB.dds"), false, linearClamp)) == TEXTURE_NONE) return false;
	if ((envExp = renderer->addTexture(ResDir("/Textures/CubeMaps/MoonLight/Exp.dds"), false, linearClamp)) == TEXTURE_NONE) return false;

	// Bah! FP16 only goes to 65504 ... so I'm reducing range here to avoid INFs later in rendering
	scaleBias = vec2(22.3204f, -6.09161f);
	//scaleBias = vec2(22.09f, -6.09161f);

	if (!map->makeDrawable(renderer, true,lighting)) return false;

	//init quad

	vec3 verts[] = {
		vec3(-1,1,0),
		vec3(1,1,0),
		vec3(1,-1,0),
		vec3(-1,-1,0)
	};
	ushort indices[] = {
		0,1,2,
		0,2,3
	};
	m_quad_vb = renderer->addVertexBuffer(sizeof(verts), STATIC, verts);
	m_quad_ib = renderer->addIndexBuffer(sizeof(indices) / sizeof(ushort), sizeof(ushort), STATIC, indices);
	FormatDesc formats[] = {
		{ 0,TYPE_VERTEX,FORMAT_FLOAT,3 }
	};
	m_quad_vf = renderer->addVertexFormat(formats, 1, convert);

	return true;
}

void App::unload(){
	
}

void App::drawEnvironment(const mat4 &invMvp){
	renderer->reset();
	renderer->setShader(skyBox);
	renderer->setTexture("RGB", envRGB);
	renderer->setTexture("Exp", envExp);
	renderer->setSamplerState("Filter", linearClamp);
	renderer->setDepthState(noDepthWrite);
	renderer->setShaderConstant2f("scaleBias", scaleBias);
	renderer->setShaderConstant4x4f("inv_mvp", invMvp);
	renderer->apply();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(3, 0);
}

void App::drawQuad(){

	renderer->changeVertexFormat(m_quad_vf);
	renderer->changeVertexBuffer(0,m_quad_vb);
	renderer->changeIndexBuffer(m_quad_ib);
	renderer->drawElements(PRIM_TRIANGLES, 0, 6, 0, 4);

}

void fixAngles(float *s){
	// Let angles wrap around ...
	if (s[1] > s[0]){
		while (s[1] - s[0] > PI) s[1] -= 2 * PI;
	} else {
		while (s[0] - s[1] > PI) s[1] += 2 * PI;
	}
	
	if (s[2] > s[1]){
		while (s[2] - s[1] > PI) s[2] -= 2 * PI;
	} else {
		while (s[1] - s[2] > PI) s[2] += 2 * PI;
	}
	
	if (s[3] > s[2]){
		while (s[3] - s[2] > PI) s[3] -= 2 * PI;
	} else {
		while (s[2] - s[3] > PI) s[3] += 2 * PI;
	}
}

void App::drawFrame(){
	float exposure, blurStrength;
	
	if (freeFly->isChecked()){
		exposure = exposureControl->getValue();
		blurStrength = blurControl->getValue();
	} else {
		float ft = 0.5f * time;
		int t = (int) ft;
		float f = ft - t;

		int t0 = (t - 1 + nodes.getCount()) % nodes.getCount();
		int t1 =  t      % nodes.getCount();
		int t2 = (t + 1) % nodes.getCount();
		int t3 = (t + 2) % nodes.getCount();

		camPos = cerp(nodes[t0].pos, nodes[t1].pos, nodes[t2].pos, nodes[t3].pos, f);

		float xAngles[] = { nodes[t0].wx, nodes[t1].wx, nodes[t2].wx, nodes[t3].wx };
		float yAngles[] = { nodes[t0].wy, nodes[t1].wy, nodes[t2].wy, nodes[t3].wy };
		fixAngles(xAngles);
		fixAngles(yAngles);

		wx = cerp(xAngles[0], xAngles[1], xAngles[2], xAngles[3], f);
		wy = cerp(yAngles[0], yAngles[1], yAngles[2], yAngles[3], f);

		exposure = cerp(nodes[t0].exposure, nodes[t1].exposure, nodes[t2].exposure, nodes[t3].exposure, f);
		blurStrength = cerp(nodes[t0].blurStrength, nodes[t1].blurStrength, nodes[t2].blurStrength, nodes[t3].blurStrength, f);
	}
	exposure = powf(16.0f, exposure);

	mat4 projection = perspectiveMatrixX(1.5f, width, height, 10, 6000);
	mat4 modelview = rotateXY(-wx, -wy) * translate(-camPos);
	mat4 envMV = rotateXY(-wx, -0.7f - wy);
	mat4 mvp = projection * modelview;

	float col[] = { 0,0,0,1 };
	renderer->clear(true, true, col, 1.0f);

	TextureID bufferRTs[] = {rtColor};
	
	renderer->changeRenderTargets(bufferRTs, 1, rtDepth);
	{
		float col[] = {0,0,0,1};
		renderer->clear(true, true, col, 1.0f);
		const float plx[] = { 0.01f, 0.03f, 0.05f };
		const float gloss[] = { 0.7f, 0.5f, 0.4f };

		for (uint i = 0; i < map->getBatchCount(); i++) {
			renderer->reset();
			renderer->setRasterizerState(cullBack);
			renderer->setShader(lighting);
			renderer->setShaderConstant4x4f("mvp", mvp);
			renderer->setTexture("Base", base[i]);
			renderer->setTexture("Bump", bump[i]);
			renderer->setSamplerState("Fliter", trilinearAniso);
			renderer->setTexture("LightMap", lightMap);
			renderer->setSamplerState("LMFliter", linearClamp);
			renderer->setShaderConstant3f("lightDir", lightDir);
			renderer->setShaderConstant3f("camPos", camPos);
			renderer->setShaderConstant2f("plx", vec2(2, -1) * plx[i]);
			renderer->setShaderConstant1f("gloss", gloss[i]);
			renderer->apply();
			map->drawBatch(renderer, i);
		}

		drawEnvironment(!(projection * envMV));
	}
	

	renderer->changeRenderTargets(&rtBlur,1,-1);
		float range = 16.0f;
		renderer->reset();
		renderer->setShader(convert);
		renderer->setDepthState(noDepthTest);
		renderer->setTexture("Image", rtColor);
		renderer->setSamplerState("Fliter", nearestClamp);
		renderer->setShaderConstant2f("halfPixel", 0.5f * size);
		renderer->setShaderConstant1f("invRange", 0.25f / range);
		
		renderer->apply();
		drawQuad();

	renderer->changeRenderTarget(rtBlur2);

		renderer->reset();
		renderer->setShader(blur);
		renderer->setSamplerState("Fliter", linearClamp);
		renderer->setDepthState(noDepthTest);

		renderer->setTexture("Image", rtBlur);
		renderer->setShaderConstant2f("sample0", size * vec2(-0.75f,  0.25f));
		renderer->setShaderConstant2f("sample1", size * vec2( 0.25f,  0.75f));
		renderer->setShaderConstant2f("sample2", size * vec2( 0.75f, -0.25f));
		renderer->setShaderConstant2f("sample3", size * vec2(-0.25f, -0.75f));
		renderer->apply();

		drawQuad();
	
	renderer->changeRenderTarget(rtBlur3);

		renderer->setTexture("Image", rtBlur2);
		renderer->setShaderConstant2f("sample0", 2 * size * vec2(-0.75f, -0.25f));
		renderer->setShaderConstant2f("sample1", 2 * size * vec2(-0.25f,  0.75f));
		renderer->setShaderConstant2f("sample2", 2 * size * vec2( 0.75f,  0.25f));
		renderer->setShaderConstant2f("sample3", 2 * size * vec2( 0.25f, -0.75f));
		renderer->apply();

		drawQuad();

	renderer->changeRenderTarget(rtBlur2);
		
		renderer->setTexture("Image", rtBlur3);
		renderer->setShaderConstant2f("sample0", 4 * size * vec2(-0.75f,  0.25f));
		renderer->setShaderConstant2f("sample1", 4 * size * vec2( 0.25f,  0.75f));
		renderer->setShaderConstant2f("sample2", 4 * size * vec2( 0.75f, -0.25f));
		renderer->setShaderConstant2f("sample3", 4 * size * vec2(-0.25f, -0.75f));
		renderer->apply();

		drawQuad();

	renderer->changeRenderTarget(rtBlur3);

		renderer->setTexture("Image", rtBlur2);
		renderer->setShaderConstant2f("sample0", 8 * size * vec2(-0.75f, -0.25f));
		renderer->setShaderConstant2f("sample1", 8 * size * vec2(-0.25f,  0.75f));
		renderer->setShaderConstant2f("sample2", 8 * size * vec2( 0.75f,  0.25f));
		renderer->setShaderConstant2f("sample3", 8 * size * vec2( 0.25f, -0.75f));
		renderer->apply();

		drawQuad();

	int mode = HDR_mode->getSelectedItem();
	renderer->changeToMainFramebuffer();
		renderer->reset();
		renderer->setDepthState(noDepthTest);
		renderer->setShader(toneMapping);
		renderer->setTexture("Base", rtColor);
		renderer->setSamplerState("BaseFliter", nearestClamp);
		renderer->setTexture("Blur", rtBlur3);
		renderer->setSamplerState("BlurFliter", linearClamp);
		renderer->setShaderConstant1f("exposure", exposure);
		renderer->setShaderConstant1f("range", range);
		renderer->setShaderConstant1f("blurStrength", blurStrength);
		renderer->setShaderConstant1i("varyHDR", mode);
		renderer->apply();
	drawQuad();

}
