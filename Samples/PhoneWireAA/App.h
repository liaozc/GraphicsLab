
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

#include "../Framework3/Direct3D10/D3D10App.h"
#include "../Framework3/Util/Model.h"

class App : public D3D10App
{
public:
	char *getTitle() const { return "Phone-wire AA"; }

	bool GetTerrainHeight(float &height, const float3 &position) const;

	void resetCamera();
	void moveCamera(const vec3 &dir);

	bool init();
	void exit();

	bool initAPI();
	void exitAPI();

	bool load();
	void unload();

	void drawFrame();

protected:
	RasterizerStateID m_NoMSAA;
	SamplerStateID m_TrilinearClamp, m_TrilinearAniso;

	ShaderID m_SkyboxShader;
	TextureID m_Environment;

	ShaderID m_TerrainShader;
	TextureID m_Terrain, m_TerrainShadow, m_Ground0, m_Ground1;
	Image m_TerrainImage;

	ShaderID m_PoleShader;
	TextureID m_Wood;
	Model *m_Pole;

	ShaderID m_WireShader;
	VertexBufferID m_WireVB;
	VertexBufferID m_WireIB;
	VertexFormatID m_WireVF;
	TextureID m_WireTex;
	BlendStateID m_WireBlend;

	CheckBox *m_UseWireAA;
	Slider *m_WireRadius;
};
