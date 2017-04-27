

#include "Direct3D11/D3D11App.h"
#include "Util/BSP.h"

class Model;

//#define COMPUTE_LIGHTMAP
#define L_SAMPLE_COUNT 512
#define O_SAMPLE_COUNT 4096

struct Node {
	vec3 pos;
	float wx, wy;
	float exposure;
	float blurStrength;
};

class App : public D3D11App {
public:
	char *getTitle() const { return "HDR - Blur Scene"; }

	void resetCamera();
	void moveCamera(const vec3 &dir);

	bool onKey(const uint key, const bool pressed);

	bool init();
	void exit();

	bool initAPI();
	bool load();
	void unload();

	void drawEnvironment(const mat4 &invMvp);
	void drawQuad();
	void drawFrame();

protected:
	TextureID envRGB, envExp;

	ShaderID skyBox;

	vec2 scaleBias;

	Image lightMapImage;
	TextureID lightMap;
	vec3 lightDir;

	Model *map;
	BSP bsp;

	vec2 size;

	SamplerStateID nearestClamp, trilinearAniso;

	VertexBufferID m_quad_vb;
	IndexBufferID m_quad_ib;
	VertexFormatID m_quad_vf;


	TextureID base[3], bump[3];
	ShaderID lighting;

	TextureID rtColor, rtDepth, rtBlur, rtBlur2, rtBlur3;
	ShaderID convert, blur, toneMapping;

	Slider *exposureControl, *blurControl;
	CheckBox *freeFly;
	DropDownList* HDR_mode;
	Array <Node> nodes;
};
