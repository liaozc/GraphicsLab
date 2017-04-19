#include "App.h"

#define ProjectDir	"/Samples/MyExample_005_OIT"
#include "InitResDir.inl"

BaseApp *app = new OITApp();

bool OITApp::initAPI() {
	// Override the user's MSAA settings
	return D3D11App::initAPI(D3D11,DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_UNKNOWN, 1, NO_SETTING_CHANGE);
}

void OITApp::exitAPI() {
	D3D11App::exitAPI();
}

bool OITApp::init()
{
	initWorkDir();
	return true;
}

bool OITApp::load()
{
	m_render_shd = renderer->addShader(ShaderDir("/render.shd"));
	m_sort_shd = renderer->addShader(ShaderDir("/sort.shd"));
	m_stencil_clear_shd = renderer->addShader(ShaderDir("/stencilClear.shd"));

	// Rasterizerstates
	m_no_msaa = renderer->addRasterizerState(CULL_NONE, SOLID, false, false);
	// Depthstates
	m_stencil_route = renderer->addDepthState(false, false, ALWAYS, true, 0xFF, EQUAL, DECR, DECR, DECR);
	m_stencil_set = renderer->addDepthState(false, false, ALWAYS, true, 0xFF, ALWAYS, REPLACE, REPLACE, REPLACE);

	m_unsorted_color_id = renderer->addRenderTarget(width, height, 1, 1, 1, FORMAT_RGBA8, 8);
	m_unsorted_depth_id = renderer->addRenderTarget(width, height, 1, 1, 1, FORMAT_R16, 8);
	m_unsorted_ds_id = renderer->addRenderDepth(width, height, 1, FORMAT_DEPTH24, 8, SS_NONE);
	
	vec3 quad[] = {
		vec3(-12,12,0),
		vec3(-12,-12,0),
		vec3(12,-12,0),
		vec3(12,12,0)
	};
	ushort indices[] = {
		0,2,1,
		0,3,2
	};
	
	m_quad_vb = renderer->addVertexBuffer(sizeof(quad), STATIC, quad);
	m_quad_ib = renderer->addIndexBuffer(sizeof(indices) / sizeof(ushort), sizeof(ushort), STATIC, indices);
	FormatDesc vfdesc[] = {
		{ 0,TYPE_VERTEX,FORMAT_FLOAT,3 },
	};
	m_quad_vf = renderer->addVertexFormat(vfdesc, 1, m_render_shd);

	RECT rect;
	GetClientRect(hwnd, &rect);
	m_ratio = float(rect.bottom - rect.top) / float(rect.right - rect.left);

	return true;
}

void OITApp::updateFrame()
{
	float4x4 world = rotateY(time * 0.5f); 
	vec3 eye(0, 0, -25);
	vec3 lookAt(0, 0, 0);
	vec3 up(0, 1, 0);
	mat4 view = makeViewMatrixD3D(eye, lookAt, up);
	mat4 proj = perspectiveMatrix(PI / 2, m_ratio, 1, 100);
	m_quad0_mvp = proj * view * world;

	world = rotateY(time * 0.5f + PI*0.25f);
	m_quad1_mvp = proj * view * world;

	world = rotateY(time * 0.5f + PI * 0.5f);
	m_quad2_mvp = proj * view * world;

	world = rotateY(time * 0.5f + PI * 0.75f);
	m_quad3_mvp = proj * view * world;
}

void OITApp::drawFrame()
{
	updateFrame();
	// Clear render targets
	((Direct3D11Renderer *)renderer)->clearRenderTarget(m_unsorted_color_id, float4(0, 0, 0, 0));
	((Direct3D11Renderer *)renderer)->clearRenderTarget(m_unsorted_depth_id, float4(1, 1, 1, 1));

	TextureID renderTargets[] = { m_unsorted_color_id, m_unsorted_depth_id };

	renderer->changeRenderTargets(renderTargets, elementsOf(renderTargets), m_unsorted_ds_id);
		// Setup stencil buffer
		renderer->reset();
		renderer->setRasterizerState(m_no_msaa);
		renderer->setShader(m_stencil_clear_shd);
		renderer->setDepthState(m_stencil_set, 1);
		renderer->setBlendState(BS_NONE, 1);
		renderer->apply();

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		for (int i = 0; i < 8; i++) {
			renderer->changeBlendState(BS_NONE, 1 << i);
			renderer->changeDepthState(m_stencil_set, i);
			context->Draw(3, 0);
		}
		{//main pass
			renderer->reset();
			renderer->setRasterizerState(m_no_msaa);
			renderer->setShader(m_render_shd);
			renderer->setDepthState(m_stencil_route, 0);

			renderer->setShaderConstant4x4f("worldViewProj", m_quad0_mvp);
			renderer->setShaderConstant4f("color", float4(1, 0, 0, 0.5f));
			renderer->apply();
			renderer->changeVertexFormat(m_quad_vf);
			renderer->changeVertexBuffer(0, m_quad_vb);
			renderer->changeIndexBuffer(m_quad_ib);
			renderer->drawElements(PRIM_TRIANGLES, 0, 6, 0, 4);

			renderer->setShaderConstant4x4f("worldViewProj", m_quad1_mvp);
			renderer->setShaderConstant4f("color", float4(1, 0, 1, 0.5f));
			renderer->apply();
			renderer->changeVertexFormat(m_quad_vf);
			renderer->changeVertexBuffer(0, m_quad_vb);
			renderer->changeIndexBuffer(m_quad_ib);
			renderer->drawElements(PRIM_TRIANGLES, 0, 6, 0, 4);

			renderer->setShaderConstant4x4f("worldViewProj", m_quad2_mvp);
			renderer->setShaderConstant4f("color", float4(0, 0, 1, 0.5f));
			renderer->apply();
			renderer->changeVertexFormat(m_quad_vf);
			renderer->changeVertexBuffer(0, m_quad_vb);
			renderer->changeIndexBuffer(m_quad_ib);
			renderer->drawElements(PRIM_TRIANGLES, 0, 6, 0, 4);

			renderer->setShaderConstant4x4f("worldViewProj", m_quad3_mvp);
			renderer->setShaderConstant4f("color", float4(0, 1, 1, 0.5f));
			renderer->apply();
			renderer->changeVertexFormat(m_quad_vf);
			renderer->changeVertexBuffer(0, m_quad_vb);
			renderer->changeIndexBuffer(m_quad_ib);
			renderer->drawElements(PRIM_TRIANGLES, 0, 6, 0, 4);
		}
	
		renderer->changeToMainFramebuffer();
		float col[4] = { 0,0,0,1 };
		renderer->clear(true, true, col, 1.0f);

		// Sorting pass
		renderer->reset();
		renderer->setRasterizerState(cullNone);
		renderer->setShader(m_sort_shd);
		renderer->setTexture("Color", m_unsorted_color_id);
		renderer->setTexture("Depth", m_unsorted_depth_id);
		renderer->setDepthState(noDepthTest);
		renderer->apply();

		context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->Draw(3, 0);

}


