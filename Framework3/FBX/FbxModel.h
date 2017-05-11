#ifndef FBX_MODEL_H
#define FBX_MODEL_H

#include "../Config.h"
#include "../Util/Array.h"
#include "../Math/Vector.h"
#include "../Renderer.h"
#include "../Util/Model.h"	//we will use the stream merge functionality

class FbxMaterial {
public:
	FbxMaterial() 
	{
		m_texs[0] = m_texs[1] = m_texs[2] = m_texs[3] = TEXTURE_NONE;
	}
	void Apply(Renderer* renderer)
	{

	}

	TextureID m_texs[4];
	ShaderID m_shader;
};


class FbxModel 
{
public:
	FbxModel();
	~FbxModel();
	void loadFbx(const char* filename,float scale = 1.0f);
	uint makeDrawable(Renderer *renderer, const bool useCache = true, const ShaderID shader = SHADER_NONE);
	void draw(Renderer *renderer);
	bool isEmpty() const;
	int	vertCount() const;
public:
	//mesh
	Array <vec3> m_verts;
	Array <uint> m_inds;
	Array <vec3> m_normals;
	Array <vec2> m_texcoord0s;
	Array <vec2> m_texcoord1s;
	Array <uint> m_texcoodIndis;
	// model
	Model* m_model;
	//Material
	FbxMaterial m_material;

	//children
	Array<FbxModel*> m_children; 
};


#endif