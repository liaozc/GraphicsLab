
#include "../Framework3/Direct3D10/D3D10App.h"
#include "../Framework3/Util/Model.h"
#include "../Framework3/Util/BSP.h"

#define LAYERS 3

class App : public D3D10App {
public:
	char *getTitle() const { return "Deep deferred shading"; }

	void moveCamera(const float3 &dir);
	void resetCamera();

	void onSize(const int w, const int h);

	bool init();
	void exit();

	bool initAPI();
	void exitAPI();

	bool load();
	void unload();

	void drawFrame();

protected:
	ShaderID fillBuffers, ambient, lighting, lightSpot, shadow, preZ[2];
	TextureID base[6], bump[6];

	TextureID shadowMap, baseRT, normalRT, depthRT, lightTex;

	SamplerStateID trilinearAniso, trilinearClamp, pointClamp, shadowFilter;
	BlendStateID blendAdd;
	DepthStateID depthEqual;

	Model *map;
	BSP bsp;
};
