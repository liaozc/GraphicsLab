#include "FbxModel.h"
#include "FBXManager.h"

FbxModel::FbxModel()
{
	m_model = nullptr;

}

FbxModel::~FbxModel()
{
	for (uint i = 0; i < m_children.getCount(); ++i) {
		delete m_children[i];
	}
	m_children.clear();
	if (m_model)
		delete m_model;
}

void FbxModel::loadFbx(const char * filename, float scale)
{
	FBXSimpleManager* fbxSMgr = FBXSimpleManager::getInstance();
	fbxSMgr->load(filename,*this, scale);
}

uint FbxModel::makeDrawable(Renderer * renderer, const bool useCache, const ShaderID shader)
{
	for (uint i = 0; i < m_children.getCount(); ++i) {
		m_children[i]->makeDrawable(renderer, useCache, shader);
	}
	if (!m_model) {
		m_model = new Model();
		if (m_verts.getCount() != 0) {//add position stream
			vec3* positions = new vec3[m_verts.getCount()];
			memcpy(positions, m_verts.getArray(), sizeof(vec3) * m_verts.getCount());
			uint* inds = new uint[m_inds.getCount()];
			memcpy(inds, m_inds.getArray(), sizeof(uint) * m_inds.getCount());
			m_model->addStream(TYPE_VERTEX, 3, m_verts.getCount(), (float*)positions, inds, false);
			m_model->setIndexCount(m_inds.getCount());
		}
		if (m_normals.getCount() != 0) {//add normal stream
			//currently,we don't split the vert by normal(but by uv btw),instead we merge the normals on the vert.
			//so we can use the same inds as vert
			vec3* normals = new vec3[m_normals.getCount()];
			memcpy(normals, m_normals.getArray(), sizeof(vec3) * m_normals.getCount());
			uint* inds = new uint[m_inds.getCount()];
			memcpy(inds, m_inds.getArray(), sizeof(uint) * m_inds.getCount());
			m_model->addStream(TYPE_NORMAL, 3, m_normals.getCount(), (float*)normals, inds, false);
		}
		if (m_texcoord0s.getCount() != 0) {
			vec2* uvs = new vec2[m_texcoord0s.getCount()];
			memcpy(uvs, m_texcoord0s.getArray(), sizeof(vec2) * m_texcoord0s.getCount());
			uint* inds = new uint[m_texcoodIndis.getCount()];
			memcpy(inds, m_texcoodIndis.getArray(), sizeof(uint) * m_texcoodIndis.getCount());
			m_model->addStream(TYPE_TEXCOORD, 2, m_texcoord0s.getCount(), (float*)uvs, inds, false);
		}
		m_material.m_shader = shader;
		bool drawable = m_verts.getCount() > 0;
		return drawable ? m_model->makeDrawable(renderer, useCache, shader) : 0;
	}

	return 0;
}

void FbxModel::draw(Renderer * renderer)
{
	if(m_model)
		m_model->draw(renderer);
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

bool FbxModel::isEmpty() const
{
	return vertCount() == 0;
}
