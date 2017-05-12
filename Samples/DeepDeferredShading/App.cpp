
#include "App.h"


#define ProjectDir	"//Samples/DeepDeferredShading"
#include "InitResDir.inl"

BaseApp *app = new App();

void App::moveCamera(const float3 &dir){
	vec3 newPos = camPos + dir * (speed * frameTime);

	// Collision detection
	vec3 point;
	const BTri *tri;
	if (bsp.intersects(camPos, newPos, &point, &tri)){
		newPos = point + tri->plane.xyz();
	}
	bsp.pushSphere(newPos, 20);

	camPos = newPos;
}

void App::resetCamera(){
	camPos = vec3(490, 75, 540);
	wx = 0.45f;
	wy = 1.85f;
}

void App::onSize(const int w, const int h){
	D3D10App::onSize(w, h);

	if (renderer){
		// Make sure render targets are the size of the window
		renderer->resizeRenderTarget(baseRT,   w, h, 1, 1, LAYERS);
		renderer->resizeRenderTarget(normalRT, w, h, 1, 1, LAYERS);
		renderer->resizeRenderTarget(depthRT,  w, h, 1, 1, LAYERS);
	}
}

bool App::init(){


	// Load the geometry
	map = new Model();
	if (!map->loadT3d(ResDir("/Models/WindowedRoom.t3d"), true, true, false, 512.0f)) return false;
	map->reverseWinding();
	map->flipNormals();


	// Create BSP tree for collision detection
	uint nIndices = map->getIndexCount();
	vec3 *src = (vec3 *) map->getStream(0).vertices;
	uint *inds = map->getStream(0).indices;
	uint nTranslucent = map->getBatch(0).nIndices;

	for (uint i = 0; i < nIndices; i += 3){
		vec3 v0 = src[inds[i]];
		vec3 v1 = src[inds[i + 1]];
		vec3 v2 = src[inds[i + 2]];

		if (i < nTranslucent){
			// Make the glass thick for the collision detection
			vec3 n = normalize(cross(v1 - v0, v2 - v0));
			bsp.addTriangle(v0 + 15 * n, v1 + 15 * n, v2 + 15 * n);
			bsp.addTriangle(v0 - 15 * n, v2 - 15 * n, v1 - 15 * n);
		} else {
			bsp.addTriangle(v0, v1, v2);
		}
	}
	bsp.build();

	map->cleanUp();

	return true;
}

void App::exit(){
	delete map;
}

bool App::initAPI(){
	// Override the user's MSAA settings
	return D3D10App::initAPI(DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_UNKNOWN, 1, NO_SETTING_CHANGE);
}

void App::exitAPI(){
	D3D10App::exitAPI();
}

bool App::load(){

	initWorkDir(renderer);

	// Shaders
	if ((fillBuffers = renderer->addShader(ShaderDir("/fillBuffers.shd"))) == SHADER_NONE) return false;
	if ((preZ[0]     = renderer->addShader(ShaderDir("/preZ0.shd"))) == SHADER_NONE) return false;
	if ((preZ[1]     = renderer->addShader(ShaderDir("/preZ1.shd"))) == SHADER_NONE) return false;
	if ((ambient     = renderer->addShader(ShaderDir("/ambient.shd"),   DEFINE_STR(LAYERS))) == SHADER_NONE) return false;
	if ((lighting    = renderer->addShader(ShaderDir("/lighting.shd"),  DEFINE_STR(LAYERS))) == SHADER_NONE) return false;
	if ((lightSpot   = renderer->addShader(ShaderDir("/lightSpot.shd"), DEFINE_STR(LAYERS))) == SHADER_NONE) return false;
	if ((shadow      = renderer->addShader(ShaderDir("/shadow.shd"))) == SHADER_NONE) return false;

	// Samplerstates
	if ((trilinearAniso = renderer->addSamplerState(TRILINEAR_ANISO, WRAP, WRAP, WRAP, 0.0f, 8)) == SS_NONE) return false;
	if ((trilinearClamp = renderer->addSamplerState(TRILINEAR, CLAMP, CLAMP, CLAMP)) == SS_NONE) return false;
	if ((pointClamp = renderer->addSamplerState(NEAREST, CLAMP, CLAMP, CLAMP)) == SS_NONE) return false;
	if ((shadowFilter = renderer->addSamplerState(LINEAR, CLAMP, CLAMP, CLAMP, 0.0f, 1, LESS)) == SS_NONE) return false;

	//addRenderTarget(const int width, const int height, const int depth, const int mipMapCount, const int arraySize, const FORMAT format, const int msaaSamples = 1, const SamplerStateID samplerState = SS_NONE, uint flags = 0)
	// Main render targets					   /*(256, 256, 1, FORMAT_DEPTH16, 1, SS_NONE, CUBEMAP | SAMPLE_DEPTH)*/
	if ((shadowMap = renderer->addRenderDepth(256, 256, 1, FORMAT_DEPTH16, 1, SS_NONE, CUBEMAP | SAMPLE_DEPTH)) == TEXTURE_NONE) return false;
											  /*(width, height, 1, LAYERS, FORMAT_RGBA8,   1, SS_NONE, RENDER_SLICES)*/
	if ((baseRT    = renderer->addRenderTarget(width,height,1,1,LAYERS,FORMAT_RGBA8,1,SS_NONE,RENDER_SLICES)) == TEXTURE_NONE) return false;
	if ((normalRT  = renderer->addRenderTarget(width, height,1, 1, LAYERS, FORMAT_RGB10A2, 1, SS_NONE, RENDER_SLICES)) == TEXTURE_NONE) return false;
	if ((depthRT   = renderer->addRenderDepth (width, height,    LAYERS, FORMAT_DEPTH24, 1, SS_NONE, RENDER_SLICES | SAMPLE_SLICES | SAMPLE_DEPTH)) == TEXTURE_NONE) return false;

	// Textures
	if ((base[0] = renderer->addTexture(ResDir("/Textures/StainedGlass.dds"),        true, trilinearAniso)) == SHADER_NONE) return false;
	if ((bump[0] = renderer->addTexture(ResDir("/Textures/StainedGlassBump3Dc.dds"), true, trilinearAniso)) == SHADER_NONE) return false;
	if ((base[1] = renderer->addTexture(ResDir("/Textures/brick01.dds"),             true, trilinearAniso)) == SHADER_NONE) return false;
	if ((bump[1] = renderer->addTexture(ResDir("/Textures/brick01Bump3Dc.dds"),      true, trilinearAniso)) == SHADER_NONE) return false;
	if ((base[2] = renderer->addTexture(ResDir("/Textures/floor_wood_3.dds"),        true, trilinearAniso)) == SHADER_NONE) return false;
	if ((bump[2] = renderer->addTexture(ResDir("/Textures/floor_wood_3Bump3Dc.dds"), true, trilinearAniso)) == SHADER_NONE) return false;
	if ((base[3] = renderer->addTexture(ResDir("/Textures/floor_wood_4.dds"),        true, trilinearAniso)) == SHADER_NONE) return false;
	if ((bump[3] = renderer->addTexture(ResDir("/Textures/floor_wood_4Bump3Dc.dds"), true, trilinearAniso)) == SHADER_NONE) return false;
	if ((base[4] = renderer->addTexture(ResDir("/Textures/laying_rock7.dds"),        true, trilinearAniso)) == SHADER_NONE) return false;
	if ((bump[4] = renderer->addTexture(ResDir("/Textures/laying_rock7Bump3Dc.dds"), true, trilinearAniso)) == SHADER_NONE) return false;
	if ((base[5] = renderer->addTexture(ResDir("/Textures/stone08.dds"),             true, trilinearAniso)) == SHADER_NONE) return false;
	if ((bump[5] = renderer->addTexture(ResDir("/Textures/stone08Bump3Dc.dds"),      true, trilinearAniso)) == SHADER_NONE) return false;

	if ((lightTex = renderer->addTexture(ResDir("/Textures/Particle.tga"), true, trilinearClamp)) == SHADER_NONE) return false;

	// Blendstates
	if ((blendAdd = renderer->addBlendState(ONE, ONE)) == BS_NONE) return false;

	// Depthstates
	if ((depthEqual = renderer->addDepthState(true, false, EQUAL)) == BS_NONE) return false;

	// Upload map to vertex/index buffer
	if (!map->makeDrawable(renderer, true, fillBuffers)) return false;

	return true;
}

void App::unload(){
}

void App::drawFrame(){
	const float fov = 1.5f;

	// Compute scene matrices
	float4x4 projection = toD3DProjection(perspectiveMatrixX(fov, width, height, 12, 5000));
	float4x4 view = rotateXY(-wx, -wy);
	view.translate(-camPos);
	float4x4 viewProj = projection * view;
	// Pre-scale-bias the matrix so we can use the texCoord directly in [0..1] range instead of [-1..1] clip-space range
	float4x4 invViewProj = (!viewProj) * (translate(-1.0f, 1.0f, 0.0f) * scale(2.0f, -2.0f, 1.0f));

	// Clear all slices at once
	((Direct3D10Renderer *) renderer)->clearDepthTarget(depthRT, 1.0f);

	/*
		Fill the different laters of the depth buffer first. This will act as a pre-z pass,
		which is neccesary because the bottleneck is filling the buffers anyway, but we'll
		also need the depth later on. To extract each layer depth peeling is used, so the
		previous depth layer is input into the next stage so we can discard pixels that are
		at or in front of the depth in the previous pass.
	*/
	for (int layer = 0; layer < LAYERS; layer++){
		renderer->changeRenderTargets(NULL, 0, depthRT, layer);

		renderer->reset();
		renderer->setRasterizerState(cullNone);
		if (layer == 0){
			// First layer is the front pixels
			renderer->setShader(preZ[0]);
		} else {
			// Depth peel the remaining layers
			renderer->setShader(preZ[1]);
			renderer->setTextureSlice("Depth", depthRT, layer - 1);
			renderer->setShaderConstant2f("invSize", float2(1.0f / width, 1.0f / height));
			renderer->setSamplerState("filter", pointClamp);
		}
		renderer->setShaderConstant4x4f("viewProj", viewProj);
		renderer->apply();

		for (uint i = 0; i < 6; i++){
			// Use back-face culling except for the first batch which is the double-sided translucent polygons
			if (i == 1) renderer->changeRasterizerState(cullBack);

			map->drawBatch(renderer, i);
		}
	}

	/*
		Main scene pass. This is where the buffers are filled base and normal data for the later deferred passes.
		We use EQUAL for the depth test to render only the current layer into each array slice.
	*/
	for (int layer = 0; layer < LAYERS; layer++){
		TextureID bufferRTs[] = { baseRT, normalRT };
		int slices[] = { layer, layer };
		renderer->changeRenderTargets(bufferRTs, elementsOf(bufferRTs), depthRT, layer, slices);

		renderer->reset();
		renderer->setRasterizerState(cullNone);
		renderer->setShader(fillBuffers);
		renderer->setShaderConstant4x4f("viewProj", viewProj);
		renderer->setShaderConstant3f("camPos", camPos);
		renderer->setSamplerState("baseFilter", trilinearAniso);
		renderer->setDepthState(depthEqual);
		renderer->apply();


		for (uint i = 0; i < 6; i++){
			// Use back-face culling except for the first batch which is the double-sided translucent polygons
			if (i == 1) renderer->changeRasterizerState(cullBack);

			renderer->setTexture("Base", base[i]);
			renderer->setTexture("Bump", bump[i]);
			renderer->applyTextures();

			map->drawBatch(renderer, i);
		}
	}

	renderer->changeToMainFramebuffer();


	/*
		Deferred ambient pass
	*/
	renderer->reset();
	renderer->setRasterizerState(cullNone);
	renderer->setShader(ambient);
	renderer->setTexture("Base", baseRT);
	renderer->setSamplerState("filter", pointClamp);
	renderer->setDepthState(noDepthTest);
	renderer->apply();

	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	device->Draw(3, 0);


	/*
		Deferred lighting pass
	*/
	float4 lights[LAYERS] = {
		float4(800 * cosf(0.57f * time), 0, 700 * sinf(2.0f * 0.57f * time), 600),
		float4(700 * sinf(time), -250,  700 * sinf(time + 0.3f), 500),
		float4(700 * cosf(time), -350, -700 * cosf(time + 0.3f), 500),
	};

	float ex = tanf(fov * 0.5f);
	float ey = ex * height / width;

	for (int i = 0; i < LAYERS; i++){
		float zNear = 1.0f;
		float zFar = lights[i].w;
		float bias = -0.0001f;

		/*
			Compute cubical shadow map for current light.
			This is done for all cube faces in a single pass using the GS.
		*/
		renderer->changeRenderTargets(NULL, 0, shadowMap);
			renderer->clear(false, true, NULL, 1.0f);

			// Compute view projection matrices for the faces of the cubemap
			float4x4 viewProjArray[6];
			float4x4 proj = cubeProjectionMatrixD3D(zNear, zFar);
			for (uint k = 0; k < 6; k++){
				viewProjArray[k] = proj * cubeViewMatrix(k) * translate(-lights[i].xyz());
			}

			renderer->reset();
			renderer->setRasterizerState(cullBack);
			renderer->setShader(shadow);
			renderer->setShaderConstantArray4x4f("viewProjArray", viewProjArray, 6);
			renderer->apply();

			// Skip first batch (translucent polygons)
			for (uint k = 1; k < 6; k++){
				map->drawBatch(renderer, k);
			}

		renderer->changeToMainFramebuffer();

		/*
			Compute the lighting for this light.
		*/
		renderer->reset();
		renderer->setRasterizerState(cullNone);
		renderer->setShader(lighting);
		renderer->setShaderConstant4x4f("view", view);
		renderer->setShaderConstant4x4f("invViewProj", invViewProj);
		renderer->setShaderConstant3f("camPos", camPos);
		renderer->setShaderConstant3f("lightPos", lights[i].xyz());
		renderer->setShaderConstant1f("lightRadius", lights[i].w);
		renderer->setShaderConstant1f("invLightRadiusSqr", 1.0f / (lights[i].w * lights[i].w));
		renderer->setShaderConstant1f("ex", 0.5f / ex);
		renderer->setShaderConstant1f("ey", 0.5f / ey);
		renderer->setShaderConstant2f("zw", projection.rows[2].zw());
		renderer->setShaderConstant2f("nf", float2(zFar * zNear / (zNear - zFar), zFar / (zFar - zNear) + bias));
		renderer->setTexture("Base", baseRT);
		renderer->setTexture("Normal", normalRT);
		renderer->setTexture("Depth", depthRT);
		renderer->setSamplerState("filter", pointClamp);
		renderer->setTexture("Shadow", shadowMap);
		renderer->setSamplerState("shadowFilter", shadowFilter);
		renderer->setDepthState(noDepthTest);
		renderer->setBlendState(blendAdd);
		renderer->apply();

		device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
		device->Draw(1, 0);
	}

	/*
		Light spot pass.
	*/
	renderer->reset();
	renderer->setRasterizerState(cullNone);
	renderer->setShader(lightSpot);
	renderer->setShaderConstant4x4f("viewProj", viewProj);
	renderer->setShaderConstant3f("dx", view.rows[0].xyz() * 20.0f);
	renderer->setShaderConstant3f("dy", view.rows[1].xyz() * 20.0f);
	renderer->setTexture("Light", lightTex);
	renderer->setTexture("Base", baseRT);
	renderer->setTexture("Depth", depthRT);
	renderer->setSamplerState("lightFilter", trilinearClamp);
	renderer->setDepthState(noDepthTest);
	renderer->setBlendState(blendAdd);
	renderer->apply();

	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
	for (int i = 0; i < elementsOf(lights); i++){
		renderer->setShaderConstant3f("light", lights[i].xyz());
		renderer->applyConstants();

		device->Draw(1, 0);
	}
}
