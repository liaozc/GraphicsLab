
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
#include "../Framework3/Util/Model.h"
#include "../Framework3/Util/BSP.h"

struct Light
{
	float3 Position;
	float  Intensity;
	float3 Color;
	float  Frequency;
};

#define LIGHT_COUNT 13

class App : public D3D11App
{
public:
	char *getTitle() const { return "Modern LightMapping"; }

	bool onKey(const uint key, const bool pressed);
	bool onMouseButton(const int x, const int y, const MouseButton button, const bool pressed);
	bool onMouseWheel(const int x, const int y, const int scroll);

	void moveCamera(const float3 &dir);
	void resetCamera();

	bool init();
	void exit();

	bool initAPI();
	void exitAPI();

	bool load();
	void unload();

	void updateLights();
	void drawLights(const float4x4& view_proj);
	void drawFrame();

protected:
	int getActiveLight() const;

	ShaderID m_Lighting[3], m_PreZ, m_LightBlob;

	TextureID m_Base[3], m_Bump[3];

	Light m_Lights[LIGHT_COUNT];
	TextureID m_LightMaps;
	TextureID m_Clusters;
	float2 m_ClusterMapSize;

	uint m_ActiveLightsMask, m_AnimatedLightsMask;
	float3 m_Ambient;
	float m_Exposure;

	SamplerStateID m_TrilinearAniso, m_LinearClamp;
	BlendStateID m_AlphaBlend;
	DepthStateID m_DepthTest;

	Model m_Map;
	Model m_Sphere;
	BSP m_BSP;

	DropDownList* m_RenderMode;
};
