#include "App.h"

#define LocalDir	"/Samples/OrderIndependentTranslucency"
#include "InitResDir.inl"

BaseApp *app = new App();

void App::resetCamera(){
	camPos = vec3(-0.052f, 0.069f, 0.144f);
	wx = 0.35f;
	wy = -2.63f;

	speed = 0.2f;
}

void App::onSize(const int w, const int h){
	D3D10App::onSize(w, h);

	if (renderer){
		// Make sure render targets are the size of the window
		renderer->resizeRenderTarget(unsortedColorRT, w, h, 1, 1,1);
		renderer->resizeRenderTarget(unsortedDepthRT, w, h, 1, 1,1);
		renderer->resizeRenderTarget(unsortedDS,      w, h, 1, 1,1);
	}
}

bool App::init(){
	initWorkDir();
	// Load the geometry
	model = new Model();
	if (!model->loadObj(ResDir("/Models/Horse.obj"))){
		ErrorMsg("Couldn't load model file");
		return false;
	}

	model->flipComponents(0, 1, 2);
	model->flipComponents(1, 1, 2);

	return true;
}

void App::exit(){
	delete model;
}

bool App::initAPI(){
	// Override the user's MSAA settings
	return D3D10App::initAPI(DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_UNKNOWN, 1, NO_SETTING_CHANGE);
}

void App::exitAPI(){
	D3D10App::exitAPI();
}

bool App::load(){
	// Shaders
	if ((render = renderer->addShader(ResDir("/render.shd"))) == SHADER_NONE) return false;
	if ((sort = renderer->addShader(ResDir("/sort.shd"))) == SHADER_NONE) return false;
	if ((stencilClear = renderer->addShader(ResDir("/stencilClear.shd"))) == SHADER_NONE) return false;

	// Samplerstates
	if ((trilinearClamp = renderer->addSamplerState(TRILINEAR, CLAMP, CLAMP, CLAMP)) == SS_NONE) return false;

	// Rasterizerstates
	if ((noMSAA = renderer->addRasterizerState(CULL_NONE, SOLID, false, false)) == RS_NONE) return false;

	// Depthstates
	if ((stencilRoute = renderer->addDepthState(false, false, ALWAYS, true, 0xFF, EQUAL, DECR, DECR, DECR)) == BS_NONE) return false;
	if ((stencilSet   = renderer->addDepthState(false, false, ALWAYS, true, 0xFF, ALWAYS, REPLACE, REPLACE, REPLACE)) == BS_NONE) return false;

	// Skybox
	const char *fileNames[] = {
		ResDir("/Textures/CubeMaps/UnionSquare/posx.jpg"),
		ResDir("/Textures/CubeMaps/UnionSquare/negx.jpg"),
		ResDir("/Textures/CubeMaps/UnionSquare/posy.jpg"),
		ResDir("/Textures/CubeMaps/UnionSquare/negy.jpg"),
		ResDir("/Textures/CubeMaps/UnionSquare/posz.jpg"),
		ResDir("/Textures/CubeMaps/UnionSquare/negz.jpg"),
	};
	if ((env = renderer->addCubemap(fileNames, true, trilinearClamp)) == TEXTURE_NONE) return false;
	
	// Render targets 
	if ((unsortedColorRT = renderer->addRenderTarget(width, height, 1, 1, 1, FORMAT_RGBA8, 8)/*(width, height, 1, 1, FORMAT_RGBA8,   8, SS_NONE)*/) == TEXTURE_NONE) return false;
	if ((unsortedDepthRT = renderer->addRenderTarget(width, height, 1, 1, 1, FORMAT_R16, 8)/*(width, height, 1, 1, FORMAT_R16,     8, SS_NONE)*/) == TEXTURE_NONE) return false;
	if ((unsortedDS      = renderer->addRenderDepth (width, height, 1,    FORMAT_DEPTH24, 8, SS_NONE)) == TEXTURE_NONE) return false;

	// Upload map to vertex/index buffer
	if (!model->makeDrawable(renderer, true, render)) return false;

	return true;
}

void App::unload(){
}

void App::drawFrame(){
	float4x4 projection = toD3DProjection(perspectiveMatrixX(1.5f, width, height, 0.001f, 1.0f));
	float4x4 view = rotateXY(-wx, -wy);
	float4x4 invViewProj = !(projection * rotateXY(-wx, -wy));
	view.translate(-camPos);
	float4x4 viewProj = projection * view;

	// Clear render targets
	((Direct3D10Renderer *) renderer)->clearRenderTarget(unsortedColorRT, float4(0, 0, 0, 0));
	((Direct3D10Renderer *) renderer)->clearRenderTarget(unsortedDepthRT, float4(1, 1, 1, 1));

	TextureID renderTargets[] = { unsortedColorRT, unsortedDepthRT };
	renderer->changeRenderTargets(renderTargets, elementsOf(renderTargets), unsortedDS);

		// Setup stencil buffer
		renderer->reset();
		renderer->setRasterizerState(noMSAA);
		renderer->setShader(stencilClear);
		renderer->setDepthState(stencilSet, 1);
		renderer->setBlendState(BS_NONE, 1);
		renderer->apply();

		device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		for (int i = 0; i < 8; i++){
			renderer->changeBlendState(BS_NONE, 1 << i);
			renderer->changeDepthState(stencilSet, i);

			device->Draw(3, 0);
		}

		// Main pass
		renderer->reset();
		renderer->setRasterizerState(noMSAA);
		renderer->setShader(render);
		renderer->setShaderConstant4x4f("viewProj", viewProj);
		renderer->setShaderConstant3f("camPos", camPos);
		renderer->setTexture("Env", env);
		renderer->setSamplerState("filter", trilinearClamp);
		renderer->setDepthState(stencilRoute, 0);
		renderer->apply();

		model->draw(renderer);

	renderer->changeToMainFramebuffer();


	// Sorting pass
	renderer->reset();
	renderer->setRasterizerState(cullNone);
	renderer->setShader(sort);
	renderer->setShaderConstant4x4f("invViewProj", invViewProj);
	renderer->setTexture("Color", unsortedColorRT);
	renderer->setTexture("Depth", unsortedDepthRT);
	renderer->setTexture("Env", env);
	renderer->setSamplerState("filter", trilinearClamp);
	renderer->setDepthState(noDepthTest);
	renderer->apply();

	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	device->Draw(3, 0);
}
