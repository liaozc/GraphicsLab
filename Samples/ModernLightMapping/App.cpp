
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
#include "Util/TexturePacker.h"


#define ProjectDir	"/Samples/ModernLightMapping"
#include "InitResDir.inl"

const uint CLUSTER_X = 32;
const uint CLUSTER_Y = 8;
const uint CLUSTER_Z = 32;

BaseApp *app = new App();

bool App::onKey(const uint key, const bool pressed)
{
	if (D3D11App::onKey(key, pressed))
		return true;

	if (pressed)
	{
		if (key >= KEY_F5 && key <= KEY_F7)
		{
			m_RenderMode->selectItem(key - KEY_F5);
			return true;
		}
	}

	return false;
}

int App::getActiveLight() const
{
	for (uint i = 0; i < LIGHT_COUNT; i++)
	{
		if (distance(camPos, m_Lights[i].Position) < 200)
			return i;
	}
	return -1;
}

bool App::onMouseButton(const int x, const int y, const MouseButton button, const bool pressed)
{
	if (D3D11App::onMouseButton(x, y, button, pressed))
		return true;

	if (pressed)
	{
		switch (button)
		{
		case MOUSE_LEFT:
			{
				int light = getActiveLight();
				if (light >= 0)
				{
					if (m_ActiveLightsMask & (1 << light))
					{
						if (m_AnimatedLightsMask & (1 << light))
						{
							// Light is on, animated mode. Switch to steady mode.
							m_AnimatedLightsMask &= ~(1 << light);
						}
						else
						{
							// Light is on, steady mode. Turn it off.
							m_ActiveLightsMask &= ~(1 << light);
						}
					}
					else
					{
						// Light is off. Turn it on in animated mode.
						m_ActiveLightsMask |= (1 << light);
						m_AnimatedLightsMask |= (1 << light);
					}

					return true;
				}
			}
			break;
		case MOUSE_MIDDLE:
			{
				int light = getActiveLight();
				if (light >= 0)
				{
					m_Lights[light].Color.x = float(rand()) / RAND_MAX;
					m_Lights[light].Color.y = float(rand()) / RAND_MAX;
					m_Lights[light].Color.z = float(rand()) / RAND_MAX;
				}
			}
			break;
		case MOUSE_RIGHT:
			if (m_ActiveLightsMask)
			{
				if (m_AnimatedLightsMask)
				{
					m_AnimatedLightsMask = 0;
				}
				else
				{
					m_ActiveLightsMask = 0;
				}
			}
			else
			{
				m_ActiveLightsMask = (1 << LIGHT_COUNT) - 1;
				m_AnimatedLightsMask = (1 << LIGHT_COUNT) - 1;
			}

			return true;
		}
	}

	return false;
}

bool App::onMouseWheel(const int x, const int y, const int scroll)
{
	if (D3D11App::onMouseWheel(x, y, scroll))
		return true;

	int light = getActiveLight();
	if (light < 0)
		return false;

	m_Lights[light].Intensity = clamp(m_Lights[light].Intensity + 0.1f * scroll, 0.1f, 2.0f);

	return true;
}

void App::moveCamera(const float3 &dir)
{
	float3 newPos = camPos + dir * (speed * frameTime);

	float3 point;
	const BTri *tri;
	if (m_BSP.intersects(camPos, newPos, &point, &tri))
	{
		newPos = point + tri->plane.xyz();
	}
	m_BSP.pushSphere(newPos, 35);

	camPos = newPos;
}

void App::resetCamera()
{
	camPos = vec3(960, 1150, 960);
	wx = 0.22f;
	wy = 2.48f;
}

bool App::init()
{
	initWorkDir();

	srand(GetTickCount());

	// Initialize all lights
	m_Lights[0]  = { float3(  595.0f,  1075.0f,  -600.0f), 1.0f, float3(0.05f, 0.05f, 0.30f), 1.4f };
	m_Lights[1]  = { float3( 1610.0f,   655.0f,  -400.0f), 2.0f, float3(1.00f, 0.25f, 0.00f), 3.0f };
	m_Lights[2]  = { float3(-1220.0f, -1024.0f,     0.0f), 1.0f, float3(0.00f, 0.30f, 0.00f), 2.3f };
	m_Lights[3]  = { float3(-1220.0f, -1024.0f,   650.0f), 1.0f, float3(0.00f, 0.00f, 0.30f), 5.2f };
	m_Lights[4]  = { float3(-1220.0f, -1024.0f,  -650.0f), 1.0f, float3(0.30f, 0.00f, 0.00f), 4.0f };
	m_Lights[5]  = { float3( -435.0f,  -950.0f,  1480.0f), 2.0f, float3(0.35f, 0.60f, 0.90f), 2.3f };
	m_Lights[6]  = { float3(-1510.0f,  -190.0f,     0.0f), 1.0f, float3(0.40f, 0.10f, 0.05f), 3.4f };
	m_Lights[7]  = { float3( -810.0f,  1210.0f,   665.0f), 1.0f, float3(0.10f, 0.20f, 0.10f), 2.3f };
	m_Lights[8]  = { float3(    0.0f,   -70.0f,   620.0f), 1.0f, float3(0.05f, 0.05f, 0.50f), 2.9f };
	m_Lights[9]  = { float3( 1610.0f,   655.0f,   400.0f), 2.0f, float3(0.25f, 1.00f, 0.00f), 5.0f };
	m_Lights[10] = { float3(    0.0f,   420.0f,  -430.0f), 1.0f, float3(0.50f, 0.10f, 1.00f), 4.0f };
	m_Lights[11] = { float3(-1860.0f,   360.0f,  -600.0f), 1.0f, float3(0.15f, 0.50f, 0.15f), 1.2f };
	m_Lights[12] = { float3(-1860.0f,   360.0f,   600.0f), 1.0f, float3(0.15f, 0.50f, 0.15f), 3.1f };

	// Initially enable all lights in animated mode
	m_ActiveLightsMask = (1 << LIGHT_COUNT) - 1;
	m_AnimatedLightsMask = (1 << LIGHT_COUNT) - 1;

	if (!m_Map.loadT3d(ResDir("/Models/Map.t3d"))) return false;
	m_Map.scale(0, float3(-1, 1, 1));
	m_Map.removeStream(4);
	m_Map.removeStream(3);
	m_Map.removeStream(2);
	m_Map.computeTangentSpace(true);


	// Generate UV set for Lightmap and build BSP
	uint nIndices = m_Map.getIndexCount();
	const float3* src = (const float3 *) m_Map.getStream(0).vertices;
	const uint* inds = m_Map.getStream(0).indices;

	TexturePacker texPacker;
	for (uint i = 0; i < nIndices; i += 6)
	{
		float3 v0 = src[inds[i]];
		float3 v1 = src[inds[i + 1]];
		float3 v3 = src[inds[i + 5]];

		float w = length(v1 - v0) / 28;
		float h = length(v3 - v0) / 28;

		if (w < 8) w = 8;
		if (h < 8) h = 8;

		uint tw = (int) w;
		uint th = (int) h;

		tw = (tw + 4) & ~7;
		th = (th + 4) & ~7;

		texPacker.addRectangle(tw, th);
	}

	uint lm_width  = 512;
	uint lm_height = 512;

	if (!texPacker.assignCoords(&lm_width, &lm_height, widthComp))
	{
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

		float x0 = float(rect->x + 0.5f) / lm_width;
		float y0 = float(rect->y + 0.5f) / lm_height;
		float x1 = float(rect->x + rect->width  - 0.5f) / lm_width;
		float y1 = float(rect->y + rect->height - 0.5f) / lm_height;

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

		float3 v0 = src[inds[i]];
		float3 v1 = src[inds[i + 1]];
		float3 v2 = src[inds[i + 2]];
		float3 v3 = src[inds[i + 5]];

		m_BSP.addTriangle(v0, v1, v2);
		m_BSP.addTriangle(v0, v2, v3);

		index++;
	}
	m_BSP.build();

	m_Map.addStream(TYPE_TEXCOORD, 2, nLmCoords, (float *) lmCoords, lmIndices, true);



	// Enable this code if you want to regenerate the lightmaps. This code is not super fast, and may take about 15min to run.
#if 0
	/*****	Lightmap generation code *****/
	const uint SAMPLE_COUNT = 250;

	float3 p_samples[SAMPLE_COUNT];
	float2 t_samples[SAMPLE_COUNT];
	for (uint s = 0; s < SAMPLE_COUNT; s++)
	{
		float3 p;
		do
		{
			p = float3(float(rand()), float(rand()), float(rand())) * (2.0f / RAND_MAX) - 1.0f;
		} while (dot(p, p) > 1.0f);
		p_samples[s] = p;

		t_samples[s] = float2(float(rand()), float(rand())) * (1.0f / RAND_MAX) - 0.5f;
	}


	const uint cluster_divisor = 2;
	const uint cm_width  = lm_width  / cluster_divisor;
	const uint cm_height = lm_height / cluster_divisor;
	uint16* clusters = new uint16[cm_width * cm_height];
	memset(clusters, 0, cm_width * cm_height * sizeof(uint16));

	for (uint light_index = 0; light_index < LIGHT_COUNT; light_index++)
	{
		const float3 light_pos = m_Lights[light_index].Position;

		uint8* lMap = new uint8[lm_width * lm_height];
		memset(lMap, 0, lm_width * lm_height);

		uint index = 0;
		for (uint i = 0; i < nIndices; i += 6)
		{
			const TextureRectangle& rect = *texPacker.getRectangle(index);
			index++;

			float3 pos = src[inds[i]];

			float3 dirS = (src[inds[i + 1]] - pos);
			float3 dirT = (src[inds[i + 5]] - pos);
			float3 normal = cross(dirS, dirT);

			float d = -dot(pos, normal);

			if (dot(light_pos, normal) + d < 0.0f)
				continue;

			// Compute light constribution on this rectangle
			for (uint t = 0; t < rect.height; t++)
			{
				for (uint s = 0; s < rect.width; s++)
				{
					float light = 0.0f;
					for (uint k = 0; k < SAMPLE_COUNT; k++)
					{
						float3 samplePos = pos +
							saturate( (float(s) / (rect.width  - 1)) + t_samples[k].x / (rect.width  - 1) ) * dirS +
							saturate( (float(t) / (rect.height - 1)) + t_samples[k].y / (rect.height - 1) ) * dirT;

						m_BSP.pushSphere(samplePos, 0.3f);
						m_BSP.pushSphere(samplePos, 0.2f);
						m_BSP.pushSphere(samplePos, 0.1f);

						if (!m_BSP.intersects(samplePos, light_pos + 150.0f * p_samples[k]))
							light += 1.0f / SAMPLE_COUNT;
					}
	
					uint8 light_val = (uint8)(255.0f * sqrtf(light) + 0.5f); // sqrtf() for poor man's gamma
					lMap[(rect.y + t) * lm_width + (rect.x + s)] = light_val;
				}
			}

			// Assign light clusters for this rectangle
			const uint s0 = rect.x / cluster_divisor;
			const uint t0 = rect.y / cluster_divisor;
			const uint s1 = s0 + rect.width  / cluster_divisor;
			const uint t1 = t0 + rect.height / cluster_divisor;
			for (uint t = t0; t < t1; t++)
			{
				for (uint s = s0; s < s1; s++)
				{
					// Sample the light within the rect, plus one extra pixel border to cover filtering, but clamp to stay within the rect
					uint ls0 = max(int(s * cluster_divisor - 1), (int) rect.x);
					uint ls1 = min((s + 1) * cluster_divisor + 1, rect.x + rect.width);
					uint lt0 = max(int(t * cluster_divisor - 1), (int) rect.y);
					uint lt1 = min((t + 1) * cluster_divisor + 1, rect.y + rect.height);

					for (uint lt = lt0; lt < lt1; lt++)
					{
						for (uint ls = ls0; ls < ls1; ls++)
						{
							const uint8 light_val = lMap[lt * lm_width + ls];
							if (light_val)
							{
								clusters[t * cm_width + s] |= (1 << light_index);
								goto double_break;
							}
						}
					}
					double_break:

					;
				}
			}
		}

		Image image;
		image.loadFromMemory(lMap, FORMAT_I8, lm_width, lm_height, 1, 1, true);
		char fileName[256];
		sprintf(fileName, "LightMap%d.dds", light_index);
		image.saveImage(fileName);
	}

	Image image;
	image.loadFromMemory(clusters, FORMAT_R16UI, cm_width, cm_height, 1, 1, true);
	image.saveImage("Clusters.dds");

#endif

	m_Sphere.createSphere(3);

	// Init GUI components
	int tab = configDialog->addTab("Rendering mode");
	configDialog->addWidget(tab, new Label(10, 10, 340, 36, "Rendering mode (F5-F7)"));

	m_RenderMode = new DropDownList(10, 50, 380, 36);
	m_RenderMode->addItem("Clustered Lightmap");
	m_RenderMode->addItem("No Clusters");
	m_RenderMode->addItem("Visualize clusters");
	m_RenderMode->selectItem(0);
	configDialog->addWidget(tab, m_RenderMode);

	return true;
}

enum RenderMode
{
	CLUSTERED,
	NO_CLUSTERS,
	VISUALIZE,
};

void App::exit()
{
}

bool App::initAPI()
{
	return D3D11App::initAPI(D3D10_0, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_D16_UNORM, max(antiAliasSamples, 1), NO_SETTING_CHANGE);
}

void App::exitAPI()
{
	D3D11App::exitAPI();
}

bool App::load()
{
	// Shaders
	if ((m_Lighting[CLUSTERED]   = renderer->addShader(ShaderDir("/Lighting.shd"), DEFINE_STR(LIGHT_COUNT))) == SHADER_NONE) return false;
	if ((m_Lighting[NO_CLUSTERS] = renderer->addShader(ShaderDir("/Lighting.shd"), DEFINE_STR(LIGHT_COUNT) "#define NO_CLUSTERS\n")) == SHADER_NONE) return false;
	if ((m_Lighting[VISUALIZE]   = renderer->addShader(ShaderDir("/Lighting.shd"), DEFINE_STR(LIGHT_COUNT) "#define SHOW_CLUSTERS\n")) == SHADER_NONE) return false;
	if ((m_PreZ      = renderer->addShader(ShaderDir("/PreZ.shd"))) == SHADER_NONE) return false;
	if ((m_LightBlob = renderer->addShader(ShaderDir("/LightBlob.shd"))) == SHADER_NONE) return false;

	// Samplerstates
	if ((m_TrilinearAniso = renderer->addSamplerState(TRILINEAR_ANISO, WRAP, WRAP, WRAP)) == SS_NONE) return false;
	if ((m_LinearClamp    = renderer->addSamplerState(LINEAR, CLAMP, CLAMP, CLAMP)) == SS_NONE) return false;

	// Textures
	if ((m_Base[0] = renderer->addTexture  (ResDir("/Textures/floor_wood_3.dds"),                    true, m_TrilinearAniso, SRGB)) == TEXTURE_NONE) return false;
	if ((m_Bump[0] = renderer->addNormalMap(ResDir("/Textures/floor_wood_3Bump.dds"), FORMAT_RGBA8S, true, m_TrilinearAniso)) == TEXTURE_NONE) return false;

	if ((m_Base[1] = renderer->addTexture  (ResDir("/Textures/floor_wood_4.dds"),                    true, m_TrilinearAniso, SRGB)) == TEXTURE_NONE) return false;
	if ((m_Bump[1] = renderer->addNormalMap(ResDir("/Textures/floor_wood_4Bump.dds"), FORMAT_RGBA8S, true, m_TrilinearAniso)) == TEXTURE_NONE) return false;

	if ((m_Base[2] = renderer->addTexture  (ResDir("/Textures/wallpaper20.dds"),                    true, m_TrilinearAniso, SRGB)) == TEXTURE_NONE) return false;
	if ((m_Bump[2] = renderer->addNormalMap(ResDir("/Textures/wallpaper20Bump.dds"), FORMAT_RGBA8S, true, m_TrilinearAniso)) == TEXTURE_NONE) return false;

	static const std::string file_name_str[] = {
		ShaderDir("/LightMap0.dds"),
		ShaderDir("/LightMap1.dds"),
		ShaderDir("/LightMap2.dds"),
		ShaderDir("/LightMap3.dds"),
		ShaderDir("/LightMap4.dds"),
		ShaderDir("/LightMap5.dds"),
		ShaderDir("/LightMap6.dds"),
		ShaderDir("/LightMap7.dds"),
		ShaderDir("/LightMap8.dds"),
		ShaderDir("/LightMap9.dds"),
		ShaderDir("/LightMap10.dds"),
		ShaderDir("/LightMap11.dds"),
		ShaderDir("/LightMap12.dds"),
	};


	static const char* file_names[] = {
		file_name_str[0].c_str(),
		file_name_str[1].c_str(),
		file_name_str[2].c_str(),
		file_name_str[3].c_str(),
		file_name_str[4].c_str(),
		file_name_str[5].c_str(),
		file_name_str[6].c_str(),
		file_name_str[7].c_str(),
		file_name_str[8].c_str(),
		file_name_str[9].c_str(),
		file_name_str[10].c_str(),
		file_name_str[11].c_str(),
		file_name_str[12].c_str()
	};

	static_assert(elementsOf(file_names) == LIGHT_COUNT, "Must provide LIGHT_COUNT filenames");

	Image light_maps;
	if (!light_maps.loadSlicedImage(file_names, 1, LIGHT_COUNT)) return false;
	if ((m_LightMaps = renderer->addTexture(light_maps, false, m_LinearClamp)) == TEXTURE_NONE) return false;

	Image clusters;
	clusters.loadDDS(ShaderDir("/Clusters.dds"));
	m_ClusterMapSize.x = float(clusters.getWidth());
	m_ClusterMapSize.y = float(clusters.getHeight());
	if ((m_Clusters = renderer->addTexture(clusters, false, m_LinearClamp)) == TEXTURE_NONE) return false;


	// Blendstates
	if ((m_AlphaBlend = renderer->addBlendState(ONE, ONE_MINUS_SRC_ALPHA)) == BS_NONE) return false;

	// Depth states - use reversed depth (1 to 0) to improve precision
	if ((m_DepthTest = renderer->addDepthState(true, true, GEQUAL)) == DS_NONE) return false;

	// Upload map to vertex/index buffer
	if (!m_Map.makeDrawable(renderer, true, m_Lighting[CLUSTERED])) return false;
	if (!m_Sphere.makeDrawable(renderer, true, m_LightBlob)) return false;

	return true;
}

void App::unload()
{
}

void App::updateLights()
{
	m_Ambient = 0.0f;

	for (int i = 0; i < LIGHT_COUNT; i++)
	{
		if (m_AnimatedLightsMask & (1 << i))
		{
			const float speed = 0.5f;

			float x = 0.5f + 0.5f * sinf(m_Lights[i].Frequency * time * speed + i);
			m_Lights[i].Intensity = 0.1f + 1.9f * x * x;
		}

		if (m_ActiveLightsMask & (1 << i))
		{
			m_Ambient += m_Lights[i].Intensity * m_Lights[i].Color;
		}
	}


	// Compute ambient and exposure. No physics was consulted to come up with this math.
	float luminance = dot(m_Ambient, float3(0.3f, 0.59f, 0.11f));
	m_Exposure = 10.0f * expf(-0.5f * luminance);

	m_Ambient *= 0.001f;
	m_Ambient += 0.0003f;
}

void App::drawLights(const float4x4& view_proj)
{
	renderer->reset();
	renderer->setRasterizerState(cullBack);
	renderer->setShader(m_LightBlob);
	renderer->setShaderConstant4x4f("ViewProj", view_proj);
	renderer->setDepthState(m_DepthTest);
	renderer->setBlendState(m_AlphaBlend);
	renderer->apply();

	for (int i = 0; i < LIGHT_COUNT; i++)
	{
		float3 color = m_Lights[i].Color;
		float m = max(max(color.x, color.y), color.z);
		if (m > 0)
			color *= 1.0f / m;

		if (m_ActiveLightsMask & (1 << i))
			color *= m_Lights[i].Intensity;
		else
			color *= 0.05f;

		renderer->setShaderConstant3f("LightPos", m_Lights[i].Position);
		renderer->setShaderConstant3f("Color", color);
		renderer->setShaderConstant3f("CamPos", camPos);
		renderer->applyConstants();

		m_Sphere.draw(renderer);
	}
}

void App::drawFrame()
{
	updateLights();

	const float near_plane = 20.0f;
	const float far_plane = 4500.0f;

	// Reversed depth
	float4x4 projection = toD3DProjection(perspectiveMatrixY(1.2f, width, height, far_plane, near_plane));
	float4x4 view = rotateXY(-wx, -wy);
	view.translate(-camPos);
	float4x4 view_proj = projection * view;

	RenderMode mode = (RenderMode) m_RenderMode->getSelectedItem();

	renderer->changeRenderTargets(NULL, 0, FB_DEPTH);

		renderer->clear(false, true, false, NULL, 0.0f);

		/*
			Pre-Z pass for a decent performance increase.
		*/
		renderer->reset();
		renderer->setRasterizerState(cullFront);
		renderer->setShader(m_PreZ);
		renderer->setShaderConstant4x4f("ViewProj", view_proj);
		renderer->setDepthState(m_DepthTest);
		renderer->apply();

		m_Map.draw(renderer);

	renderer->changeToMainFramebuffer();


	/*
		Main lighting pass.
	*/
	renderer->reset();
	renderer->setRasterizerState(cullBack);
	renderer->setShader(m_Lighting[mode]);
	renderer->setShaderConstant4x4f("ViewProj", view_proj);
	renderer->setShaderConstant3f("CamPos", camPos);
	renderer->setShaderConstant1i("ActiveLightsMask", m_ActiveLightsMask);
	renderer->setShaderConstant3f("Ambient", m_Ambient);
	renderer->setShaderConstant1f("Exposure", m_Exposure);
	renderer->setShaderConstant2f("ClusterMapSize", m_ClusterMapSize);
	renderer->setShaderConstantArray4f("Lights", (const float4 *) m_Lights, LIGHT_COUNT * (sizeof(Light) / 16));
	renderer->setSamplerState("LmFilter", m_LinearClamp);
	if (mode != VISUALIZE)
	{
		renderer->setSamplerState("Filter", m_TrilinearAniso);
		renderer->setTexture("LightMaps", m_LightMaps);
	}
	if (mode != NO_CLUSTERS)
		renderer->setTexture("Clusters", m_Clusters);
	renderer->setDepthState(m_DepthTest);
	renderer->apply();

	for (uint i = 0; i < m_Map.getBatchCount(); i++)
	{
		if (mode != VISUALIZE)
		{
			renderer->setTexture("Base", m_Base[i]);
			renderer->setTexture("Bump", m_Bump[i]);
		}
		renderer->applyTextures();

		m_Map.drawBatch(renderer, i);
	}

	drawLights(view_proj);

}
