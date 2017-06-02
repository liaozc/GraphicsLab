
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
#include "../Framework3/Util/Model.h"

#define ProjectDir	"/Samples/BSPBlending"
#include "InitResDir.inl"

BaseApp *app = new App();

// Comment this out if you want to step through a CPU version of what the compute shader does
#define USE_COMPUTE

struct Vertex
{
	float3 Position;
	float3 UV;
	float3 Tangent;
	float3 Bitangent;
	float3 Normal;
};

struct Quad
{
	Vertex Vertices[4];
};

static void BuildBSP(Quad* vertex_buffer, BSPNode* nodes, uint& dest, const Quad* quads, uint count)
{
	uint best_index = 0;
	uint best_score = INT_MAX;
	uint best_split = INT_MAX;
	uint best_back = 0;
	uint best_front = 0;
	uint best_coplanar_back_count = 0;

	// Find the best splitting plane. Naive O(n^2) search.
	for (uint i = 0; i < count; i++)
	{
		float3 v0 = quads[i].Vertices[0].Position;
		float3 d0 = quads[i].Vertices[1].Position - v0;
		float3 d1 = quads[i].Vertices[2].Position - v0;

		float3 normal = normalize(cross(d0, d1));
		float d = -dot(v0, normal);

		uint back  = 0;
		uint front = 0;
		uint split = 0;
		uint coplanar = 0;
		for (uint j = 0; j < count; j++)
		{
			if (i == j)
				continue;

			float d0 = dot(quads[j].Vertices[0].Position, normal) + d;
			float d1 = dot(quads[j].Vertices[1].Position, normal) + d;
			float d2 = dot(quads[j].Vertices[2].Position, normal) + d;
			float d3 = dot(quads[j].Vertices[3].Position, normal) + d;

			bool has_back =  (d0 < 0.0f || d1 < 0.0f || d2 < 0.0f || d3 < 0.0f);
			bool has_front = (d0 > 0.0f || d1 > 0.0f || d2 > 0.0f || d3 > 0.0f);
			bool is_coplanar = !(has_back || has_front);

			if (has_back)  back++;
			if (has_front) front++;
			if (has_back && has_front) split++;
			if (is_coplanar) coplanar++;
		}

		// For coplanar planes we distribute them to get the best balance. This reduced the max depth from 19 to 11 for this demo.
		// A better and more general approach is to have each node store an arbitrary number of polygons. That would further reduce
		// depth, and can still be made parallel traversable, but would need to separate node index from polygon index, so each node
		// stored both size of back tree in terms of nodes and in terms of triangles. Many neighboring threads would land on the same
		// leaf node, but pick different triangles from it. This was not interesting enough for me to explore in this demo, but worth
		// noting the possibility.
		uint coplanar_back_count = 0;
		if (back < front)
		{
			uint balance_count = front - back;
			if (coplanar <= balance_count)
				coplanar_back_count = coplanar;
			else
				coplanar_back_count = balance_count + (coplanar - balance_count) / 2;
		}
		else
		{
			uint balance_count = back - front;
			if (coplanar <= balance_count)
				coplanar_back_count = 0;
			else
				coplanar_back_count = coplanar - (balance_count + (coplanar - balance_count) / 2);
		}

		back += coplanar_back_count;
		front += coplanar - coplanar_back_count;

		uint score = max(back, front);

		if (score < best_score || (score == best_score && split < best_split))
		{
			best_score = score;
			best_split = split;
			best_back = back;
			best_front = front;
			best_coplanar_back_count = coplanar_back_count;

			best_index = i;
		}
	}

	float3 v0 = quads[best_index].Vertices[0].Position;
	float3 d0 = quads[best_index].Vertices[1].Position - v0;
	float3 d1 = quads[best_index].Vertices[2].Position - v0;

	float3 normal = normalize(cross(d0, d1));
	float d = -dot(v0, normal);

	vertex_buffer[dest] = quads[best_index];

	BSPNode* node = nodes + dest;
	node->Normal = normal;
	node->D = d;
	node->BackNodeCount = 0;

	dest++;

	if (best_score == 0)
		return;

	Quad* new_list = new Quad[best_score]; // Allocate for the max case, and reuse memory for both paths

	// Fill back list
	if (best_back)
	{
		uint n = 0;
		uint c = 0;
		for (uint i = 0; i < count; i++)
		{
			if (i == best_index)
				continue;

			float d0 = dot(quads[i].Vertices[0].Position, normal) + d;
			float d1 = dot(quads[i].Vertices[1].Position, normal) + d;
			float d2 = dot(quads[i].Vertices[2].Position, normal) + d;
			float d3 = dot(quads[i].Vertices[3].Position, normal) + d;

			bool has_back =  (d0 < 0.0f || d1 < 0.0f || d2 < 0.0f || d3 < 0.0f);
			bool has_front = (d0 > 0.0f || d1 > 0.0f || d2 > 0.0f || d3 > 0.0f);
			bool coplanar = !(has_back || has_front);
			if (coplanar)
				c++;

			if (has_back || (coplanar && c <= best_coplanar_back_count))
			{
				if (has_front)
				{
					// Handle split
					uint v = 0;
					for (uint k = 0; k < 4; k++)
					{
						Vertex curr = quads[i].Vertices[k];
						float curr_d = dot(curr.Position, normal) + d;

						if (curr_d < 0.0f)
						{
							new_list[n].Vertices[v] = curr;
							++v;
						}
						else
						{
							Vertex last = quads[i].Vertices[(k + 3) & 3];
							float last_d = dot(last.Position, normal) + d;
							if (last_d < 0.0f)
							{
								float f = last_d / (last_d - curr_d);
								new_list[n].Vertices[v].Position = lerp(last.Position, curr.Position, f);
								new_list[n].Vertices[v].UV = lerp(last.UV, curr.UV, f);
								new_list[n].Vertices[v].Tangent   = curr.Tangent;
								new_list[n].Vertices[v].Bitangent = curr.Bitangent;
								new_list[n].Vertices[v].Normal    = curr.Normal;
								++v;
							}

							Vertex next = quads[i].Vertices[(k + 1) & 3];
							float next_d = dot(next.Position, normal) + d;
							if (next_d < 0.0f)
							{
								float f = curr_d / (curr_d - next_d);
								new_list[n].Vertices[v].Position = lerp(curr.Position, next.Position, f);
								new_list[n].Vertices[v].UV = lerp(curr.UV, next.UV, f);
								new_list[n].Vertices[v].Tangent   = curr.Tangent;
								new_list[n].Vertices[v].Bitangent = curr.Bitangent;
								new_list[n].Vertices[v].Normal    = curr.Normal;
								++v;
							}
						}
					}
					ASSERT(v == 4);
				}
				else
				{
					new_list[n] = quads[i];
				}
				n++;
				ASSERT(n <= best_score);
			}
		}
		ASSERT(n == best_back);

		uint back_node = dest;

		BuildBSP(vertex_buffer, nodes, dest, new_list, best_back);

		node->BackNodeCount = dest - back_node;
	}


	// Fill front list
	if (best_front)
	{
		uint n = 0;
		uint c = 0;
		for (uint i = 0; i < count; i++)
		{
			if (i == best_index)
				continue;

			float d0 = dot(quads[i].Vertices[0].Position, normal) + d;
			float d1 = dot(quads[i].Vertices[1].Position, normal) + d;
			float d2 = dot(quads[i].Vertices[2].Position, normal) + d;
			float d3 = dot(quads[i].Vertices[3].Position, normal) + d;

			bool has_back =  (d0 < 0.0f || d1 < 0.0f || d2 < 0.0f || d3 < 0.0f);
			bool has_front = (d0 > 0.0f || d1 > 0.0f || d2 > 0.0f || d3 > 0.0f);
			bool coplanar = !(has_back || has_front);
			if (coplanar)
				c++;

			if (has_front || (coplanar && c > best_coplanar_back_count))
			{
				if (has_back)
				{
					// Handle split
					uint v = 0;
					for (uint k = 0; k < 4; k++)
					{
						Vertex curr = quads[i].Vertices[k];
						float curr_d = dot(curr.Position, normal) + d;

						if (curr_d > 0.0f)
						{
							new_list[n].Vertices[v] = curr;
							++v;
						}
						else
						{
							Vertex last = quads[i].Vertices[(k + 3) & 3];
							float last_d = dot(last.Position, normal) + d;
							if (last_d > 0.0f)
							{
								float f = last_d / (last_d - curr_d);
								new_list[n].Vertices[v].Position = lerp(last.Position, curr.Position, f);
								new_list[n].Vertices[v].UV = lerp(last.UV, curr.UV, f);
								new_list[n].Vertices[v].Tangent   = curr.Tangent;
								new_list[n].Vertices[v].Bitangent = curr.Bitangent;
								new_list[n].Vertices[v].Normal    = curr.Normal;
								++v;
							}

							Vertex next = quads[i].Vertices[(k + 1) & 3];
							float next_d = dot(next.Position, normal) + d;
							if (next_d > 0.0f)
							{
								float f = curr_d / (curr_d - next_d);
								new_list[n].Vertices[v].Position = lerp(curr.Position, next.Position, f);
								new_list[n].Vertices[v].UV = lerp(curr.UV, next.UV, f);
								new_list[n].Vertices[v].Tangent   = curr.Tangent;
								new_list[n].Vertices[v].Bitangent = curr.Bitangent;
								new_list[n].Vertices[v].Normal    = curr.Normal;
								++v;
							}
						}
					}
					ASSERT(v == 4);
				}
				else
				{
					new_list[n] = quads[i];
				}
				n++;
				ASSERT(n <= best_score);
			}
		}
		ASSERT(n == best_front);

//		uint front_node = dest;

		BuildBSP(vertex_buffer, nodes, dest, new_list, best_front);

//		node->FrontNodeCount = dest - front_node;
	}

	delete [] new_list;
}

void App::moveCamera(const float3 &dir)
{
	BaseApp::moveCamera(dir);

	// Check whether visibility order has changed.

	// First check whether we moved enough to possibly have crossed any plane. If not, no need to do anything.
	float ref_dist = distance(camPos, m_ReferencePos);
	if (ref_dist > m_ClosestPlaneDist)
	{
		m_ReferencePos = camPos;
		m_ClosestPlaneDist = FLT_MAX;

		for (uint i = 0; i < m_PlaneCount; i++)
		{
			float plane_dist = dot(camPos, m_Planes[i].Normal) + m_Planes[i].D;
			bool front_of_plane = (plane_dist > 0.0f);

			if (m_FrontOfPlane[i] != front_of_plane)
			{
				m_FrontOfPlane[i] = front_of_plane;

				m_VisibilityOrderChanged = true;
			}

			plane_dist = fabsf(plane_dist);
			if (plane_dist < m_ClosestPlaneDist)
			{
				m_ClosestPlaneDist = plane_dist;
			}
		}
	}
}

void App::resetCamera()
{
	camPos = vec3(880.0f, 1270.0f, -500.0f);
	wx = 0.39f;
	wy = 0.92f;

	m_ReferencePos = camPos;
	m_ClosestPlaneDist = 0.0f;
	m_VisibilityOrderChanged = true;
}

bool App::init()
{
	// Init GUI components
	int tab = configDialog->addTab("Options");
	configDialog->addWidget(tab, new Label(0, 0, 200, 36, "Opacity"));
	configDialog->addWidget(tab, m_Opacity = new Slider(0, 40, 250, 36, 0.0f, 1.0f, 0.3f));
	configDialog->addWidget(tab, m_Update = new CheckBox(0, 80, 290, 36, "Update visibility order", true));

	return true;
}

void App::exit()
{
}

bool App::initAPI()
{
	return D3D11App::initAPI(D3D11, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_UNKNOWN, max(antiAliasSamples, 1), NO_SETTING_CHANGE);
}

void App::exitAPI()
{
	D3D11App::exitAPI();
}

bool App::load()
{
	initWorkDir(renderer);
	// Shaders
	if ((m_Lighting  = renderer->addShader(ShaderDir("/Lighting.shd"))) == SHADER_NONE) return false;
	if ((m_Skybox    = renderer->addShader(ShaderDir("/Skybox.shd"))) == SHADER_NONE) return false;
	if ((m_Traverse  = renderer->addShader(ShaderDir("/Traverse.shd"))) == SHADER_NONE) return false;

	// Samplerstates
	if ((m_TrilinearAniso = renderer->addSamplerState(TRILINEAR_ANISO, WRAP, WRAP, CLAMP)) == SS_NONE) return false;
	if ((m_TrilinearClamp = renderer->addSamplerState(TRILINEAR, CLAMP, CLAMP, CLAMP)) == SS_NONE) return false;

	// Textures
	{
		const std::string strFiles[] = {
			ResDir("/Textures/Glass-0376.dds"),
			ResDir("/Textures/Lightpanel.dds"),
			ResDir("/Textures/Window.dds"),
		};

		const char* filenames[] = {
			strFiles[0].c_str(),
			strFiles[1].c_str(),
			strFiles[2].c_str()
		};

		Image tex_array;
		if (!tex_array.loadSlicedImage(filenames, 1, elementsOf(filenames))) return false;
		if ((m_Base = renderer->addTexture(tex_array, m_TrilinearAniso, SRGB)) == TEXTURE_NONE) return false;
	}

	{

		const std::string strFiles[] = {
			ResDir("/Textures/Glass-0376Bump.dds"),
			ResDir("/Textures/LightpanelBump.dds"),
			ResDir("/Textures/WindowBump.dds"),
		};

		const char* filenames[] = {
			strFiles[0].c_str(),
			strFiles[1].c_str(),
			strFiles[2].c_str()
		};

		Image tex_array;
		if (!tex_array.loadSlicedImage(filenames, 1, elementsOf(filenames))) return false;
		if ((m_Bump = renderer->addTexture(tex_array, m_TrilinearAniso)) == TEXTURE_NONE) return false;
	}

	const std::string strFiles[] = {
		ResDir("/Textures/CubeMaps/Plains-of-abraham/posx.jpg"),
		ResDir("/Textures/CubeMaps/Plains-of-abraham/negx.jpg"),
		ResDir("/Textures/CubeMaps/Plains-of-abraham/posy.jpg"),
		ResDir("/Textures/CubeMaps/Plains-of-abraham/negy.jpg"),
		ResDir("/Textures/CubeMaps/Plains-of-abraham/posz.jpg"),
		ResDir("/Textures/CubeMaps/Plains-of-abraham/negz.jpg"),
	};

	const char *cube_files[] = {
		strFiles[0].c_str(),
		strFiles[1].c_str(),
		strFiles[2].c_str(),
		strFiles[3].c_str(),
		strFiles[4].c_str(),
		strFiles[5].c_str()
	};

	if ((m_Environment = renderer->addCubemap(cube_files, true, m_TrilinearClamp, 1, SRGB)) == TEXTURE_NONE) return false;

	// Blendstates
	if ((m_AlphaBlend = renderer->addBlendState(ONE, ONE_MINUS_SRC_ALPHA)) == BS_NONE) return false;

	const FormatDesc vertex_format[] =	{
		{ 0, TYPE_VERTEX,   FORMAT_FLOAT, 3 },
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, 3 },
		{ 0, TYPE_TANGENT,  FORMAT_FLOAT, 3 },
		{ 0, TYPE_BINORMAL, FORMAT_FLOAT, 3 },
		{ 0, TYPE_NORMAL,   FORMAT_FLOAT, 3 },
	};
	if ((m_VertexFormat = renderer->addVertexFormat(vertex_format, elementsOf(vertex_format), m_Lighting)) == VF_NONE) return false;


	// Read in the source model
	Model map;
	if (!map.loadT3d(ResDir("/Models/Map.t3d"))) return false;
	map.scale(0, float3(-1, 1, 1));
	map.scale(1, float2(0.5f, 0.5f));

	uint nIndices = map.getIndexCount();
	const float3* pos = (const float3*) map.getStream(0).vertices;
	const float2* uvs = (const float2*) map.getStream(1).vertices;

	const uint* pos_inds = map.getStream(0).indices;
	const uint* uv_inds  = map.getStream(1).indices;

	const uint batch1start = map.getBatch(1).startIndex;
	const uint batch2start = map.getBatch(2).startIndex;

	Quad src_quads[278]; // Hardcoded size for this model
	uint index = 0;

	for (uint i = 0; i < nIndices; i += 6) {
		float3 v0 = pos[pos_inds[i]];
		float3 v1 = pos[pos_inds[i + 1]];
		float3 v2 = pos[pos_inds[i + 4]];
		float3 v3 = pos[pos_inds[i + 5]];

		float2 uv0 = uvs[uv_inds[i]];
		float2 uv1 = uvs[uv_inds[i + 1]];
		float2 uv2 = uvs[uv_inds[i + 4]];
		float2 uv3 = uvs[uv_inds[i + 5]];

		float3 t0, t1, normal;
		tangentVectors(v0, v1, v3, uv0, uv1, uv3, t0, t1, normal);
		t0 = normalize(t0);
		t1 = normalize(t1);


		float batch = (i < batch1start)? 0.0f : (i < batch2start)? 1.0f : 2.0f;

		src_quads[index].Vertices[0].Position = v0;
		src_quads[index].Vertices[1].Position = v1;
		src_quads[index].Vertices[2].Position = v2;
		src_quads[index].Vertices[3].Position = v3;

		src_quads[index].Vertices[0].UV = float3(uv0, batch);
		src_quads[index].Vertices[1].UV = float3(uv1, batch);
		src_quads[index].Vertices[2].UV = float3(uv2, batch);
		src_quads[index].Vertices[3].UV = float3(uv3, batch);

		src_quads[index].Vertices[0].Tangent   = t0;
		src_quads[index].Vertices[0].Bitangent = t1;
		src_quads[index].Vertices[0].Normal    = normal;
		src_quads[index].Vertices[1].Tangent   = t0;
		src_quads[index].Vertices[1].Bitangent = t1;
		src_quads[index].Vertices[1].Normal    = normal;
		src_quads[index].Vertices[2].Tangent   = t0;
		src_quads[index].Vertices[2].Bitangent = t1;
		src_quads[index].Vertices[2].Normal    = normal;
		src_quads[index].Vertices[3].Tangent   = t0;
		src_quads[index].Vertices[3].Bitangent = t1;
		src_quads[index].Vertices[3].Normal    = normal;

		index++;
	}
	ASSERT(index == elementsOf(src_quads));


	// Construct the BSP tree
	Quad dest_quads[MAX_NODE_COUNT];

	m_BSPNodeCount = 0;
	BuildBSP(dest_quads, m_BSP, m_BSPNodeCount, src_quads, elementsOf(src_quads));

	ASSERT(m_BSPNodeCount <= MAX_NODE_COUNT);

	if ((m_VertexBuffer = renderer->addVertexBuffer(m_BSPNodeCount * sizeof(Quad), STATIC, dest_quads)) == VB_NONE) return false;


	// Find all unique separating planes from the BSP tree
	m_PlaneCount = 0;
	for (uint i = 0; i < m_BSPNodeCount; i++)
	{
		float3 normal = m_BSP[i].Normal;
		float d = m_BSP[i].D;

		bool found = false;
		for (uint j = 0; j < m_PlaneCount; j++)
		{
			if (normal == m_Planes[j].Normal && d == m_Planes[j].D)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			m_Planes[m_PlaneCount].Normal = normal;
			m_Planes[m_PlaneCount].D      = d;
			m_PlaneCount++;
			ASSERT(m_PlaneCount <= MAX_PLANE_COUNT);
		}
	}

	memset(m_FrontOfPlane, 0, sizeof(m_FrontOfPlane));



#ifdef USE_COMPUTE
	{
		D3D11_BUFFER_DESC desc = {};
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.ByteWidth = 6 * m_BSPNodeCount * sizeof(uint16);
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER | D3D11_BIND_UNORDERED_ACCESS;
		if (FAILED(device->CreateBuffer(&desc, NULL, &m_D3DIndexBuffer)))
			return false;

		D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
		uav_desc.Format = DXGI_FORMAT_R16G16_UINT;
		uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uav_desc.Buffer.NumElements = 3 * m_BSPNodeCount;
		if (FAILED(device->CreateUnorderedAccessView(m_D3DIndexBuffer, &uav_desc, &m_D3DIndexBufferUAV)))
			return false;

		if ((m_IndexBuffer = ((Direct3D11Renderer*) renderer)->addIndexBuffer(m_D3DIndexBuffer, desc.ByteWidth / 2, 2)) == IB_NONE) return false;
	}

	{
		D3D11_BUFFER_DESC desc = {};
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.ByteWidth = m_BSPNodeCount * sizeof(BSPNode);
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.StructureByteStride = sizeof(BSPNode);

		D3D11_SUBRESOURCE_DATA data = { &m_BSP };
		if (FAILED(device->CreateBuffer(&desc, &data, &m_BSPBuffer)))
			return false;

		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srv_desc.Buffer.NumElements = m_BSPNodeCount;
		if (FAILED(device->CreateShaderResourceView(m_BSPBuffer, &srv_desc, &m_BSPBufferSRV)))
			return false;
	}

#else
	if ((m_IndexBuffer = renderer->addIndexBuffer(6 * m_BSPNodeCount, 2, DYNAMIC)) == IB_NONE) return false;

#endif

	return true;
}

void App::unload()
{
#ifdef USE_COMPUTE
	m_D3DIndexBufferUAV->Release();
	m_BSPBufferSRV->Release();
	m_BSPBuffer->Release();
#endif
}

void App::drawFrame()
{
	if (m_VisibilityOrderChanged && m_Update->isChecked())
	{
		m_VisibilityOrderChanged = false;

#ifdef USE_COMPUTE

		renderer->reset();
		renderer->setShader(m_Traverse);
		renderer->setShaderConstant3f("CamPos", camPos);
		renderer->setShaderConstant1i("MaxNode", m_BSPNodeCount - 1);
		renderer->apply();

		context->CSSetUnorderedAccessViews(0, 1, &m_D3DIndexBufferUAV, nullptr);
		context->CSSetShaderResources(0, 1, &m_BSPBufferSRV);

		uint32 num_thread_groups = (m_BSPNodeCount + 63) / 64;
		context->Dispatch(num_thread_groups, 1, 1);

		ID3D11UnorderedAccessView* null_uav = nullptr;
		context->CSSetUnorderedAccessViews(0, 1, &null_uav, nullptr);

		ID3D11ShaderResourceView* null_srv = nullptr;
		context->CSSetShaderResources(0, 1, &null_srv);

#else
		ID3D11Resource* d3d_ib = ((Direct3D11Renderer*) renderer)->getIBResource(m_IndexBuffer);

		D3D11_MAPPED_SUBRESOURCE map;
		context->Map(d3d_ib, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

		uint16* indices = (uint16*) map.pData;

		for (uint i = 0; i < m_BSPNodeCount; i++)
		{
			uint curr = 0;

			uint first = 0;
			uint last = m_BSPNodeCount - 1;

			while (true)
			{
				float d = dot(camPos, m_BSP[curr].Normal) + m_BSP[curr].D;
				if (d > 0)
				{
					uint curr_node_position = first + m_BSP[curr].BackNodeCount;
					if (i == curr_node_position)
						break;

					if (i < curr_node_position)
					{
						curr++;
						last = curr_node_position - 1;
					}
					else
					{
						curr = curr + m_BSP[curr].BackNodeCount + 1; // Move pointer to front tree by skipping all nodes in back tree
						first = curr_node_position + 1;
					}
				}
				else
				{
					uint curr_node_position = last - m_BSP[curr].BackNodeCount;
					if (i == curr_node_position)
						break;

					if (i < curr_node_position)
					{
						curr = curr + m_BSP[curr].BackNodeCount + 1; // Move pointer to front tree by skipping all nodes in back tree
						last = curr_node_position - 1;
					}
					else
					{
						curr++;
						first = curr_node_position + 1;
					}
				}
			}

			uint index = curr * 4;

			indices[i * 6 + 0] = index;
			indices[i * 6 + 1] = index + 1;
			indices[i * 6 + 2] = index + 2;

			indices[i * 6 + 3] = index;
			indices[i * 6 + 4] = index + 2;
			indices[i * 6 + 5] = index + 3;
		}
		context->Unmap(d3d_ib, 0);
#endif
	}

	const float near_plane = 10.0f;
	const float far_plane = 20000.0f;

	// Reversed depth
	float4x4 projection = toD3DProjection(perspectiveMatrixY(1.2f, width, height, far_plane, near_plane));
	float4x4 view = rotateXY(-wx, -wy);
	float4x4 inv_vp_env = !(projection * view);
	view.translate(-camPos);
	float4x4 view_proj = projection * view;

	renderer->clear(true, false, false, float4(0, 0, 0, 0), 0.0f);

	/*
		Skybox
	*/
	renderer->reset();
	renderer->setRasterizerState(cullNone);
	renderer->setShader(m_Skybox);
	renderer->setShaderConstant4x4f("InvMvp", inv_vp_env);
	renderer->setTexture("Env", m_Environment);
	renderer->setSamplerState("Filter", m_TrilinearClamp);
	renderer->setDepthState(noDepthTest);
	renderer->apply();

	renderer->drawArrays(PRIM_TRIANGLES, 0, 3);

	/*
		Main lighting pass.
	*/
	renderer->reset();
	renderer->setRasterizerState(cullNone);
	renderer->setShader(m_Lighting);
	renderer->setShaderConstant4x4f("ViewProj", view_proj);
	renderer->setShaderConstant3f("CamPos", camPos);
	renderer->setShaderConstant1f("Opacity", m_Opacity->getValue());
	renderer->setTexture("Base", m_Base);
	renderer->setTexture("Bump", m_Bump);
	renderer->setSamplerState("Filter", m_TrilinearAniso);
	renderer->setDepthState(noDepthTest);
	renderer->setBlendState(m_AlphaBlend);
	renderer->setVertexFormat(m_VertexFormat);
	renderer->setVertexBuffer(0, m_VertexBuffer);
	renderer->setIndexBuffer(m_IndexBuffer);
	renderer->apply();

	renderer->drawElements(PRIM_TRIANGLES, 0, 6 * m_BSPNodeCount, 0, 4 * m_BSPNodeCount);
}
