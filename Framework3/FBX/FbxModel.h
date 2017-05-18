#ifndef FBX_MODEL_H
#define FBX_MODEL_H

#include "../Config.h"
#include "../Util/Array.h"
#include "../Math/Vector.h"
#include "../Renderer.h"
#include "../Util/Model.h"	//we will use the stream merge functionality
#include "../Util/String.h"

class FbxMaterial {
public:
	FbxMaterial() 
	{
		m_texs[0] = m_texs[1] = m_texs[2] = m_texs[3] = TEXTURE_NONE;
		m_filters[0] = m_filters[1] = m_filters[2] = m_filters[3] = SS_NONE;
		m_texName = "";
	}
	void Apply(Renderer* renderer,mat4 world);
	TextureID m_texs[4];
	SamplerStateID m_filters[4];
	ShaderID m_shader;
	RasterizerStateID m_cull;
	String m_texName;
};


struct FbxTriangle
{
	int posIndex0;
	int posIndex1;
	int posIndex2;
	union {
		struct {
			int normalIndex0;
			int normalIndex1;
			int normalIndex2;
		};
		int normals[3];
	};
	union {
		struct {
			int tex0Index0;
			int tex0Index1;
			int tex0Index2;
		};
		int tex0Inds[3];
	};
	int matIndex;
};

class FbxModel 
{
protected:
	void preAssemble();
public:
	FbxModel();
	~FbxModel();
	void loadFbx(const char* filename);
	uint makeDrawable(Renderer *renderer, const bool useCache = true, const ShaderID shader = SHADER_NONE);
	void draw(Renderer *renderer);
	bool isEmpty() const;
	int	vertCount() const;
	void setWorld(mat4 world);
public:
	//mesh
	Array <vec3> m_verts;
	Array <vec3> m_normals;
	Array <vec2> m_texcoord0s;
	Array <FbxTriangle> m_tris;
	Array <FbxMaterial> m_mats;
	Array <Model*> m_models;
	//children
	Array<FbxModel*> m_children; 
	FbxModel* m_parent;
	mat4 m_world;
};


#endif