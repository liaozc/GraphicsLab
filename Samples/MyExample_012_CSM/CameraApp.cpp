#include "CameraApp.h"

#ifdef TEST_MODEL
#define ProjectDir	"/Samples/MyExample_000_EmptyWithCamera"
#include "InitResDir.inl"

#include "Util/Model.h"
BaseApp *app = new EmptyWithCameraApp();

#endif

EmptyWithCameraApp::EmptyWithCameraApp(float fov, float n, float f)
{
	m_fov = fov;
	m_near = n;
	m_far = f;
	m_leftPress = false;
	m_midPress = false;
	m_rightPress = false;
	m_shift = false;
}

bool EmptyWithCameraApp::init()
{
#ifdef TEST_MODEL
	AllocConsole();
	freopen("CONOUT$", "w+t", stdout);
#endif
	return true;
}

void EmptyWithCameraApp::exit()
{
#ifdef TEST_MODEL
	delete m_sphere;
#endif
}

bool EmptyWithCameraApp::load()
{

#ifdef TEST_MODEL	
	initWorkDir(renderer);
	m_cameraTest = renderer->addShader(ShaderDir("/cameraTest.shd"));
	m_sphere = new Model();
	m_sphere->createSphere(3);
	m_sphere->computeNormals(false);
	m_sphere->makeDrawable(renderer, true, m_cameraTest);
#endif

	RECT rect;
	GetWindowRect(hwnd, &rect);
	float ratio = float(rect.bottom - rect.top) / float(rect.right - rect.left);
	m_cameraProj = perspectiveMatrix(m_fov, ratio, m_near, m_far);

	return true;
}

void EmptyWithCameraApp::resetCamera()
{
	D3D11App::resetCamera();
	m_eye = vec3(0, 480, -480);
	m_lookAt = vec3(0, 0, 0);
	m_up = vec3(0, 1, 0);
	updateView();
}

bool EmptyWithCameraApp::onMouseMove(const int x, const int y, const int deltaX, const int deltaY)
{
	if (m_midPress) {
		vec2 point(x,y);
		vec2 dist = m_midPoint - point;
		dist *= vec2(0.05f, -0.05f) * (m_shift ? 10 : 1);
		vec3 lookDir = m_lookAt - m_eye;
		m_eye += dist.x * m_right;
		m_eye += dist.y * m_up;
		m_lookAt = m_eye + lookDir;
		updateView();
		m_midPoint = point;
	}
	else if (m_rightPress) {
		vec2 point(x, y);
		vec2 dist = m_rightPoint - point;
		dist *= vec2(-0.003f, 0.003f);
		if (!m_shift) {
			vec3 lookDir = m_lookAt - m_eye;
			float xAngel = dist.x;
			float cosX = cosf(xAngel);
			float sinX = sinf(xAngel);
			vec3 offsetX = m_right * sinX + lookDir * (cosX - 1.0f);
			float yAngel = dist.y;
			float cosY = cosf(yAngel);
			float sinY = sinf(yAngel);
			vec3 offsetY = m_up * sinY + lookDir * (cosY - 1.0f);
			lookDir = lookDir + offsetX + offsetY;
			m_lookAt = m_eye + lookDir;
		}
		else {
			float zAngel = dist.y;
			float cosZ = cosf(zAngel);
			float sinZ = sinf(zAngel);
			vec3 offsetZ = m_right * sinZ + m_up * (cosZ - 1.0f);
			m_up += offsetZ;
		}
		updateView();
		m_rightPoint = point;
	}
	
	return D3D11App::onMouseMove(x, y, deltaX, deltaY);
}

bool EmptyWithCameraApp::onMouseButton(const int x, const int y, const MouseButton button, const bool pressed)
{
	m_leftPress = button == MOUSE_LEFT ? pressed : m_leftPress;
	m_leftPoint = button == MOUSE_LEFT ? vec2(x, y) : m_leftPoint;

	m_midPress = button == MOUSE_MIDDLE ? pressed : m_midPress;
	m_midPoint = button == MOUSE_MIDDLE ? vec2(x, y) : m_midPoint;

	m_rightPress = button == MOUSE_RIGHT ? pressed : m_rightPress;
	m_rightPoint = button == MOUSE_RIGHT ? vec2(x, y) : m_rightPoint;

	return  true;//D3D11App::onMouseButton(x, y, button, pressed);
}

bool EmptyWithCameraApp::onMouseWheel(const int x, const int y, const int scroll)
{
	vec3 lookDir = m_lookAt - m_eye;
	m_eye += scroll * lookDir * 10 * (m_shift ? 10 : 1);
	m_lookAt = m_eye + lookDir;
	updateView();
	return D3D11App::onMouseWheel(x,y,scroll);
}

bool EmptyWithCameraApp::onKey(const uint key, const bool pressed)
{
	m_shift = key == KEY_SHIFT ? pressed : m_shift;
	vec3 lookDir = m_lookAt - m_eye;
	printf("key : %d \n",key);
	bool bUpdateView = false;
	if (key == KEY_W) {
		m_eye += lookDir * 5 * (m_shift ? 10 : 1);
		m_lookAt = m_eye + lookDir;
		bUpdateView = true;
	}
	else if (key == KEY_S) {
		m_eye -= lookDir * 5 * (m_shift ? 10 : 1);
		m_lookAt = m_eye + lookDir;
		bUpdateView = true;
	}
	else if (key == KEY_A) {
		m_eye -= m_right * 0.5 * (m_shift ? 10 : 1);
		m_lookAt = m_eye + lookDir;
		bUpdateView = true;
	}
	else if (key == KEY_D) {
		m_eye += m_right * 0.5 * (m_shift ? 10 : 1);
		m_lookAt = m_eye + lookDir;
		bUpdateView = true;
	}
	if (bUpdateView) updateView();
	return D3D11App::onKey(key,pressed);

}

void EmptyWithCameraApp::updateView()
{
	m_cameraView = makeViewMatrixD3D(m_eye, m_lookAt, m_up);
	m_right = m_cameraView.rows[0].xyz();
	m_up = m_cameraView.rows[1].xyz();
	m_lookAt = m_eye + m_cameraView.rows[2].xyz();
}

void EmptyWithCameraApp::drawFrame()
{

#ifdef TEST_MODEL
	float col[4] = { 0.5f,0.5f,0.5f,1 };
	renderer->clear(true, true, col, 1.0f);
	renderer->reset();
	renderer->setShader(m_cameraTest);
	renderer->setShaderConstant4x4f("world", identity4());
	renderer->setShaderConstant4x4f("worldViewProj", m_cameraProj * m_cameraView * scale(10,10,10));
	renderer->apply();
	m_sphere->draw(renderer);

#endif

}


