
#include "../Framework3/Direct3D10/D3D10App.h"
#include "../Framework3/Util/Model.h"

class App : public D3D10App {
public:
	char *getTitle() const { return "Order independent translucency"; }

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
	ShaderID render, sort, stencilClear;

	TextureID unsortedColorRT, unsortedDepthRT, unsortedDS;

	TextureID env;
	SamplerStateID trilinearClamp;

	RasterizerStateID noMSAA;
	DepthStateID stencilRoute, stencilSet;

	Model *model;
};
