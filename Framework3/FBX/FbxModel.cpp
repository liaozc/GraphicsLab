#include "FbxModel.h"
#include "FBXManager.h"



void FbxMaterial::Apply(Renderer * renderer,mat4 world)
{
	renderer->reset();
	renderer->setRasterizerState(m_cull);
	renderer->setShader(m_shader);
	renderer->setShaderConstant4x4f("worldMatrix", world);
	renderer->setShaderConstant4x4f("viewProj", renderer->getViewProj());
	renderer->setTexture("tex0", m_texs[0]);
	renderer->setSamplerState("filter0", m_filters[0]);
	renderer->apply();
}



////////////////////model/////////////////////

FbxModel::FbxModel()
{
	m_parent = nullptr;
	m_world = identity4();
}

FbxModel::~FbxModel()
{
	for (uint i = 0; i < m_children.getCount(); ++i) {
		delete m_children[i];
	}
	m_children.clear();
}

void FbxModel::loadFbx(const char * filename)
{
	FBXSimpleManager* fbxSMgr = FBXSimpleManager::getInstance();
	fbxSMgr->load(filename,*this);
}

void FbxModel::preAssemble()
{
	//split the mesh into material count
	for (int i = 0; i < m_mats.getCount(); ++i) {
		//cal. how may tris there are in the mat
		int triCount = m_tris.getCount();
		int subTriCount = 0;
		for (int j = 0; j < triCount; ++j) {
			if (m_tris[j].matIndex == i) {
				subTriCount++;
			}
		}
		if (subTriCount == 0) continue;
		uint* polyInds = new uint[subTriCount * 3];
		uint* normalInds = new uint[subTriCount * 3];
		uint* texInds = new uint[subTriCount * 3];
		int index = 0;
		for (int j = 0; j < triCount; ++j) {
			if (m_tris[j].matIndex == i) {
				//pos
				polyInds[3 * index] = m_tris[j].posIndex0;
				polyInds[3 * index + 1] = m_tris[j].posIndex1;
				polyInds[3 * index + 2] = m_tris[j].posIndex2;
				//normal
				normalInds[3 * index] = m_tris[j].normalIndex0;
				normalInds[3 * index + 1] = m_tris[j].normalIndex1;
				normalInds[3 * index + 2] = m_tris[j].normalIndex2;
				//tex
				texInds[3 * index] = m_tris[j].tex0Index0;
				texInds[3 * index + 1] = m_tris[j].tex0Index1;
				texInds[3 * index + 2] = m_tris[j].tex0Index2;
				index++;
			}
		}
		vec3* verts = new vec3[m_verts.getCount()];
		memcpy(verts, m_verts.getArray(), sizeof(vec3) * m_verts.getCount());
		vec3* normals = new vec3[m_normals.getCount()];
		memcpy(normals, m_normals.getArray(), sizeof(vec3) * m_normals.getCount());
		vec2* texes = new vec2[m_texcoord0s.getCount()];
		memcpy(texes, m_texcoord0s.getArray(), sizeof(vec2) * m_texcoord0s.getCount());
		Model* model = new Model();
		model->setIndexCount(subTriCount * 3);
		model->addStream(TYPE_VERTEX, 3, m_verts.getCount(), (float*)verts, polyInds, false);
		model->addStream(TYPE_NORMAL, 3, m_normals.getCount(), (float*)normals, normalInds, false);
		model->addStream(TYPE_TEXCOORD, 2, m_texcoord0s.getCount(), (float*)texes, texInds, false);
		m_models.add(model);
	}

}

uint FbxModel::makeDrawable(Renderer * renderer, const bool useCache, const ShaderID shader)
{
	preAssemble();
	uint count = 0;
	for (uint i = 0; i < m_models.getCount(); ++i) {
		count += m_models[i]->makeDrawable(renderer, useCache, shader);
		//temp mat impenlented.
		m_mats[i].m_texs[0] = renderer->addTexture(m_mats[i].m_texName, true, SS_NONE);
		m_mats[i].m_filters[0] = renderer->addSamplerState(LINEAR, WRAP, WRAP, WRAP);
		m_mats[i].m_cull = renderer->addRasterizerState(CULL_BACK);
		m_mats[i].m_shader = shader;
	}
	for (uint i = 0; i < m_children.getCount(); ++i) {
		count += m_children[i]->makeDrawable(renderer, useCache, shader);
	}
	return count;
}

void FbxModel::draw(Renderer * renderer)
{
	for (uint i = 0; i < m_models.getCount(); ++i) {
		mat4 world = m_world;
		FbxModel* parent = m_parent;
		while (parent) {
			world = world * parent->m_world;
			parent = parent->m_parent;
		}
		m_mats[i].Apply(renderer,world);
		m_models[i]->draw(renderer);
	}
	for (uint i = 0; i < m_children.getCount(); ++i) {
		m_children[i]->draw(renderer);
	}
}

int FbxModel::vertCount() const
{
	int vertCount = 0;
	for (uint i = 0; i < m_children.getCount(); ++i) {
		 vertCount += m_children[i]->vertCount();
	}
	vertCount += m_verts.getCount();
	return vertCount;
}

void FbxModel::setWorld(mat4 world)
{
	m_world = world;
}

bool FbxModel::isEmpty() const
{
	return vertCount() == 0;
}

