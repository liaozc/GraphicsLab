
#include "FBXManager.h"
#include "fbxsdk/include/fbxsdk.h"
#include <string>
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

bool FBXSimpleManager::loadNode(fbxsdk::FbxNode * node, FbxModel & model)
{
	int attributeCount = node->GetNodeAttributeCount();
	for (int i = 0; i < attributeCount; ++i) {
		fbxsdk::FbxNodeAttribute* nodeAtt = node->GetNodeAttributeByIndex(i);
		loadAttributes(nodeAtt, model);
	}
	//process children
	for (int i = 0; i<node->GetChildCount(); i++) {
		FbxModel* subModel = new FbxModel();
		loadNode(node->GetChild(i), *subModel);
		subModel->m_parent = &model;
		model.m_children.add(subModel);
	}

	return false;
}


bool FBXSimpleManager::loadAttributes(fbxsdk::FbxNodeAttribute* nodeAtt, FbxModel & model)
{
	if (nodeAtt == NULL) return false;
	switch (nodeAtt->GetAttributeType()) {
	case fbxsdk::FbxNodeAttribute::eMarker:                  break;
	case fbxsdk::FbxNodeAttribute::eSkeleton:                break;
	case fbxsdk::FbxNodeAttribute::eMesh:
		loadAttributes_mesh(nodeAtt,model);
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

bool FBXSimpleManager::loadAttributes_mesh(fbxsdk::FbxNodeAttribute * nodeAtt, FbxModel & model)
{
	fbxsdk::FbxMesh *mesh = static_cast<fbxsdk::FbxMesh*>(nodeAtt);
	if (!mesh->IsTriangleMesh()){
		fbxsdk::FbxGeometryConverter converter(m_fbxMgr);
		nodeAtt = converter.Triangulate(nodeAtt,true);
		mesh = static_cast<fbxsdk::FbxMesh*>(nodeAtt);
	}
	//all tris
	int indexCount = mesh->GetPolygonVertexCount(); //this is all the faces
	int triCount = indexCount / 3;
	model.m_tris.setCount(triCount);
	//get vertex
	int vertCount = mesh->GetControlPointsCount();
	for (int i = 0; i < vertCount; ++i) {
		fbxsdk::FbxVector4 pos = mesh->GetControlPointAt(i);
		vec3 position = vec3((float)pos.mData[0], (float)pos.mData[1], (float)pos.mData[2]);
		model.m_verts.add(position);
	}
	//set index
	int* inds = mesh->GetPolygonVertices();
	for (int i = 0; i < triCount; ++i) {
		ushort index0 = (ushort)inds[i * 3 + 0];
		ushort index1 = (ushort)inds[i * 3 + 1];
		ushort index2 = (ushort)inds[i * 3 + 2];
		model.m_tris[i].posIndex0 = index0;
		model.m_tris[i].posIndex1 = index1;
		model.m_tris[i].posIndex2 = index2;
	}
	//elements
	// -- normals
		loadElement_normal(mesh, model);
	// -- texcoord0
		loadElement_uv(mesh, model);
	//texture0
		loadElement_material(mesh, model);

#if 1
	//make the right-handeness coord to d3d
	for (int i = 0; i < vertCount; ++i) {
		model.m_verts[i].z = -model.m_verts[i].z;
	}
	for (int i = 0; i < model.m_tris.getCount(); ++i) {
		int temp = model.m_tris[i].posIndex0;
		model.m_tris[i].posIndex0 = model.m_tris[i].posIndex2;
		model.m_tris[i].posIndex2 = temp;
		temp = model.m_tris[i].tex0Index0;
		model.m_tris[i].tex0Index0 = model.m_tris[i].tex0Index2;
		model.m_tris[i].tex0Index2 = temp;
		temp = model.m_tris[i].normalIndex0;
		model.m_tris[i].normalIndex0 = model.m_tris[i].normalIndex2;
		model.m_tris[i].normalIndex2 = temp;
	}
	for (int i = 0; i < model.m_normals.getCount(); ++i) {
		model.m_normals[i].z = -model.m_normals[i].z;
	}
		
#endif
	return false;
}

bool FBXSimpleManager::loadElement_normal(fbxsdk::FbxMesh* mesh, FbxModel & model)
{
	fbxsdk::FbxGeometryElementNormal* pNormals = nullptr;
	pNormals = mesh->GetElementNormal(0);
	if (!pNormals) return false;

	fbxsdk::FbxLayerElement::EMappingMode mapMode = pNormals->GetMappingMode();
	bool useIndex = pNormals->GetReferenceMode() != FbxGeometryElement::eDirect;

	uint polyCount = mesh->GetPolygonCount();
	
	if (mapMode == fbxsdk::FbxLayerElement::eByControlPoint) {
		model.m_normals.setCount(mesh->GetControlPointsCount());
		for (uint i = 0; i < polyCount; ++i) {
			const int polySize = mesh->GetPolygonSize(i);
			for (int vi = 0; vi < polySize; ++vi){
				int ci = mesh->GetPolygonVertex(i, vi);//get the index of the current vertex in control points array
				int nIndex = useIndex ? pNormals->GetIndexArray().GetAt(ci) : ci; //the UV index depends on the reference mode
				FbxVector4 normal = pNormals->GetDirectArray().GetAt(nIndex);
				model.m_normals[ci] = vec3((float)normal.mData[0], (float)normal.mData[1], (float)normal.mData[2]);
				model.m_tris[i].normals[vi] = ci;
			}
		}
	}
	else if (mapMode == fbxsdk::FbxLayerElement::eByPolygonVertex) {
		int normalCount = pNormals->GetDirectArray().GetCount();
		model.m_normals.setCount(normalCount);
		for (int i = 0; i < normalCount; ++i) {
			FbxVector4 normal = pNormals->GetDirectArray().GetAt(i);
			model.m_normals[i] = vec3((float)normal.mData[0], (float)normal.mData[1], (float)normal.mData[2]);
		}
		for (uint i = 0; i < polyCount; i++) {
			if (useIndex) {
				model.m_tris[i].normalIndex0 = pNormals->GetIndexArray().GetAt(i * 3);
				model.m_tris[i].normalIndex1 = pNormals->GetIndexArray().GetAt(i * 3 + 1);
				model.m_tris[i].normalIndex2 = pNormals->GetIndexArray().GetAt(i * 3 + 2);
			}
			else {
				model.m_tris[i].normalIndex0 = i * 3;
				model.m_tris[i].normalIndex1 = i * 3 + 1;
				model.m_tris[i].normalIndex2 = i * 3 + 2;
			}
		}	
	}
	return true;
}

bool FBXSimpleManager::loadElement_uv(fbxsdk::FbxMesh * mesh, FbxModel & model)
{
	{//print uv
		printf("mesh : %d \n", (int)mesh);
		int uvCount = mesh->GetElementUVCount();
		printf("uvCount: %d \n",uvCount);
		for (int i = 0; i < uvCount; ++i) {
			fbxsdk::FbxGeometryElementUV* pUVs = mesh->GetElementUV(i);
			fbxsdk::FbxLayerElement::EMappingMode mapMode = pUVs->GetMappingMode();
			bool useIndex = pUVs->GetReferenceMode() != FbxGeometryElement::eDirect;
			printf("mapMode: %d : count : %d \n", (int)mapMode, useIndex ? pUVs->GetIndexArray().GetCount(): pUVs->GetDirectArray().GetCount());	
		}
	}

	int uvCount = mesh->GetElementUVCount();
	uvCount = min(2, uvCount);
	uvCount = 1;//force for test
	for (int i = 0; i < uvCount; ++i) {
		fbxsdk::FbxGeometryElementUV* pUVs = nullptr;
		pUVs = mesh->GetElementUV(i);
		if (!pUVs) continue;
		Array<vec2>* texCoords = i == 0 ? &model.m_texcoord0s : nullptr;
		if (!texCoords) continue;
		fbxsdk::FbxLayerElement::EMappingMode mapMode = pUVs->GetMappingMode();
		bool useIndex = pUVs->GetReferenceMode() != FbxGeometryElement::eDirect;
		uint polyCount = mesh->GetPolygonCount();
		if (mapMode == fbxsdk::FbxLayerElement::eByControlPoint) {
			texCoords->setCount(mesh->GetControlPointsCount());
			for (uint i = 0; i < polyCount; ++i) {
				const int polySize = mesh->GetPolygonSize(i);
				for (int vi = 0; vi < polySize; ++vi) {
					int ci = mesh->GetPolygonVertex(i, vi);	//get the index of the current vertex in control points array
					int uvIndex = useIndex ? pUVs->GetIndexArray().GetAt(ci) : ci; //the UV index depends on the reference mode
					FbxVector2 uv = pUVs->GetDirectArray().GetAt(uvIndex);
					(*texCoords)[ci] = vec2((float)uv.mData[0], (float)uv.mData[1]);
					model.m_tris[i].tex0Inds[vi] = ci;
				}
			}
		}
		else if (mapMode == fbxsdk::FbxLayerElement::eByPolygonVertex) {
			int uvCount = pUVs->GetDirectArray().GetCount();
			texCoords->setCount(uvCount);
			for (int i = 0; i < uvCount; ++i) {
				FbxVector2 uv = pUVs->GetDirectArray().GetAt(i);
				(*texCoords)[i] = (vec2((float)uv.mData[0], (float)uv.mData[1]));
			}
			for (uint i = 0; i < polyCount; ++i) {
				if (useIndex) {
					model.m_tris[i].tex0Index0 = pUVs->GetIndexArray().GetAt(i * 3);
					model.m_tris[i].tex0Index1 = pUVs->GetIndexArray().GetAt(i * 3 + 1);
					model.m_tris[i].tex0Index2 = pUVs->GetIndexArray().GetAt(i * 3 + 2);
				}
				else {
					model.m_tris[i].tex0Index0 = 3 * i;
					model.m_tris[i].tex0Index1 = 3 * i + 1;
					model.m_tris[i].tex0Index2 = 3 * i + 2;
				}
			}
		}
	}

	return true;
}

bool FBXSimpleManager::loadElement_material(fbxsdk::FbxMesh * mesh, FbxModel & model)
{
	{//print matriel
		printf("mesh : %d \n", (int)mesh);
		int matCount = mesh->GetNode()->GetSrcObjectCount<fbxsdk::FbxSurfaceMaterial>();
		printf("  mat count : %d \n", matCount);
		for (int i = 0; i < matCount; ++i) {
			FbxSurfaceMaterial* pMat = mesh->GetNode()->GetSrcObject<FbxSurfaceMaterial>(i);
			printf("    mat%d: \n", i);
			int texTypeCount = fbxsdk::FbxLayerElement::eTextureDisplacementVector - fbxsdk::FbxLayerElement::eTextureDiffuse + 1;
			for (int j = 0; j < texTypeCount; ++j) {
				fbxsdk::FbxProperty prop = pMat->FindProperty(fbxsdk::FbxLayerElement::sTextureChannelNames[j]);
				if (!prop.IsValid()) continue;
				int texCount = prop.GetSrcObjectCount<fbxsdk::FbxTexture>();
				printf("      texType%d:(%s)(%d) \n", j, fbxsdk::FbxLayerElement::sTextureChannelNames[j],texCount);
				if (texCount == 0) continue;
				for (int k = 0; k < texCount; ++k) {
					printf("        tex%d:\n", k);
					fbxsdk::FbxLayeredTexture *layerTex = prop.GetSrcObject<FbxLayeredTexture>(k);
					if (layerTex) {
						int ltexCount = layerTex->GetSrcObjectCount<fbxsdk::FbxTexture>();
						for (int s = 0; s < ltexCount; ++s) {
							FbxTexture* ltex = layerTex->GetSrcObject<fbxsdk::FbxTexture>(s);
							if (ltex) {
								FbxFileTexture *lftex = FbxCast<FbxFileTexture>(ltex);
								if (!lftex) return false;
								printf("          layerTex->tex%d : %s \n", s, (char*)lftex->GetFileName());
							}
						}
					}
					else {
						FbxTexture* ltex = prop.GetSrcObject<FbxTexture>(k);
						if (ltex) {
							FbxFileTexture *lftex = FbxCast<FbxFileTexture>(ltex);
							if (!lftex) return false;
							printf("          Tex%d : %s \n", k, (char*)lftex->GetFileName());
						}
					
					}
				
				}
			}
		}
	}
	
	//it is temp  implemented. it has many potential issues.
	int matCount = mesh->GetNode()->GetSrcObjectCount<fbxsdk::FbxSurfaceMaterial>();
	if (matCount == 0) return false;
	model.m_mats.setCount(matCount);
	for (int i = 0; i < matCount; ++i) {
		model.m_mats[i].FbxMaterial::FbxMaterial();
		fbxsdk::FbxSurfaceMaterial* pMat = mesh->GetNode()->GetSrcObject<FbxSurfaceMaterial>(i);
		if (!pMat) continue;
		fbxsdk::FbxProperty prop = pMat->FindProperty(FbxLayerElement::sTextureChannelNames[0]); // only use diffuse color tex currently.
		if (!prop.IsValid()) continue;
		int texCount = prop.GetSrcObjectCount<fbxsdk::FbxTexture>();
		if (texCount != 1) continue; //only support use one image on one texture type.
		fbxsdk::FbxLayeredTexture *layerTex = prop.GetSrcObject<FbxLayeredTexture>(0);
		char* fileName = nullptr;
		if (layerTex) {
			int ltexCount = layerTex->GetSrcObjectCount<fbxsdk::FbxTexture>();
			fbxsdk::FbxTexture* ltex = layerTex->GetSrcObject<fbxsdk::FbxTexture>(0);
			if (ltex) {
				FbxFileTexture *lftex = FbxCast<FbxFileTexture>(ltex);
				if (!lftex) continue;
				fileName = (char*)lftex->GetFileName();
			}
		}
		else {
			fbxsdk::FbxTexture* ltex = prop.GetSrcObject<FbxTexture>(0);
			if (ltex) {
				FbxFileTexture *lftex = FbxCast<FbxFileTexture>(ltex);
				if (!lftex) return false;
				fileName = (char*)lftex->GetFileName();
			}
		}
		if (fileName) {
			std::string sFileName = fileName;
			std::string::size_type pos = sFileName.rfind('/');
			if (pos == std::string::npos) pos = sFileName.rfind('\\');
			if (pos != std::string::npos) sFileName = sFileName.substr(pos + 1, sFileName.length());
			//model.m_mats[i].m_texName.setLength(strlen(sFileName.c_str()) + 1);
			model.m_mats[i].m_texName = sFileName.c_str();
		}
	}

	//get tri material index
	uint polyCount = mesh->GetPolygonCount();
	fbxsdk::FbxLayerElementMaterial* pMat = mesh->GetElementMaterial(0);
	fbxsdk::FbxLayerElement::EMappingMode mapMode = pMat->GetMappingMode();
	for (int i = 0; i < polyCount; ++i) {
		if (mapMode == fbxsdk::FbxLayerElement::eAllSame) {
			model.m_tris[i].matIndex = pMat->GetIndexArray().GetAt(0);
		}
		else if (mapMode == fbxsdk::FbxLayerElement::eByPolygon) {
			model.m_tris[i].matIndex = pMat->GetIndexArray().GetAt(i);
		}
	}
	
	return true;
}

bool FBXSimpleManager::load(const char * fileName, FbxModel & model) 
{
	fbxsdk::FbxScene* scene = fbxsdk::FbxScene::Create(m_fbxMgr, "");
	FbxImporter* sceneImporter = FbxImporter::Create(m_fbxMgr, "");
	sceneImporter->Initialize(fileName, -1, m_fbxMgr->GetIOSettings());
	sceneImporter->Import(scene);
	sceneImporter->Destroy();
	//fill the model
	//fbxsdk::FbxAxisSystem::DirectX;
	fbxsdk::FbxAxisSystem coordSystem = scene->GetGlobalSettings().GetAxisSystem();
	fbxsdk::FbxNode* rootNode = scene->GetRootNode();
	loadNode(rootNode, model);
	scene->Destroy(true);

	return true;
}

