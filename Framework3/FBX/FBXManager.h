#ifndef FBX_MANAGER_H
#define FBX_MANAGER_H

#include "FbxModel.h"

namespace fbxsdk {
	class FbxManager;
	class FbxScene;
	class FbxNode;
	class FbxNodeAttribute;
	class FbxMesh;
}

class FBXSimpleManager
{
protected:
	FBXSimpleManager();
	bool init();
	bool loadNode(fbxsdk::FbxNode* node, FbxModel& model, float scale);
	bool loadAttributes(fbxsdk::FbxNodeAttribute* nodeAtt, FbxModel& model, float scale);
	bool loadAttributes_mesh(fbxsdk::FbxNodeAttribute* nodeAtt, FbxModel& model,float scale);
	bool loadElement_normal(fbxsdk::FbxMesh* mesh, FbxModel& model);
	bool loadElement_uv(fbxsdk::FbxMesh* mesh, FbxModel& model,int uvIndex = 0);
public:
	static FBXSimpleManager* getInstance();
	static void destory();
	bool load(const char* fileName, FbxModel& model,float scale);
protected:
	fbxsdk::FbxManager* m_fbxMgr;
	static FBXSimpleManager* m_instance;
};


#endif
