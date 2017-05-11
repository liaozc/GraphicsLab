
#include "FBXManager.h"
#include "fbxsdk/include/fbxsdk.h"
using namespace fbxsdk;

FBXSimpleManager* FBXSimpleManager::m_instance = nullptr;

FBXSimpleManager::FBXSimpleManager()
{
	m_fbxMgr = nullptr;
}

bool FBXSimpleManager::init()
{
	m_fbxMgr = fbxsdk::FbxManager::Create();
#ifdef _DEBUG
	const char* version = m_fbxMgr->GetVersion();
	printf("fbx version :%s \n", version);
#endif
	FbxIOSettings* ios = fbxsdk::FbxIOSettings::Create(m_fbxMgr, IOSROOT);
	m_fbxMgr->SetIOSettings(ios);
	return true;
}


FBXSimpleManager * FBXSimpleManager::getInstance()
{
	if (!m_instance) {
		m_instance = new FBXSimpleManager();
		m_instance->init();
	}
	return m_instance;
}

void FBXSimpleManager::destory()
{
	m_instance->m_fbxMgr->Destroy();
	delete m_instance;
	m_instance = nullptr;
}

bool FBXSimpleManager::loadNode(fbxsdk::FbxNode * node, FbxModel & model, float scale)
{
	int attributeCount = node->GetNodeAttributeCount();
	for (int i = 0; i < attributeCount; ++i) {
		fbxsdk::FbxNodeAttribute* nodeAtt = node->GetNodeAttributeByIndex(i);
		loadAttributes(nodeAtt, model, scale);
	}
	//process children
	for (int i = 0; i<node->GetChildCount(); i++) {
		FbxModel* subModel = new FbxModel();
		loadNode(node->GetChild(i), *subModel, scale);
		model.m_children.add(subModel);
	}

	return false;
}


bool FBXSimpleManager::loadAttributes(fbxsdk::FbxNodeAttribute* nodeAtt, FbxModel & model,float scale)
{
	if (nodeAtt == NULL) return false;
	switch (nodeAtt->GetAttributeType()) {
	case fbxsdk::FbxNodeAttribute::eMarker:                  break;
	case fbxsdk::FbxNodeAttribute::eSkeleton:                break;
	case fbxsdk::FbxNodeAttribute::eMesh:
		loadAttributes_mesh(nodeAtt,model, scale);
		break;
	case fbxsdk::FbxNodeAttribute::eCamera:                  break;
	case fbxsdk::FbxNodeAttribute::eLight:                   break;
	case fbxsdk::FbxNodeAttribute::eBoundary:                break;
	case fbxsdk::FbxNodeAttribute::eOpticalMarker:          break;
	case fbxsdk::FbxNodeAttribute::eOpticalReference:       break;
	case fbxsdk::FbxNodeAttribute::eCameraSwitcher:         break;
	case fbxsdk::FbxNodeAttribute::eNull:                    break;
	case fbxsdk::FbxNodeAttribute::ePatch:                   break;
	case fbxsdk::FbxNodeAttribute::eNurbs:                    break;
	case fbxsdk::FbxNodeAttribute::eNurbsSurface:           break;
	case fbxsdk::FbxNodeAttribute::eNurbsCurve:             break;
	case fbxsdk::FbxNodeAttribute::eCameraStereo:      break;
	case fbxsdk::FbxNodeAttribute::eShape:      break;
	case fbxsdk::FbxNodeAttribute::eLODGroup:      break;
	case fbxsdk::FbxNodeAttribute::eSubDiv:      break;
	case fbxsdk::FbxNodeAttribute::eCachedEffect:      break;
	case fbxsdk::FbxNodeAttribute::eLine:      break;
	case fbxsdk::FbxNodeAttribute::eUnknown: break;
	}
	return true;
}

bool FBXSimpleManager::loadAttributes_mesh(fbxsdk::FbxNodeAttribute * nodeAtt, FbxModel & model, float scale)
{
	fbxsdk::FbxMesh *mesh = static_cast<fbxsdk::FbxMesh*>(nodeAtt);
	if (!mesh->IsTriangleMesh()){
		fbxsdk::FbxGeometryConverter converter(m_fbxMgr);
		nodeAtt = converter.Triangulate(nodeAtt,true);
		mesh = static_cast<fbxsdk::FbxMesh*>(nodeAtt);
	}
	//get vertex
	int vertCount = mesh->GetControlPointsCount();
	for (int i = 0; i < vertCount; ++i) {
		fbxsdk::FbxVector4 pos = mesh->GetControlPointAt(i);
		vec3 position = vec3(pos.mData[0], pos.mData[1], pos.mData[2]) * scale;
		model.m_verts.add(position);
	}
	//get indices
	int indexCount = mesh->GetPolygonVertexCount();
	int* inds = mesh->GetPolygonVertices();
	for (int i = 0; i < indexCount; ++i) {
		ushort index = (ushort)inds[i];
		model.m_inds.add(index);
	}
	//elements
	// -- normals
		loadElement_normal(mesh, model);
	// -- texcoord0
		loadElement_uv(mesh, model, 0);


#if 1
	//make the right-handeness coord to d3d
	for (int i = 0; i < vertCount; ++i) {
		vec3 X(1, 0, 0);
		vec3 Y(0, 0, 1);
		vec3 Z(0, 1, 0);
		vec3 newPos;
		newPos.x = dot(model.m_verts[i], X);
		newPos.y = dot(model.m_verts[i], Y);
		newPos.z = dot(model.m_verts[i], Z);
		vec3 newNormal;
		newNormal.x = dot(model.m_normals[i], X);
		newNormal.y = dot(model.m_normals[i], Y);
		newNormal.z = dot(model.m_normals[i], Z);
		model.m_verts[i] = newPos;
		model.m_normals[i] = newNormal;
	}
	for (int i = 0; i < indexCount; i += 3) {
		ushort temp = model.m_inds[i];
		model.m_inds[i] = model.m_inds[i + 2];
		model.m_inds[i + 2] = temp;
	}
#endif
	return false;
}

bool FBXSimpleManager::loadElement_normal(fbxsdk::FbxMesh* mesh, FbxModel & model)
{
	fbxsdk::FbxGeometryElementNormal* pNormals = nullptr;
	pNormals = mesh->GetElementNormal();
	if (!pNormals) return false;

	model.m_normals.setCount(mesh->GetControlPointsCount());
	memset(model.m_normals.getArray(), 0, sizeof(vec3) * mesh->GetControlPointsCount());
	
	fbxsdk::FbxLayerElement::EMappingMode mapMode = pNormals->GetMappingMode();
	bool useIndex = pNormals->GetReferenceMode() != FbxGeometryElement::eDirect;
	uint polyCount = mesh->GetPolygonCount();
	
	if (mapMode == fbxsdk::FbxLayerElement::eByControlPoint) {
		for (uint i = 0; i < polyCount; ++i) {
			const int polySize = mesh->GetPolygonSize(i);
			for (int vi = 0; vi < polySize; ++vi){
				int ci = mesh->GetPolygonVertex(i, vi);//get the index of the current vertex in control points array
				int nIndex = useIndex ? pNormals->GetIndexArray().GetAt(ci) : ci; //the UV index depends on the reference mode
				FbxVector4 normal = pNormals->GetDirectArray().GetAt(nIndex);
				model.m_normals[ci] = vec3(normal.mData[0], normal.mData[1], normal.mData[2]);
			}
		}
	}
	else if (mapMode == fbxsdk::FbxLayerElement::eByPolygonVertex) {
		int polyIndexCounter = 0;
		int uvCount = pNormals->GetDirectArray().GetCount();
		for (int i = 0; i < polyCount; i ++ )	{
			const int polySize = mesh->GetPolygonSize(i);
			for (int vi = 0; vi < polySize; vi ++) {
				int nIndex = useIndex ? pNormals->GetIndexArray().GetAt(polyIndexCounter) : polyIndexCounter;
				FbxVector4 normal = pNormals->GetDirectArray().GetAt(nIndex);
				int ci = mesh->GetPolygonVertex(i, vi);
				model.m_normals[ci] += vec3(normal.mData[0], normal.mData[1], normal.mData[2]);
				polyIndexCounter++;
			}
		}
	
	}
	return true;
}

bool FBXSimpleManager::loadElement_uv(fbxsdk::FbxMesh * mesh, FbxModel & model, int uvIndex)
{
	fbxsdk::FbxGeometryElementUV* pUVs = nullptr;
	pUVs = mesh->GetElementUV(uvIndex);
	if (!pUVs) return false;
	Array<vec2>* texCoords = uvIndex == 0 ? &model.m_texcoord0s : uvIndex == 1 ? &model.m_texcoord1s : nullptr;
	if (!texCoords) return false;
	texCoords->setCount(mesh->GetControlPointsCount());
	memset(texCoords->getArray(), 0, sizeof(vec2) * mesh->GetControlPointsCount());

	fbxsdk::FbxLayerElement::EMappingMode mapMode = pUVs->GetMappingMode();
	bool useIndex = pUVs->GetReferenceMode() != FbxGeometryElement::eDirect;
	uint polyCount = mesh->GetPolygonCount();

	if (mapMode == fbxsdk::FbxLayerElement::eByControlPoint) {
		for (uint i = 0; i < polyCount; ++i) {
			const int polySize = mesh->GetPolygonSize(i);
			for (int vi = 0; vi < polySize; ++vi) {
				int ci = mesh->GetPolygonVertex(i, vi);//get the index of the current vertex in control points array
				int uvIndex = useIndex ? pUVs->GetIndexArray().GetAt(ci) : ci; //the UV index depends on the reference mode
				FbxVector2 uv = pUVs->GetDirectArray().GetAt(uvIndex);
				(*texCoords)[ci] = vec2(uv.mData[0], uv.mData[1]);
			}
		}
	}
	else if (mapMode == fbxsdk::FbxLayerElement::eByPolygonVertex) {
		int polyIndexCounter = 0;
		for (int i = 0; i < polyCount; i++) {
			const int polySize = mesh->GetPolygonSize(i);
			for (int vi = 0; vi < polySize; vi++) {
				int uvIndex = useIndex ? pUVs->GetIndexArray().GetAt(polyIndexCounter) : polyIndexCounter;
				FbxVector2 uv = pUVs->GetDirectArray().GetAt(uvIndex);
				int ci = mesh->GetPolygonVertex(i, vi);
				(*texCoords)[ci] = vec2(uv.mData[0], uv.mData[1]);
				polyIndexCounter++;
			}
		}
	}
	return true;
}

bool FBXSimpleManager::load(const char * fileName, FbxModel & model,float scale) 
{
	FbxScene* scene = FbxScene::Create(m_fbxMgr, "");
	FbxImporter* sceneImporter = FbxImporter::Create(m_fbxMgr, "");
	sceneImporter->Initialize(fileName, -1, m_fbxMgr->GetIOSettings());
	sceneImporter->Import(scene);
	sceneImporter->Destroy();
	//fill the model
	fbxsdk::FbxNode* rootNode = scene->GetRootNode();
	loadNode(rootNode, model,scale);
	scene->Destroy(true);

	return true;
}

