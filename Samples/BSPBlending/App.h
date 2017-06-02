
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

#include "../Framework3/Direct3D11/D3D11App.h"

// Mirrored in Traverse.shd
struct BSPNode
{
	// Plane equation for this node
	float3 Normal;
	float D;

	// Instead of storing back and front node pointers, we only need to store the total number of nodes in the back sub-tree.
	// The location of back and front sub-trees are implicit. Back-tree (if there is one) is always at the next node, so
	// simply +1 gets you there. To get to front tree you add the provided number of nodes in the back tree.
	uint BackNodeCount;
};

class App : public D3D11App
{
public:
	char *getTitle() const { return "BSP Blending"; }

	void moveCamera(const float3 &dir);
	void resetCamera();

	bool init();
	void exit();

	bool initAPI();
	void exitAPI();

	bool load();
	void unload();

	void drawFrame();

protected:
	ShaderID m_Lighting, m_Skybox, m_Traverse;

	TextureID m_Base, m_Bump, m_Environment;

	SamplerStateID m_TrilinearAniso, m_TrilinearClamp;
	BlendStateID m_AlphaBlend;

	// Source mesh
	VertexFormatID m_VertexFormat;
	VertexBufferID m_VertexBuffer;

	// The index buffer that the compute shader will sort in visibility order
	IndexBufferID m_IndexBuffer;
	ID3D11Buffer* m_D3DIndexBuffer;
	ID3D11UnorderedAccessView* m_D3DIndexBufferUAV;

	// The BSP tree for the compute shader to traverse
	ID3D11Buffer* m_BSPBuffer;
	ID3D11ShaderResourceView* m_BSPBufferSRV;

	static const uint MAX_NODE_COUNT = 512;
	BSPNode m_BSP[MAX_NODE_COUNT];
	uint m_BSPNodeCount;

	// Resources for tracking when we need to update visibility order
	struct Plane
	{
		float3 Normal;
		float D;
	};
	static const uint MAX_PLANE_COUNT = 128;
	Plane m_Planes[MAX_PLANE_COUNT];
	uint m_PlaneCount;
	bool m_FrontOfPlane[MAX_PLANE_COUNT];

	float3 m_ReferencePos;
	float m_ClosestPlaneDist;
	bool m_VisibilityOrderChanged;

	// UI
	CheckBox* m_Update;
	Slider* m_Opacity;
};
