
/* * * * * * * * * * * * * Author's note * * * * * * * * * * * *\
*   _       _   _       _   _       _   _       _     _ _ _ _   *
*  |_|     |_| |_|     |_| |_|_   _|_| |_|     |_|  _|_|_|_|_|  *
*  |_|_ _ _|_| |_|     |_| |_|_|_|_|_| |_|     |_| |_|_ _ _     *
*  |_|_|_|_|_| |_|     |_| |_| |_| |_| |_|     |_|   |_|_|_|_   *
*  |_|     |_| |_|_ _ _|_| |_|     |_| |_|_ _ _|_|  _ _ _ _|_|  *
*  |_|     |_|   |_|_|_|   |_|     |_|   |_|_|_|   |_|_|_|_|    *
*                                                               *
*                     http://www.humus.name                     *
*                                                                *
* This file is a part of the work done by Humus. You are free to   *
* use the code in any way you like, modified, unmodified or copied   *
* into your own work. However, I expect you to respect these points:  *
*  - If you use this file and its contents unmodified, or use a major *
*    part of this file, please credit the author and leave this note. *
*  - For use in anything commercial, please request my approval.     *
*  - Share your work and ideas too as much as you can.             *
*                                                                *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "App.h"
#include <math.h>

#define ProjectDir	"/Samples/PhoneWireAA"
#include "InitResDir.inl"

BaseApp *app = new App();

struct Pole
{
	float3 pos;
	float wy;
};

static const Pole poles[] =
{
	{ float3(-126.41f, 32.47f, -126.36f),	-1.05f },
	{ float3(-108.14f, 46.86f, -114.83f),	-0.70f },
	{ float3( -90.38f, 47.39f,  -71.62f),	-0.25f },
	{ float3( -87.54f, 41.54f,  -25.75f),	-0.15f },
	{ float3( -78.44f, 42.12f,   25.46f),	-0.45f },
	{ float3( -24.56f, 39.10f,   72.70f),	-1.00f },
	{ float3(  14.73f, 38.23f,   94.46f),	-1.25f },
	{ float3(  66.08f, 45.37f,   97.89f),	-1.30f },
	{ float3( 125.61f, 31.79f,  126.45f),	-1.15f },
};

bool App::GetTerrainHeight(float &height, const float3 &position) const
{
	// Collision detection against terrain
	float xc = position.x + 127.5f;
	float yc = position.z + 127.5f;

	uint ix = (int) floorf(xc);
	uint iy = (int) floorf(yc);

	if (ix < 255 && iy < 255)
	{
		float fx = xc - ix;
		float fy = yc - iy;

		ubyte *pixels = m_TerrainImage.getPixels();

		float h00 = (float) pixels[(iy + 0) * 256 + (ix + 0)];
		float h01 = (float) pixels[(iy + 0) * 256 + (ix + 1)];
		float h10 = (float) pixels[(iy + 1) * 256 + (ix + 0)];
		float h11 = (float) pixels[(iy + 1) * 256 + (ix + 1)];

		float h0 = lerp(h00, h01, fx);
		float h1 = lerp(h10, h11, fx);
		float h = lerp(h0, h1, fy);

		h *= (50.0f / 255.0f);
		h += 1.0f;

		height = h;

		return true;
	}

	return false;
}


void App::resetCamera()
{
	camPos = vec3(125.0f, 50.0f, 70.0f);
	wx = 0.059f;
	wy = 1.59f;
}

void App::moveCamera(const vec3 &dir)
{
	camPos += dir * (frameTime * speed);

	// Collision detection against terrain
	float h;
	if (GetTerrainHeight(h, camPos))
	{
		if (camPos.y < h)
			camPos.y = h;
	}
}

bool App::init()
{
	speed = 50;

	m_Pole = new Model();
	if (!m_Pole->loadObj(ResDir("/Models/pole.oobj"))) return false;

	// Init GUI components
	int tab = configDialog->addTab("Phone Wire AA");
	configDialog->addWidget(tab, m_UseWireAA = new CheckBox(0, 0, 250, 36, "Use Phone-wire AA", true));
	configDialog->addWidget(tab, new Label(0, 40, 192, 36, "Wire radius"));
	configDialog->addWidget(tab, m_WireRadius = new Slider(0, 80, 350, 24, -5.0f, -2.0f, -3.5f));

	return true;
}

void App::exit()
{
	delete m_Pole;
}

bool App::initAPI()
{
	return D3D10App::initAPI(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_D32_FLOAT, max(antiAliasSamples, 1), 0);
}

void App::exitAPI()
{
	D3D10App::exitAPI();
}

const int SEGMENT_COUNT = 32;
const int VERTEX_COUNT = (SEGMENT_COUNT + 1) * (8 + 1);
const int INDEX_COUNT = SEGMENT_COUNT * (8 + 1) * 2;
const int WIRE_COUNT = elementsOf(poles) - 1;

bool App::load()
{
	initWorkDir(renderer);

	// Shaders
	if ((m_TerrainShader = renderer->addShader(ShaderDir("/Terrain.shd"))) == SHADER_NONE) return false;
	if ((m_SkyboxShader = renderer->addShader(ShaderDir("/Skybox.shd"))) == SHADER_NONE) return false;
	if ((m_PoleShader = renderer->addShader(ShaderDir("/Pole.shd"))) == SHADER_NONE) return false;
	if ((m_WireShader = renderer->addShader(ShaderDir("/Phonewire.shd"))) == SHADER_NONE) return false;

	// Samplerstates
	if ((m_TrilinearClamp = renderer->addSamplerState(TRILINEAR, CLAMP, CLAMP, CLAMP)) == SS_NONE) return false;
	if ((m_TrilinearAniso = renderer->addSamplerState(TRILINEAR_ANISO, WRAP, WRAP, WRAP)) == SS_NONE) return false;

	// Terrain
	m_TerrainImage.loadDDS(ShaderDir("/Terrain.dds"));
	if ((m_Terrain = renderer->addTexture(m_TerrainImage, false, linearClamp)) == TEXTURE_NONE) return false;
	m_TerrainImage.uncompressImage();

	if ((m_TerrainShadow = renderer->addTexture(ShaderDir("/TerrainShadow.dds"), false, linearClamp)) == TEXTURE_NONE) return false;

	// Textures
	if ((m_Ground0 = renderer->addTexture(ResDir("/Textures/grass-texture-1.dds"), true, m_TrilinearAniso, SRGB)) == TEXTURE_NONE) return false;
	if ((m_Ground1 = renderer->addTexture(ResDir("/Textures/rock.pcx"), true, m_TrilinearAniso, SRGB)) == TEXTURE_NONE) return false;
	if ((m_Wood    = renderer->addTexture(ResDir("/Textures/floor_wood_4.dds"), true, m_TrilinearAniso, SRGB)) == TEXTURE_NONE) return false;
	if ((m_WireTex = renderer->addTexture(ResDir("/Textures/Wire.dds"), true, m_TrilinearAniso, SRGB)) == TEXTURE_NONE) return false;

	const std::string filestrs[] = {
		ResDir("/Textures/CubeMaps/Plains-of-abraham/posx.jpg"),
		ResDir("/Textures/CubeMaps/Plains-of-abraham/negx.jpg"),
		ResDir("/Textures/CubeMaps/Plains-of-abraham/posy.jpg"),
		ResDir("/Textures/CubeMaps/Plains-of-abraham/negy.jpg"),
		ResDir("/Textures/CubeMaps/Plains-of-abraham/posz.jpg"),
		ResDir("/Textures/CubeMaps/Plains-of-abraham/negz.jpg"),
	};

	const char *files[] =
	{
		filestrs[0].c_str(),
		filestrs[1].c_str(),
		filestrs[2].c_str(),
		filestrs[3].c_str(),
		filestrs[4].c_str(),
		filestrs[5].c_str(),
	};
	if ((m_Environment = renderer->addCubemap(files, true, m_TrilinearClamp, 1, SRGB)) == TEXTURE_NONE) return false;

	// Blendstates
	if ((m_WireBlend = renderer->addBlendState(SRC_ALPHA, ONE_MINUS_SRC_ALPHA)) == BS_NONE) return false;

	// Rasterizerstates
	if ((m_NoMSAA = renderer->addRasterizerState(CULL_NONE, SOLID, false, false)) == RS_NONE) return false;


	// Generate wire mesh
	struct Vertex
	{
		float3 Position;
		float3 Normal;
		float2 TexCoord;
	};
	Vertex vertices[3][WIRE_COUNT][(SEGMENT_COUNT + 1)][8 + 1];

	for (int n = 0; n < 3; n++)
	{
		float height_offset = (n == 1)? 11.79f : 10.63f;
		float tangent_scale = float(n - 1) * 1.11f;
		float bitangent_scale = abs(n - 1) * -0.29f;

		for (int w = 0; w < WIRE_COUNT; w++)
		{
			const float c0 = cosf(poles[w + 0].wy);
			const float s0 = sinf(poles[w + 0].wy);
			const float c1 = cosf(poles[w + 1].wy);
			const float s1 = sinf(poles[w + 1].wy);

			float3 tangent0(c0, 0, s0);
			float3 tangent1(c1, 0, s1);
			float3 bitangent0(-s0, 0, c0);
			float3 bitangent1(-s1, 0, c1);

			float3 pos0 = poles[w + 0].pos + tangent_scale * tangent0 + bitangent_scale * bitangent0;
			float3 pos1 = poles[w + 1].pos + tangent_scale * tangent1 + bitangent_scale * bitangent1;
			pos0.y += height_offset;
			pos1.y += height_offset;

			const float hang = distance(pos0, pos1) * 0.25f;


			float3 positions[SEGMENT_COUNT + 1];

			for (int i = 0; i <= SEGMENT_COUNT; i++)
			{
				float t = float(i) / SEGMENT_COUNT;

				positions[i] = lerp(pos0, pos1, t);
				positions[i].y -= t * (1.0f - t) * hang;
			}


			for (int i = 0; i <= SEGMENT_COUNT; i++)
			{
				int prev = (i > 0)? i - 1 : 0;
				int next = (i < SEGMENT_COUNT)? i + 1 : SEGMENT_COUNT;

				float3 dir = normalize(positions[next] - positions[prev]);
				float3 dir0 = normalize(cross(dir, float3(0, 1, 0)));
				float3 dir1 = normalize(cross(dir0, dir));

				for (int a = 0; a < 8; a++)
				{
					float angle = a * (2 * PI / 8);
					float x = cosf(angle);
					float y = sinf(angle);

					float3 normal = x * dir0 + y * dir1;

					vertices[n][w][i][a].Position = positions[i];
					vertices[n][w][i][a].Normal = normal;
					vertices[n][w][i][a].TexCoord.x = float(i * 2);
					vertices[n][w][i][a].TexCoord.y = float(a * 0.125f);
				}
				vertices[n][w][i][8] = vertices[n][w][i][0];
				vertices[n][w][i][8].TexCoord.y = 1.0f;
			}
		}
	}
	if ((m_WireVB = renderer->addVertexBuffer(sizeof(vertices), STATIC, vertices)) == VB_NONE) return false;


	uint16 indices[SEGMENT_COUNT][(8 + 1) * 2];

	for (int i = 0; i < SEGMENT_COUNT; i++)
	{
		for (int a = 0; a <= 8; a++)
		{
			indices[i][2 * a + 0] = i * (8 + 1) + a;
			indices[i][2 * a + 1] = (i + 1) * (8 + 1) + a;
		}
	}
	if ((m_WireIB = renderer->addIndexBuffer(INDEX_COUNT, sizeof(uint16), STATIC, indices)) == IB_NONE) return false;


	FormatDesc wire_format[] =
	{
		{ 0, TYPE_VERTEX,   FORMAT_FLOAT, 3 },
		{ 0, TYPE_NORMAL,   FORMAT_FLOAT, 3 },
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, 2 },
	};
	if ((m_WireVF = renderer->addVertexFormat(wire_format, elementsOf(wire_format), m_WireShader)) == VF_NONE) return false;


	// Upload map to vertex/index buffer
	if (!m_Pole->makeDrawable(renderer, true, m_PoleShader)) return false;

	return true;
}

void App::unload()
{
	m_TerrainImage.clear();
}

void App::drawFrame()
{
	const float fov = 1.0f;

	float4x4 projection = toD3DProjection(perspectiveMatrixY(fov, width, height, 0.1f, 5000));
	float4x4 view = rotateXY(-wx, -wy);
	float4x4 inv_vp_env = !(projection * view);
	view.translate(-camPos);
	float4x4 view_proj = projection * view;


	renderer->clear(false, true, true);

	/*
		Terrain
	*/
	renderer->reset();
	renderer->setRasterizerState(cullBack);
	renderer->setShader(m_TerrainShader);
	renderer->setShaderConstant4x4f("ViewProj", view_proj);
	renderer->setTexture("Terrain", m_Terrain);
	renderer->setTexture("Ground0", m_Ground0);
	renderer->setTexture("Ground1", m_Ground1);
	renderer->setTexture("Shadow", m_TerrainShadow);
	renderer->setSamplerState("Filter", m_TrilinearAniso);
	renderer->setSamplerState("ShadowFilter", linearClamp);
	renderer->setSamplerState("VSFilter", linearClamp);
	renderer->apply();

	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	device->DrawInstanced(256, 255, 0, 0);


	/*
		Skybox
	*/
	renderer->reset();
	renderer->setRasterizerState(m_NoMSAA);
	renderer->setShader(m_SkyboxShader);
	renderer->setShaderConstant4x4f("inv_mvp", rotateY(1.11f) * inv_vp_env);
	renderer->setTexture("Env", m_Environment);
	renderer->setSamplerState("Filter", m_TrilinearClamp);
	renderer->setDepthState(noDepthWrite);
	renderer->apply();

	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	device->Draw(3, 0);


	/*
		Pole
	*/
	renderer->reset();
	renderer->setRasterizerState(cullBack);
	renderer->setShader(m_PoleShader);
	renderer->setSamplerState("Filter", m_TrilinearAniso);
	renderer->apply();

	for (uint p = 0; p < elementsOf(poles); p++)
	{
		float4x4 world = translate(poles[p].pos) * rotateY(PI / 2 + poles[p].wy) * scale(0.3f, 0.3f, 0.3f);
		
		renderer->setShaderConstant4x4f("WorldViewProj", view_proj * world);
		renderer->setShaderConstant4x4f("World", world);
		renderer->applyConstants();

		uint count = m_Pole->getBatchCount();
		for (uint i = 0; i < count; i++)
		{
			renderer->setTexture("Base", (i == 0 || i == 2)? m_WireTex : m_Wood);
			renderer->applyTextures();

			m_Pole->drawBatch(renderer, i);
		}
	}


	/*
		Phone wire
	*/
	float pixel_scale;
	if (m_UseWireAA->isChecked())
		pixel_scale = tanf(0.5f * fov) / float(height);
	else
		pixel_scale = 1e-10f; // Arbitrary small non-zero number to effectively disable clamping

	float radius = expf(m_WireRadius->getValue());

	renderer->reset();
	renderer->setRasterizerState(cullBack);
	renderer->setBlendState(m_WireBlend);
	renderer->setVertexFormat(m_WireVF);
	renderer->setVertexBuffer(0, m_WireVB);
	renderer->setIndexBuffer(m_WireIB);
	renderer->setShader(m_WireShader);
	renderer->setShaderConstant4x4f("ViewProj", view_proj);
	renderer->setShaderConstant1f("PixelScale", pixel_scale);
	renderer->setShaderConstant1f("Radius", radius);
	renderer->setTexture("Wire", m_WireTex);
	renderer->setSamplerState("Filter", m_TrilinearAniso);
	renderer->apply();

	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	for (int i = 0; i < WIRE_COUNT * 3; i++)
	{
		device->DrawIndexed(INDEX_COUNT, 0, i * VERTEX_COUNT);
	}
}
