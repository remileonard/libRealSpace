#include "VRScreen.h"
#include "../../engine/gametimer.h"
#include "vrtimer.h"
#include <cmath>

#include <algorithm>
#include <cstdio>
#include <cmath>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl2.h"

// Windows <GL/gl.h> est souvent limité à OpenGL 1.1.
// Définir quelques constantes (valeurs standard) si elles ne sont pas exposées.
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_TEXTURE_BASE_LEVEL
#define GL_TEXTURE_BASE_LEVEL 0x813C
#endif
#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL 0x813D
#endif

#ifndef GL_SRGB8_ALPHA8
#define GL_SRGB8_ALPHA8 0x8C43
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif


namespace {
	PFNGLGENFRAMEBUFFERSEXTPROC pglGenFramebuffersEXT = nullptr;
	PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC pglCheckFramebufferStatusEXT = nullptr;
	PFNGLDELETEFRAMEBUFFERSEXTPROC pglDeleteFramebuffersEXT = nullptr;
	PFNGLBINDFRAMEBUFFEREXTPROC pglBindFramebufferEXT = nullptr;
	PFNGLFRAMEBUFFERTEXTURE2DEXTPROC pglFramebufferTexture2DEXT = nullptr;
	PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC pglFramebufferRenderbufferEXT = nullptr;
	PFNGLGENRENDERBUFFERSEXTPROC pglGenRenderbuffersEXT = nullptr;
	PFNGLDELETERENDERBUFFERSEXTPROC pglDeleteRenderbuffersEXT = nullptr;
	PFNGLBINDRENDERBUFFEREXTPROC pglBindRenderbufferEXT = nullptr;
	PFNGLRENDERBUFFERSTORAGEEXTPROC pglRenderbufferStorageEXT = nullptr;

	template <typename T>
	T loadGlProc(const char* name) {
		return reinterpret_cast<T>(SDL_GL_GetProcAddress(name));
	}

	bool loadGlFboExtProcs() {
		pglGenFramebuffersEXT = loadGlProc<PFNGLGENFRAMEBUFFERSEXTPROC>("glGenFramebuffersEXT");
		pglDeleteFramebuffersEXT = loadGlProc<PFNGLDELETEFRAMEBUFFERSEXTPROC>("glDeleteFramebuffersEXT");
		pglBindFramebufferEXT = loadGlProc<PFNGLBINDFRAMEBUFFEREXTPROC>("glBindFramebufferEXT");
		pglFramebufferTexture2DEXT = loadGlProc<PFNGLFRAMEBUFFERTEXTURE2DEXTPROC>("glFramebufferTexture2DEXT");
		pglFramebufferRenderbufferEXT = loadGlProc<PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC>("glFramebufferRenderbufferEXT");
		pglGenRenderbuffersEXT = loadGlProc<PFNGLGENRENDERBUFFERSEXTPROC>("glGenRenderbuffersEXT");
		pglDeleteRenderbuffersEXT = loadGlProc<PFNGLDELETERENDERBUFFERSEXTPROC>("glDeleteRenderbuffersEXT");
		pglBindRenderbufferEXT = loadGlProc<PFNGLBINDRENDERBUFFEREXTPROC>("glBindRenderbufferEXT");
		pglRenderbufferStorageEXT = loadGlProc<PFNGLRENDERBUFFERSTORAGEEXTPROC>("glRenderbufferStorageEXT");
		pglCheckFramebufferStatusEXT = loadGlProc<PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC>("glCheckFramebufferStatusEXT");

		return pglGenFramebuffersEXT && pglDeleteFramebuffersEXT && pglBindFramebufferEXT &&
			pglFramebufferTexture2DEXT && pglFramebufferRenderbufferEXT &&
			pglGenRenderbuffersEXT && pglDeleteRenderbuffersEXT && pglBindRenderbufferEXT &&
			pglRenderbufferStorageEXT && pglCheckFramebufferStatusEXT;
	}
}
static void drawMirrorTextureToWindow(GLuint tex, int winW, int winH)
{
    if (tex == 0 || winW <= 0 || winH <= 0) return;

    // Revenir sur le framebuffer par défaut (compat OpenGL 1.1 via EXT)
    if (pglBindFramebufferEXT) {
        pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    }

    glViewport(0, 0, winW, winH);

    // Sauvegarde état minimal pour ne pas polluer le rendu
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT | GL_VIEWPORT_BIT);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
        // Si ton mirror est inversé verticalement, swap les v (0<->1)
        glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 1.0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glPopAttrib();
}
static const char* xrResultToString(XrResult result) {
	switch (result) {
		case XR_SUCCESS: return "XR_SUCCESS";
		case XR_TIMEOUT_EXPIRED: return "XR_TIMEOUT_EXPIRED";
		case XR_SESSION_LOSS_PENDING: return "XR_SESSION_LOSS_PENDING";
		case XR_EVENT_UNAVAILABLE: return "XR_EVENT_UNAVAILABLE";
		case XR_ERROR_VALIDATION_FAILURE: return "XR_ERROR_VALIDATION_FAILURE";
		case XR_ERROR_RUNTIME_FAILURE: return "XR_ERROR_RUNTIME_FAILURE";
		case XR_ERROR_OUT_OF_MEMORY: return "XR_ERROR_OUT_OF_MEMORY";
		case XR_ERROR_API_VERSION_UNSUPPORTED: return "XR_ERROR_API_VERSION_UNSUPPORTED";
		case XR_ERROR_INITIALIZATION_FAILED: return "XR_ERROR_INITIALIZATION_FAILED";
		case XR_ERROR_FUNCTION_UNSUPPORTED: return "XR_ERROR_FUNCTION_UNSUPPORTED";
		case XR_ERROR_EXTENSION_NOT_PRESENT: return "XR_ERROR_EXTENSION_NOT_PRESENT";
		case XR_ERROR_LIMIT_REACHED: return "XR_ERROR_LIMIT_REACHED";
		case XR_ERROR_SIZE_INSUFFICIENT: return "XR_ERROR_SIZE_INSUFFICIENT";
		case XR_ERROR_HANDLE_INVALID: return "XR_ERROR_HANDLE_INVALID";
		case XR_ERROR_INSTANCE_LOST: return "XR_ERROR_INSTANCE_LOST";
		case XR_ERROR_SESSION_RUNNING: return "XR_ERROR_SESSION_RUNNING";
		case XR_ERROR_SESSION_NOT_RUNNING: return "XR_ERROR_SESSION_NOT_RUNNING";
		case XR_ERROR_SESSION_LOST: return "XR_ERROR_SESSION_LOST";
		case XR_ERROR_LAYER_INVALID: return "XR_ERROR_LAYER_INVALID";
		default: return "XR_<unknown>";
	}
}

static bool xrCheck(XrResult result, const char* what) {
	if (XR_FAILED(result)) {
		std::printf("[OpenXR] %s failed: %s (%d)\n", what, xrResultToString(result), (int)result);
		return false;
	}
	return true;
}

VRScreen::VRScreen() = default;

VRScreen::~VRScreen() {
	shutdownOpenXR();
}

void VRScreen::setTitle(const char* title) {
	// Garde la fenêtre SDL comme miroir desktop.
	RSScreen::setTitle(title);
}

void VRScreen::init(int width, int height, bool fullscreen) {
	// 1) Initialiser SDL + créer le contexte OpenGL via l'impl existante.
	RSScreen::init(width, height, false);

    auto timer = std::make_unique<VRTimer>();
    timer->setFrameDriver(this);              // <-- le timer pilotera xrWaitFrame
    GameTimer::getInstance().setTimer(std::move(timer));

	// 2) Initialiser ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Setup ImGui style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(m_window, SDL_GL_GetCurrentContext());
    ImGui_ImplOpenGL2_Init();


	// 3) Initialiser OpenXR en réutilisant le contexte courant.
	if (!initOpenXR()) {
		std::printf("[OpenXR] initOpenXR() a échoué, fallback vers RSScreen.\n");
	}
}
void VRScreen::ensureImGuiFbo(int32_t w, int32_t h) {
    if (!m_hasGlFbo) {
        return;
    }
    if (!m_imguiFbo) {
        pglGenFramebuffersEXT(1, &m_imguiFbo);
    }
    if (!m_imguiDepthRb) {
        pglGenRenderbuffersEXT(1, &m_imguiDepthRb);
        m_imguiFboW = 0;
        m_imguiFboH = 0;
    }
    if (m_imguiFboW != w || m_imguiFboH != h) {
        pglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_imguiDepthRb);
        pglRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, w, h);
        pglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
        m_imguiFboW = w;
        m_imguiFboH = h;
    }
}

void VRScreen::renderImGuiToTexture() {
    if (!m_showTimingDebug || m_imguiSwapchain.handle == XR_NULL_HANDLE) {
        return;
    }
    
    auto& sc = m_imguiSwapchain;
    
    // Acquérir une image
    uint32_t imageIndex = 0;
    XrSwapchainImageAcquireInfo acqInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, nullptr};
    if (!xrCheck(xrAcquireSwapchainImage(sc.handle, &acqInfo, &imageIndex), "xrAcquireSwapchainImage(imgui)")) {
        return;
    }
    
    XrSwapchainImageWaitInfo waitImg{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO, nullptr};
    waitImg.timeout = XR_INFINITE_DURATION;
    xrCheck(xrWaitSwapchainImage(sc.handle, &waitImg), "xrWaitSwapchainImage(imgui)");
    
    // Sauvegarder l'état OpenGL complet
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
    
    GLint oldFbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &oldFbo);
    
    GLint oldViewport[4];
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    
    GLint oldScissor[4];
    glGetIntegerv(GL_SCISSOR_BOX, oldScissor);
    
    // Sauvegarder matrices
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    
    // Bind FBO
    ensureImGuiFbo(sc.width, sc.height);
    pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_imguiFbo);
    pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
        GL_TEXTURE_2D, sc.images[imageIndex].image, 0);
    pglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, 
        GL_RENDERBUFFER_EXT, m_imguiDepthRb);
    
    // Vérifier que le FBO est complet
    GLenum status = pglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
        printf("[ImGui] FBO incomplete: 0x%x\n", status);
        goto cleanup;
    }
    
    // Configurer viewport ET scissor pour le FBO
    glViewport(0, 0, sc.width, sc.height);
    glScissor(0, 0, sc.width, sc.height);
    
    // Clear
    glClearColor(0.1f, 0.1f, 0.1f, 0.9f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Configure ImGui pour ce rendu off-screen
    {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)sc.width, (float)sc.height);
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
        
        // Rendu ImGui
        ImGui_ImplOpenGL2_NewFrame();
        ImGui::NewFrame();
        
        // Fenêtre plein écran
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2((float)sc.width, (float)sc.height), ImGuiCond_Always);
        renderTimingDebugWindow();
        
        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    }
    
cleanup:
    // Restaurer matrices
    glMatrixMode(GL_TEXTURE);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    
    // Restaurer viewport et scissor
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    glScissor(oldScissor[0], oldScissor[1], oldScissor[2], oldScissor[3]);
    
    // Restaurer FBO
    pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, oldFbo);
    
    // Restaurer attributs
    glPopClientAttrib();
    glPopAttrib();
    
    // Release swapchain
    XrSwapchainImageReleaseInfo relInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, nullptr};
    xrCheck(xrReleaseSwapchainImage(sc.handle, &relInfo), "xrReleaseSwapchainImage(imgui)");
}

void VRScreen::renderTimingDebugWindow() {
    if (!m_showTimingDebug) {
        return;
    }
    if (ImGui::Begin("VR Timing Debug", &m_showTimingDebug)) {
		ImGui::SetWindowFontScale(2.0f);
        VRTimer* vrTimer = static_cast<VRTimer*>(GameTimer::getInstance().getTimer());
        
        // Informations de timing
        ImGui::SeparatorText("Frame Timing");
        if (m_stereoFrameBegun || m_sessionRunning) {
            ImGui::Text("Predicted Display Time: %.3f ms", 
                vrTimer->getLastXrTime() / 1e6);
            ImGui::Text("Real Frame Delta: %.2f ms (%.1f FPS)", 
                vrTimer->m_realDeltaTime * 1000.0f,
                1.0f / vrTimer->m_realDeltaTime);
			ImGui::Text("Limit Frame Delta: %.2f ms (%.1f FPS)", 
                vrTimer->getDeltaTime() * 1000.0f,
                1.0f / vrTimer->getDeltaTime());
            // Historique des frame times
            m_frameTimeHistory[m_frameHistoryIndex] = vrTimer->m_realDeltaTime * 1000.0f;
            m_frameHistoryIndex = (m_frameHistoryIndex + 1) % FRAME_HISTORY_SIZE;
            
            ImGui::PlotLines("Frame Time (ms)", m_frameTimeHistory, FRAME_HISTORY_SIZE, 
                m_frameHistoryIndex, nullptr, 0.0f, 33.33f, ImVec2(0, 80));
        } else {
            ImGui::TextDisabled("Session not running");
        }
        
        // Informations de rendu stéréo
        if (m_presentMode == PresentMode::StereoProjection && !m_projectionSwapchains.empty()) {
            ImGui::SeparatorText("Stereo Rendering");
            ImGui::Text("Should Render: %s", m_stereoShouldRender ? "Yes" : "No");
            ImGui::Text("Frame Begun: %s", m_stereoFrameBegun ? "Yes" : "No");
            ImGui::Text("Eye Count: %zu", m_projectionSwapchains.size());
            
            for (size_t i = 0; i < m_projectionSwapchains.size() && i < 2; ++i) {
                ImGui::Text("Eye %zu: %dx%d", i, 
                    m_projectionSwapchains[i].width,
                    m_projectionSwapchains[i].height);
            }
            
            ImGui::Text("Mirror Valid: %s", m_mirrorValid ? "Yes" : "No");
            if (m_mirrorValid) {
                ImGui::Text("Mirror: %dx%d", m_mirrorW, m_mirrorH);
            }
        }
        
        // Swapchains QUAD
        if (!m_swapchains.empty()) {
            ImGui::SeparatorText("Quad Layer");
            ImGui::Text("Swapchain: %dx%d", m_swapchains[0].width, m_swapchains[0].height);
            ImGui::Text("Cinema Pose Valid: %s", m_cinemaPoseValid ? "Yes" : "No");
            if (m_cinemaPoseValid) {
                ImGui::Text("Position: (%.2f, %.2f, %.2f)", 
                    m_cinemaPose.position.x, 
                    m_cinemaPose.position.y, 
                    m_cinemaPose.position.z);
            }
        }
        
        // Bouton pour reset le cinema pose
        if (ImGui::Button("Reset Cinema Pose")) {
            m_cinemaPoseValid = false;
        }
    }
    ImGui::End();
}

void VRScreen::pollXrEvents() {
	if (m_xrInstance == XR_NULL_HANDLE) {
		return;
	}

	XrEventDataBuffer eventBuffer{XR_TYPE_EVENT_DATA_BUFFER, nullptr};
	while (true) {
		eventBuffer = {XR_TYPE_EVENT_DATA_BUFFER, nullptr};
		XrResult res = xrPollEvent(m_xrInstance, &eventBuffer);
		if (res == XR_EVENT_UNAVAILABLE) {
			break;
		}
		if (!xrCheck(res, "xrPollEvent")) {
			break;
		}

		// TODO: intégrer ces événements dans EventManager (plus tard).
		switch (eventBuffer.type) {
			case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
				const auto* changed = reinterpret_cast<const XrEventDataSessionStateChanged*>(&eventBuffer);
				m_sessionState = changed->state;
				break;
			}
			default:
				break;
		}
	}
}

void VRScreen::ensureSessionRunning() {
	if (m_xrSession == XR_NULL_HANDLE) {
		return;
	}

	// Drive state changes.
	pollXrEvents();

	if (!m_sessionRunning && m_sessionState == XR_SESSION_STATE_READY) {
		XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO, nullptr};
		beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
		if (xrCheck(xrBeginSession(m_xrSession, &beginInfo), "xrBeginSession")) {
			m_sessionRunning = true;
		}
	}

	if (m_sessionRunning && m_sessionState == XR_SESSION_STATE_STOPPING) {
		xrCheck(xrEndSession(m_xrSession), "xrEndSession");
		m_sessionRunning = false;
	}
}

static bool hasExtension(const std::vector<XrExtensionProperties>& exts, const char* name) {
	for (const auto& e : exts) {
		if (std::strcmp(e.extensionName, name) == 0) {
			return true;
		}
	}
	return false;
}

Matrix VRScreen::mul(const Matrix& a, const Matrix& b) {
	Matrix out;
	out.Clear();
	for (int r = 0; r < 4; ++r) {
		for (int c = 0; c < 4; ++c) {
			float acc = 0.0f;
			for (int k = 0; k < 4; ++k) {
				// stockage colonne-major: v[col][row] == m[row][col]
				acc += a.v[k][r] * b.v[c][k];
			}
			out.v[c][r] = acc;
		}
	}
	return out;
}

Matrix VRScreen::invertRigidBody(const Matrix& m) {
	// Inversion d'une matrice affine rigide (rotation + translation), format colonne-major.
	Matrix out;
	out.Identity();

	// R = m(0..2,0..2)
	// out.R = R^T
	for (int r = 0; r < 3; ++r) {
		for (int c = 0; c < 3; ++c) {
			out.v[c][r] = m.v[r][c];
		}
	}

	const float tx = m.v[3][0];
	const float ty = m.v[3][1];
	const float tz = m.v[3][2];

	// out.t = -R^T * t
	out.v[3][0] = -(out.v[0][0] * tx + out.v[1][0] * ty + out.v[2][0] * tz);
	out.v[3][1] = -(out.v[0][1] * tx + out.v[1][1] * ty + out.v[2][1] * tz);
	out.v[3][2] = -(out.v[0][2] * tx + out.v[1][2] * ty + out.v[2][2] * tz);

	return out;
}

Matrix VRScreen::makeTransformFromPose(const XrPosef& pose, float metersToWorld) {
	Matrix out;
	out.Identity();

	const float x = pose.orientation.x;
	const float y = pose.orientation.y;
	const float z = pose.orientation.z;
	const float w = pose.orientation.w;

	const float xx = x * x;
	const float yy = y * y;
	const float zz = z * z;
	const float xy = x * y;
	const float xz = x * z;
	const float yz = y * z;
	const float wx = w * x;
	const float wy = w * y;
	const float wz = w * z;

	// Matrice standard (row-major), stockée en colonne-major: out.v[col][row] = m[row][col]
	const float m00 = 1.0f - 2.0f * (yy + zz);
	const float m01 = 2.0f * (xy - wz);
	const float m02 = 2.0f * (xz + wy);

	const float m10 = 2.0f * (xy + wz);
	const float m11 = 1.0f - 2.0f * (xx + zz);
	const float m12 = 2.0f * (yz - wx);

	const float m20 = 2.0f * (xz - wy);
	const float m21 = 2.0f * (yz + wx);
	const float m22 = 1.0f - 2.0f * (xx + yy);

	out.v[0][0] = m00;
	out.v[1][0] = m01;
	out.v[2][0] = m02;

	out.v[0][1] = m10;
	out.v[1][1] = m11;
	out.v[2][1] = m12;

	out.v[0][2] = m20;
	out.v[1][2] = m21;
	out.v[2][2] = m22;

	out.v[3][0] = pose.position.x * metersToWorld;
	out.v[3][1] = pose.position.y * metersToWorld;
	out.v[3][2] = pose.position.z * metersToWorld;
	out.v[3][3] = 1.0f;

	return out;
}

Matrix VRScreen::makeProjectionFromFov(const XrFovf& fov, float zNear, float zFar) {
	Matrix out;
	out.Clear();

	const float tanLeft = std::tanf(fov.angleLeft);
	const float tanRight = std::tanf(fov.angleRight);
	const float tanDown = std::tanf(fov.angleDown);
	const float tanUp = std::tanf(fov.angleUp);

	const float w = tanRight - tanLeft;
	const float h = tanUp - tanDown;

	// OpenGL clip space Z in [-1,1]
	const float m00 = 2.0f / w;
	const float m11 = 2.0f / h;
	const float m20 = (tanRight + tanLeft) / w;
	const float m21 = (tanUp + tanDown) / h;
	const float m22 = (zFar + zNear) / (zNear - zFar);
	const float m32 = (2.0f * zFar * zNear) / (zNear - zFar);

	out.v[0][0] = m00;
	out.v[1][1] = m11;
	out.v[2][0] = m20;
	out.v[2][1] = m21;
	out.v[2][2] = m22;
	out.v[2][3] = -1.0f;
	out.v[3][2] = m32;

	return out;
}

bool VRScreen::initOpenXR() {
	if (m_xrInstance != XR_NULL_HANDLE) {
		return true;
	}

	// Extensions
	uint32_t extCount = 0;
	if (!xrCheck(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extCount, nullptr), "xrEnumerateInstanceExtensionProperties(count)")) {
		return false;
	}
	std::vector<XrExtensionProperties> exts(extCount, {XR_TYPE_EXTENSION_PROPERTIES, nullptr});
	if (!xrCheck(xrEnumerateInstanceExtensionProperties(nullptr, extCount, &extCount, exts.data()), "xrEnumerateInstanceExtensionProperties(list)")) {
		return false;
	}

	std::vector<const char*> enabledExts;
	if (!hasExtension(exts, XR_KHR_OPENGL_ENABLE_EXTENSION_NAME)) {
		std::printf("[OpenXR] Extension requise absente: %s\n", XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);
		return false;
	}
	enabledExts.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);

	XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO, nullptr};
	std::snprintf(createInfo.applicationInfo.applicationName, XR_MAX_APPLICATION_NAME_SIZE, "NeoSC");
	createInfo.applicationInfo.applicationVersion = 1;
	std::snprintf(createInfo.applicationInfo.engineName, XR_MAX_ENGINE_NAME_SIZE, "libRealSpace");
	createInfo.applicationInfo.engineVersion = 1;
	createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	createInfo.enabledExtensionCount = (uint32_t)enabledExts.size();
	createInfo.enabledExtensionNames = enabledExts.data();

	if (!xrCheck(xrCreateInstance(&createInfo, &m_xrInstance), "xrCreateInstance")) {
		m_xrInstance = XR_NULL_HANDLE;
		return false;
	}

	// Proc addr
	xrCheck(xrGetInstanceProcAddr(m_xrInstance, "xrGetOpenGLGraphicsRequirementsKHR",
								 reinterpret_cast<PFN_xrVoidFunction*>(&m_xrGetOpenGLGraphicsRequirementsKHR)),
			"xrGetInstanceProcAddr(xrGetOpenGLGraphicsRequirementsKHR)");
	if (!m_xrGetOpenGLGraphicsRequirementsKHR) {
		std::printf("[OpenXR] xrGetOpenGLGraphicsRequirementsKHR indisponible.\n");
		return false;
	}

	// System
	XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO, nullptr};
	systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	if (!xrCheck(xrGetSystem(m_xrInstance, &systemInfo, &m_xrSystemId), "xrGetSystem")) {
		return false;
	}

	// Graphics requirements
	XrGraphicsRequirementsOpenGLKHR glReq{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR, nullptr};
	if (!xrCheck(m_xrGetOpenGLGraphicsRequirementsKHR(m_xrInstance, m_xrSystemId, &glReq), "xrGetOpenGLGraphicsRequirementsKHR")) {
		return false;
	}

	// Create session (Win32 OpenGL binding)
	// SDL a déjà rendu le contexte courant via RSScreen::init.
	HGLRC hglrc = wglGetCurrentContext();
	HDC hdc = wglGetCurrentDC();
	if (!hglrc || !hdc) {
		std::printf("[OpenXR] Contexte OpenGL Win32 introuvable (hglrc/hdc nuls).\n");
		return false;
	}

	// OpenGL 2.1 + Windows: charger les fonctions FBO via extensions.
	m_hasGlFbo = loadGlFboExtProcs();
	if (!m_hasGlFbo) {
		std::printf("[OpenXR] FBO OpenGL (EXT) indisponible. Mode stéréo désactivé.\n");
	}

	XrGraphicsBindingOpenGLWin32KHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR, nullptr};
	graphicsBinding.hDC = hdc;
	graphicsBinding.hGLRC = hglrc;

	XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO, nullptr};
	sessionInfo.next = &graphicsBinding;
	sessionInfo.systemId = m_xrSystemId;
	if (!xrCheck(xrCreateSession(m_xrInstance, &sessionInfo, &m_xrSession), "xrCreateSession")) {
		m_xrSession = XR_NULL_HANDLE;
		return false;
	}

	// Reference space
	XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO, nullptr};
	spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
	spaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
	if (!xrCheck(xrCreateReferenceSpace(m_xrSession, &spaceInfo, &m_xrAppSpace), "xrCreateReferenceSpace")) {
		m_xrAppSpace = XR_NULL_HANDLE;
		return false;
	}

	// VIEW space (head-locked) pour un "écran" VR toujours devant l'utilisateur.
	{
		XrReferenceSpaceCreateInfo viewSpaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO, nullptr};
		viewSpaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
		viewSpaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
		if (!xrCheck(xrCreateReferenceSpace(m_xrSession, &viewSpaceInfo, &m_xrViewSpace), "xrCreateReferenceSpace(VIEW)")) {
			m_xrViewSpace = XR_NULL_HANDLE;
		}
	}

	// View config / swapchains
	uint32_t viewCount = 0;
	if (!xrCheck(xrEnumerateViewConfigurationViews(m_xrInstance, m_xrSystemId,
												   XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
												   0, &viewCount, nullptr),
				 "xrEnumerateViewConfigurationViews(count)")) {
		return false;
	}
	m_viewConfigs.assign(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW, nullptr});
	if (!xrCheck(xrEnumerateViewConfigurationViews(m_xrInstance, m_xrSystemId,
												   XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
												   viewCount, &viewCount, m_viewConfigs.data()),
				 "xrEnumerateViewConfigurationViews(list)")) {
		return false;
	}

	m_views.assign(viewCount, {XR_TYPE_VIEW, nullptr});

	// Environment blend mode: choisir une valeur supportée par le runtime.
	{
		uint32_t blendCount = 0;
		if (xrCheck(xrEnumerateEnvironmentBlendModes(m_xrInstance, m_xrSystemId,
												XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
												0, &blendCount, nullptr),
					"xrEnumerateEnvironmentBlendModes(count)")) {
			std::vector<XrEnvironmentBlendMode> blends(blendCount);
			if (xrCheck(xrEnumerateEnvironmentBlendModes(m_xrInstance, m_xrSystemId,
												XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
												blendCount, &blendCount, blends.data()),
					"xrEnumerateEnvironmentBlendModes(list)")) {
				m_environmentBlendMode = blends.empty() ? XR_ENVIRONMENT_BLEND_MODE_OPAQUE : blends[0];
				for (auto b : blends) {
					if (b == XR_ENVIRONMENT_BLEND_MODE_OPAQUE) {
						m_environmentBlendMode = b;
						break;
					}
				}
			}
		}
	}

	// Swapchain formats
	uint32_t formatCount = 0;
	if (!xrCheck(xrEnumerateSwapchainFormats(m_xrSession, 0, &formatCount, nullptr), "xrEnumerateSwapchainFormats(count)")) {
		return false;
	}
	std::vector<int64_t> formats(formatCount);
	if (!xrCheck(xrEnumerateSwapchainFormats(m_xrSession, formatCount, &formatCount, formats.data()), "xrEnumerateSwapchainFormats(list)")) {
		return false;
	}

	// Choix du format couleur: tester sRGB d'abord
    const int64_t preferred[] = {
        (int64_t)GL_SRGB8_ALPHA8,
        (int64_t)GL_RGBA8,
    };

    m_colorSwapchainFormat = 0;
    for (int64_t f : preferred) {
        if (std::find(formats.begin(), formats.end(), f) != formats.end()) {
            m_colorSwapchainFormat = f;
            break;
        }
    }

    if (m_colorSwapchainFormat == 0) {
        // fallback: prendre le premier format fourni
        m_colorSwapchainFormat = formats.empty() ? (int64_t)GL_RGBA8 : formats[0];
    }

	m_swapchains.clear();
	{
		// Layer QUAD => un seul swapchain suffit.
		// On prend la résolution recommandée max parmi les vues (souvent identiques).
		int32_t recW = 0;
		int32_t recH = 0;
		uint32_t recSamples = 1;
		for (const auto& vp : m_viewConfigs) {
			recW = (std::max)(recW, (int32_t)vp.recommendedImageRectWidth);
			recH = (std::max)(recH, (int32_t)vp.recommendedImageRectHeight);
			recSamples = (std::max)(recSamples, vp.recommendedSwapchainSampleCount);
		}
		if (recW <= 0) recW = 1024;
		if (recH <= 0) recH = 1024;

		Swapchain sc;
		sc.width = recW;
		sc.height = recH;

		XrSwapchainCreateInfo scInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO, nullptr};
		scInfo.arraySize = 1;
		scInfo.mipCount = 1;
		scInfo.faceCount = 1;
		scInfo.format = m_colorSwapchainFormat;
		scInfo.width = sc.width;
		scInfo.height = sc.height;
		scInfo.sampleCount = recSamples;
		// QUAD layer => le runtime sample la texture: XR_SWAPCHAIN_USAGE_SAMPLED_BIT requis.
		scInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

		if (!xrCheck(xrCreateSwapchain(m_xrSession, &scInfo, &sc.handle), "xrCreateSwapchain")) {
			return false;
		}

		uint32_t imageCount = 0;
		if (!xrCheck(xrEnumerateSwapchainImages(sc.handle, 0, &imageCount, nullptr), "xrEnumerateSwapchainImages(count)")) {
			return false;
		}
		sc.images.assign(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR, nullptr});
		if (!xrCheck(xrEnumerateSwapchainImages(sc.handle, imageCount, &imageCount,
										reinterpret_cast<XrSwapchainImageBaseHeader*>(sc.images.data())),
					 "xrEnumerateSwapchainImages(list)")) {
			return false;
		}

		// Important: ces textures sont échantillonnées par le runtime (QUAD layer).
		// Assurer un état GL "complet" (pas de mipmaps requises) pour éviter un rendu noir.
		for (const auto& img : sc.images) {
			if (img.image == 0) {
				continue;
			}
			glBindTexture(GL_TEXTURE_2D, img.image);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		m_swapchains.push_back(std::move(sc));
	}

	// Swapchains stéréo (projection): un swapchain par oeil (arraySize=1).
	m_projectionSwapchains.clear();
	{
		for (uint32_t eye = 0; eye < (uint32_t)m_viewConfigs.size(); ++eye) {
			Swapchain sc;
			sc.width = (int32_t)m_viewConfigs[eye].recommendedImageRectWidth;
			sc.height = (int32_t)m_viewConfigs[eye].recommendedImageRectHeight;
			if (sc.width <= 0) sc.width = 1024;
			if (sc.height <= 0) sc.height = 1024;

			XrSwapchainCreateInfo scInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO, nullptr};
			scInfo.arraySize = 1;
			scInfo.mipCount = 1;
			scInfo.faceCount = 1;
			scInfo.format = m_colorSwapchainFormat;
			scInfo.width = sc.width;
			scInfo.height = sc.height;
			scInfo.sampleCount = (uint32_t)(std::max)(1u, m_viewConfigs[eye].recommendedSwapchainSampleCount);
			scInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

			if (!xrCheck(xrCreateSwapchain(m_xrSession, &scInfo, &sc.handle), "xrCreateSwapchain(stereo)")) {
				return false;
			}

			uint32_t imageCount = 0;
			if (!xrCheck(xrEnumerateSwapchainImages(sc.handle, 0, &imageCount, nullptr), "xrEnumerateSwapchainImages(stereo,count)")) {
				return false;
			}
			sc.images.assign(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR, nullptr});
			if (!xrCheck(xrEnumerateSwapchainImages(sc.handle, imageCount, &imageCount,
									reinterpret_cast<XrSwapchainImageBaseHeader*>(sc.images.data())),
					 "xrEnumerateSwapchainImages(stereo,list)")) {
				return false;
			}

			m_projectionSwapchains.push_back(std::move(sc));
		}
	}

	{
        Swapchain sc;
        sc.width = 800;
        sc.height = 600;
        
        XrSwapchainCreateInfo scInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO, nullptr};
        scInfo.arraySize = 1;
        scInfo.mipCount = 1;
        scInfo.faceCount = 1;
        scInfo.format = m_colorSwapchainFormat;
        scInfo.width = sc.width;
        scInfo.height = sc.height;
        scInfo.sampleCount = 1;
        scInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        
        if (xrCheck(xrCreateSwapchain(m_xrSession, &scInfo, &sc.handle), "xrCreateSwapchain(imgui)")) {
            uint32_t imageCount = 0;
            if (xrCheck(xrEnumerateSwapchainImages(sc.handle, 0, &imageCount, nullptr), "xrEnumerateSwapchainImages(imgui,count)")) {
                sc.images.assign(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR, nullptr});
                if (xrCheck(xrEnumerateSwapchainImages(sc.handle, imageCount, &imageCount,
                                        reinterpret_cast<XrSwapchainImageBaseHeader*>(sc.images.data())),
                         "xrEnumerateSwapchainImages(imgui,list)")) {
                    
                    // Configuration des textures
                    for (const auto& img : sc.images) {
                        if (img.image == 0) continue;
                        glBindTexture(GL_TEXTURE_2D, img.image);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
                        glBindTexture(GL_TEXTURE_2D, 0);
                    }
                    
                    m_imguiSwapchain = std::move(sc);
                }
            }
        }
    }
	m_stereoAcquiredImageIndex.assign(m_viewConfigs.size(), 0);
	m_stereoImageAcquired.assign(m_viewConfigs.size(), false);
	m_projectionViews.assign(m_viewConfigs.size(), {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW, nullptr});

	// La boucle principale du jeu appelle refresh() : on pilote le start session ici.
	pollXrEvents();
	return true;
}

void VRScreen::shutdownOpenXR() {
	// Nettoyage ImGui FBO
    if (m_hasGlFbo && m_imguiFbo) {
        pglDeleteFramebuffersEXT(1, &m_imguiFbo);
        m_imguiFbo = 0;
    }
    if (m_hasGlFbo && m_imguiDepthRb) {
        pglDeleteRenderbuffersEXT(1, &m_imguiDepthRb);
        m_imguiDepthRb = 0;
    }
    
    // Détruire swapchain ImGui
    if (m_imguiSwapchain.handle != XR_NULL_HANDLE) {
        xrDestroySwapchain(m_imguiSwapchain.handle);
        m_imguiSwapchain.handle = XR_NULL_HANDLE;
    }
	if (m_hasGlFbo && m_stereoFbo) {
		pglDeleteFramebuffersEXT(1, &m_stereoFbo);
		m_stereoFbo = 0;
	}
	if (m_hasGlFbo && m_stereoDepthRb) {
		pglDeleteRenderbuffersEXT(1, &m_stereoDepthRb);
		m_stereoDepthRb = 0;
		m_stereoDepthW = 0;
		m_stereoDepthH = 0;
	}

	if (!m_projectionSwapchains.empty()) {
		for (auto& sc : m_projectionSwapchains) {
			if (sc.handle != XR_NULL_HANDLE) {
				xrDestroySwapchain(sc.handle);
				sc.handle = XR_NULL_HANDLE;
			}
		}
		m_projectionSwapchains.clear();
	}

	if (!m_swapchains.empty()) {
		for (auto& sc : m_swapchains) {
			if (sc.handle != XR_NULL_HANDLE) {
				xrDestroySwapchain(sc.handle);
				sc.handle = XR_NULL_HANDLE;
			}
		}
		m_swapchains.clear();
	}

	if (m_xrViewSpace != XR_NULL_HANDLE) {
		xrDestroySpace(m_xrViewSpace);
		m_xrViewSpace = XR_NULL_HANDLE;
	}
	if (m_xrAppSpace != XR_NULL_HANDLE) {
		xrDestroySpace(m_xrAppSpace);
		m_xrAppSpace = XR_NULL_HANDLE;
	}
	if (m_xrSession != XR_NULL_HANDLE) {
		xrDestroySession(m_xrSession);
		m_xrSession = XR_NULL_HANDLE;
	}
	if (m_xrInstance != XR_NULL_HANDLE) {
		xrDestroyInstance(m_xrInstance);
		m_xrInstance = XR_NULL_HANDLE;
	}

	m_sessionRunning = false;
	m_sessionState = XR_SESSION_STATE_UNKNOWN;
	m_stereoFrameBegun = false;
	m_stereoShouldRender = false;
	m_projectionLayerReady = false;
}

void VRScreen::ensureStereoFbo(int32_t w, int32_t h) {
	if (!m_hasGlFbo) {
		return;
	}
	if (!m_stereoFbo) {
		pglGenFramebuffersEXT(1, &m_stereoFbo);
	}
	if (!m_stereoDepthRb) {
		pglGenRenderbuffersEXT(1, &m_stereoDepthRb);
		m_stereoDepthW = 0;
		m_stereoDepthH = 0;
	}
	if (m_stereoDepthW != w || m_stereoDepthH != h) {
		pglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_stereoDepthRb);
		pglRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, w, h);
		pglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
		m_stereoDepthW = w;
		m_stereoDepthH = h;
	}
}

bool VRScreen::vrPrepareStereoFrame(bool& outShouldRender) {
    if (!m_hasGlFbo) {
        outShouldRender = false;
        return false;
    }
    m_presentMode = PresentMode::StereoProjection;
    const bool ok = prepareStereoFrame();
    outShouldRender = ok && stereoShouldRender();
    return ok;
}

bool VRScreen::vrBeginStereoEye(uint32_t eye,
						const Matrix& worldFromLocal,
						float metersToWorld,
						float zNear,
						float zFar,
						VRStereoEyeRenderInfo& out) {
	Matrix proj;
	Matrix view;
	
	const bool ok = beginStereoEye(eye, worldFromLocal, metersToWorld, zNear, zFar, proj, view);
	if (!ok) {
		return false;
	}
	out.width = stereoEyeWidth(eye);
	out.height = stereoEyeHeight(eye);
	out.projection = proj;
	out.view = view;
	return true;
}

void VRScreen::vrEndStereoFrame() {
	endStereoFrame();
}

int32_t VRScreen::stereoEyeWidth(uint32_t eye) const {
	if (eye >= m_projectionSwapchains.size()) return 0;
	return m_projectionSwapchains[eye].width;
}

int32_t VRScreen::stereoEyeHeight(uint32_t eye) const {
	if (eye >= m_projectionSwapchains.size()) return 0;
	return m_projectionSwapchains[eye].height;
}

bool VRScreen::prepareStereoFrame() {
    // La frame a déjà été démarrée par VRTimer::update() → xrBeginTickFrame().
    // Plus AUCUN xrWaitFrame/xrBeginFrame ici.
    if (!m_frameStarted) {
        return false;
    }
    // Synchronise les drapeaux stéréo sur l'état de la frame courante.
    m_stereoFrameState   = m_currentFrameState;
    m_stereoFrameBegun   = true;             // signale à refresh() qu'on est en 3D
    m_stereoShouldRender = m_frameShouldRender;
    m_projectionLayerReady = false;
    std::fill(m_stereoImageAcquired.begin(), m_stereoImageAcquired.end(), false);
    return true;
}

bool VRScreen::beginStereoEye(uint32_t eye,
                        const Matrix& worldFromLocal,
                        float metersToWorld,
                        float zNear,
                        float zFar,
                        Matrix& outProjection,
                        Matrix& outView) {
    if (!m_hasGlFbo) {
		return false;
	} 
    if (!m_frameStarted || !m_stereoShouldRender) {
		return false;
	}
    if (eye >= m_projectionSwapchains.size() || eye >= m_views.size()) {
		return false;
	}

    auto& sc = m_projectionSwapchains[eye];
    uint32_t imageIndex = 0;
    XrSwapchainImageAcquireInfo acqInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, nullptr};
    if (!xrCheck(xrAcquireSwapchainImage(sc.handle, &acqInfo, &imageIndex), "xrAcquireSwapchainImage(stereo)")) {
        return false;
    }

    XrSwapchainImageWaitInfo waitImg{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO, nullptr};
    waitImg.timeout = XR_INFINITE_DURATION;
    xrCheck(xrWaitSwapchainImage(sc.handle, &waitImg), "xrWaitSwapchainImage(stereo)");

    m_stereoAcquiredImageIndex[eye] = imageIndex;
    m_stereoImageAcquired[eye] = true;

    // Matrices
    outProjection = makeProjectionFromFov(m_views[eye].fov, zNear, zFar);
    const Matrix localFromEye = makeTransformFromPose(m_views[eye].pose, metersToWorld);
    const Matrix worldFromEye = mul(worldFromLocal, localFromEye);
    outView = invertRigidBody(worldFromEye);

    // Bind FBO
    ensureStereoFbo(sc.width, sc.height);
    pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_stereoFbo);
    pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, sc.images[imageIndex].image, 0);
    pglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_stereoDepthRb);

    if (eye == 0) {
        m_mirrorTex = sc.images[imageIndex].image;
        m_mirrorW = sc.width;
        m_mirrorH = sc.height;
        m_mirrorValid = (m_mirrorTex != 0);
    }

    // Nettoyage minimal (le renderer fera son clear si nécessaire)
    glViewport(0, 0, sc.width, sc.height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    return true;
}



void VRScreen::endStereoFrame() {
    if (!m_stereoFrameBegun) {
        return;
    }

    // PAS DE RENDU IMGUI ICI EN MODE STEREO
    // ImGui sera rendu comme un QUAD layer séparé

    // Release des swapchains
    if (m_stereoShouldRender) {
        for (uint32_t eye = 0; eye < (uint32_t)m_projectionSwapchains.size(); ++eye) {
            if (!m_stereoImageAcquired[eye]) continue;
            auto& sc = m_projectionSwapchains[eye];
            XrSwapchainImageReleaseInfo relInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, nullptr};
            xrCheck(xrReleaseSwapchainImage(sc.handle, &relInfo), "xrReleaseSwapchainImage(stereo)");
            m_stereoImageAcquired[eye] = false;
        }
    }

    // Unbind FBO
    if (m_hasGlFbo) {
        pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    }

    // Préparer layer projection pour refresh().
    if (m_stereoShouldRender && m_projectionSwapchains.size() == m_views.size()) {
        m_projectionLayer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION, nullptr};
        m_projectionLayer.space = m_xrAppSpace;
        m_projectionLayer.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
        m_projectionLayer.next = nullptr;
        m_projectionLayer.layerFlags = 0;

        for (uint32_t eye = 0; eye < (uint32_t)m_views.size(); ++eye) {
            XrCompositionLayerProjectionView pv{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW, nullptr};
            pv.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
            pv.next = nullptr;
            pv.pose = m_views[eye].pose;
            pv.fov = m_views[eye].fov;
            pv.subImage.swapchain = m_projectionSwapchains[eye].handle;
            pv.subImage.imageArrayIndex = 0;
            pv.subImage.imageRect.offset = {0, 0};
            pv.subImage.imageRect.extent = {m_projectionSwapchains[eye].width, m_projectionSwapchains[eye].height};
            m_projectionViews[eye] = pv;
        }
        m_projectionLayer.viewCount = (uint32_t)m_projectionViews.size();
        m_projectionLayer.views = m_projectionViews.data();
        m_projectionLayerReady = true;
    }
}

void VRScreen::refresh(void) {
    // OpenXR indisponible -> desktop classique.
    if (m_xrInstance == XR_NULL_HANDLE || m_xrSession == XR_NULL_HANDLE) {
        RSScreen::refresh();
        return;
    }

    // Aucune frame XR démarrée ce tick (session non active, ou wait échoué) :
    // on se contente du mirror desktop.
    if (!m_frameStarted) {
        RSScreen::refresh();
        return;
    }

    // Le runtime ne veut rien afficher : fermer avec 0 layer (le moins coûteux).
    if (!m_frameShouldRender) {
        endFrameNoLayers();
        RSScreen::refresh();
        return;
    }

    // Aiguillage présentation.
    // Mode 3D RÉEL = mode projection ET l'activité a effectivement préparé les yeux.
    const bool stereoActive =
        (m_presentMode == PresentMode::StereoProjection) && m_stereoFrameBegun;

    if (stereoActive) {
        presentStereoFrame();   // simulation : layer projection (+ quad ImGui)
    } else {
        presentCinemaFrame();   // tout le reste : quad "écran cinéma"
    }

    RSScreen::refresh();        // mirror desktop + swap
}

void VRScreen::presentStereoFrame() {
    const XrFrameState& st = m_currentFrameState;

    updateImguiPanelPose(st.predictedDisplayTime);

    const bool wantImgui =
        m_showTimingDebug && m_imguiPoseValid && m_imguiSwapchain.handle != XR_NULL_HANDLE;
    if (wantImgui && ++m_imguiFrameCounter >= m_imguiFrameDivider) {
        m_imguiFrameCounter = 0;
        renderImGuiToTexture();   // sans glFinish()
    }

    // Tableau fixe (pas d'allocation par frame).
    const XrCompositionLayerBaseHeader* layers[2];
    uint32_t layerCount = 0;

    if (m_stereoShouldRender && m_projectionLayerReady) {
        layers[layerCount++] =
            reinterpret_cast<const XrCompositionLayerBaseHeader*>(&m_projectionLayer);
    }

    XrCompositionLayerQuad imguiQuad{XR_TYPE_COMPOSITION_LAYER_QUAD, nullptr};
    if (wantImgui) {
        buildImguiQuadLayer(imguiQuad);
        layers[layerCount++] =
            reinterpret_cast<const XrCompositionLayerBaseHeader*>(&imguiQuad);
    }

    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO, nullptr};
    endInfo.displayTime = st.predictedDisplayTime;
    endInfo.environmentBlendMode = m_environmentBlendMode;
    endInfo.layerCount = layerCount;
    endInfo.layers = layerCount ? layers : nullptr;
    xrCheck(xrEndFrame(m_xrSession, &endInfo), "xrEndFrame(stereo)");

    // Reset de l'état de frame.
    m_frameStarted = false;
    m_stereoFrameBegun = false;
    m_projectionLayerReady = false;
    if (auto* t = static_cast<VRTimer*>(GameTimer::getInstance().getTimer())) {
        t->markFramePresented();
    }

    if (m_mirrorValid && m_window) {
        int winW = 0, winH = 0;
        SDL_GL_GetDrawableSize(m_window, &winW, &winH);
        drawMirrorTextureToWindow(m_mirrorTex, winW, winH);
    }
}

void VRScreen::presentCinemaFrame() {
    const XrFrameState& st = m_currentFrameState;

    // Pose cinéma : calculée une seule fois (identique à l'existant).
    if (!m_cinemaPoseValid && m_xrViewSpace != XR_NULL_HANDLE && m_xrAppSpace != XR_NULL_HANDLE) {
        XrSpaceLocation viewInApp{XR_TYPE_SPACE_LOCATION, nullptr};
        if (xrCheck(xrLocateSpace(m_xrViewSpace, m_xrAppSpace, st.predictedDisplayTime, &viewInApp),
                    "xrLocateSpace(VIEW->LOCAL,cinema)")) {
            const XrSpaceLocationFlags need =
                XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
            if ((viewInApp.locationFlags & need) == need) {
                // ... calcul forward projeté XZ + m_cinemaPose (inchangé) ...
                m_cinemaPoseValid = true;
            }
        }
    }

    if (m_swapchains.empty()) {
        endFrameNoLayers();
        return;
    }

    int drawableW = 0, drawableH = 0;
    if (m_window) SDL_GL_GetDrawableSize(m_window, &drawableW, &drawableH);

    auto& sc = m_swapchains[0];
    uint32_t imageIndex = 0;
    XrSwapchainImageAcquireInfo acqInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, nullptr};
    if (!xrCheck(xrAcquireSwapchainImage(sc.handle, &acqInfo, &imageIndex), "xrAcquireSwapchainImage(cinema)")) {
        endFrameNoLayers();
        return;
    }
    XrSwapchainImageWaitInfo waitImg{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO, nullptr};
    waitImg.timeout = XR_INFINITE_DURATION;
    xrCheck(xrWaitSwapchainImage(sc.handle, &waitImg), "xrWaitSwapchainImage(cinema)");

    // ImGui directement dans le backbuffer (throttlé) avant la copie.
    if (m_showTimingDebug && ++m_imguiFrameCounter >= m_imguiFrameDivider) {
        m_imguiFrameCounter = 0;
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        renderTimingDebugWindow();
        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    }

    // Copie centrée (sans glFlush : xrReleaseSwapchainImage pose la barrière).
    const GLuint targetTex = sc.images[imageIndex].image;
    const int copyW = std::max(0, std::min(drawableW, sc.width));
    const int copyH = std::max(0, std::min(drawableH, sc.height));
    const int srcX = std::max(0, (drawableW - copyW) / 2);
    const int srcY = std::max(0, (drawableH - copyH) / 2);
    const int dstX = std::max(0, (sc.width  - copyW) / 2);
    const int dstY = std::max(0, (sc.height - copyH) / 2);
    if (targetTex != 0 && copyW > 0 && copyH > 0) {
        glBindTexture(GL_TEXTURE_2D, targetTex);
        glReadBuffer(GL_BACK);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, dstX, dstY, srcX, srcY, copyW, copyH);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    XrSwapchainImageReleaseInfo relInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, nullptr};
    xrCheck(xrReleaseSwapchainImage(sc.handle, &relInfo), "xrReleaseSwapchainImage(cinema)");

    XrCompositionLayerQuad quad{XR_TYPE_COMPOSITION_LAYER_QUAD, nullptr};
    quad.space = m_xrAppSpace;
    quad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
    quad.pose = m_cinemaPose;
    const float aspect = (sc.height > 0) ? (sc.width / (float)sc.height) : 1.3333f;
    const float quadHeight = 4.0f;
    quad.size = {quadHeight * aspect, quadHeight};
    quad.subImage.swapchain = sc.handle;
    quad.subImage.imageRect.offset = {0, 0};
    quad.subImage.imageRect.extent = {sc.width, sc.height};
    quad.subImage.imageArrayIndex = 0;

    const XrCompositionLayerBaseHeader* layers[1] = {
        reinterpret_cast<const XrCompositionLayerBaseHeader*>(&quad)
    };

    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO, nullptr};
    endInfo.displayTime = st.predictedDisplayTime;
    endInfo.environmentBlendMode = m_environmentBlendMode;
    endInfo.layerCount = 1;
    endInfo.layers = layers;
    xrCheck(xrEndFrame(m_xrSession, &endInfo), "xrEndFrame(cinema)");

    m_frameStarted = false;
    if (auto* t = static_cast<VRTimer*>(GameTimer::getInstance().getTimer())) {
        t->markFramePresented();
    }
}
void VRScreen::endFrameNoLayers() {
    if (!m_frameStarted) return;
    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO, nullptr};
    endInfo.displayTime = m_currentFrameState.predictedDisplayTime;
    endInfo.environmentBlendMode = m_environmentBlendMode;
    endInfo.layerCount = 0;
    endInfo.layers = nullptr;
    xrCheck(xrEndFrame(m_xrSession, &endInfo), "xrEndFrame(empty)");
    m_frameStarted = false;
    m_stereoFrameBegun = false;
    m_projectionLayerReady = false;
    if (auto* t = static_cast<VRTimer*>(GameTimer::getInstance().getTimer())) {
        t->markFramePresented();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// xrBeginTickFrame : UNIQUE xrWaitFrame/xrBeginFrame du tick.
// Appelé par VRTimer::update() AVANT runFrame(), pour que la physique
// dispose du predictedDisplayTime du compositeur.
// ─────────────────────────────────────────────────────────────────────────────
bool VRScreen::xrBeginTickFrame(XrTime& predictedDisplayTime, bool& shouldRender) {
    if (m_xrInstance == XR_NULL_HANDLE || m_xrSession == XR_NULL_HANDLE) {
        return false;
    }

    pollXrEvents();
    ensureSessionRunning();
    if (!m_sessionRunning) {
        return false;
    }

    // Sécurité : si une frame précédente n'a jamais été présentée (cas anormal),
    // on la ferme sans layer pour rééquilibrer le runtime.
    if (m_frameStarted) {
        endFrameNoLayers();
    }

    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO, nullptr};
    m_currentFrameState = {XR_TYPE_FRAME_STATE, nullptr};
    if (!xrCheck(xrWaitFrame(m_xrSession, &waitInfo, &m_currentFrameState), "xrWaitFrame")) {
        return false;
    }

    XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO, nullptr};
    if (!xrCheck(xrBeginFrame(m_xrSession, &beginInfo), "xrBeginFrame")) {
        return false;
    }

    m_frameStarted = true;
    m_frameShouldRender = m_currentFrameState.shouldRender;

    // Si le mode stéréo est actif, on pré-localise les vues ici pour que
    // beginStereoEye() (appelé dans runFrame) n'ait plus qu'à acquérir l'image.
    if (m_presentMode == PresentMode::StereoProjection && m_frameShouldRender) {
        XrViewLocateInfo locateInfo{XR_TYPE_VIEW_LOCATE_INFO, nullptr};
        locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        locateInfo.displayTime = m_currentFrameState.predictedDisplayTime;
        locateInfo.space = m_xrAppSpace;

        XrViewState viewState{XR_TYPE_VIEW_STATE, nullptr};
        uint32_t viewCountOutput = 0;
        xrCheck(xrLocateViews(m_xrSession, &locateInfo, &viewState,
                (uint32_t)m_views.size(), &viewCountOutput, m_views.data()), "xrLocateViews");
    }

    predictedDisplayTime = m_currentFrameState.predictedDisplayTime;
    shouldRender = m_frameShouldRender;
    return true;
}
void VRScreen::updateImguiPanelPose(XrTime predictedDisplayTime) {
    if (m_imguiPoseValid) {
        return;
    }
    if (m_xrViewSpace == XR_NULL_HANDLE || m_xrAppSpace == XR_NULL_HANDLE) {
        return;
    }

    XrSpaceLocation viewInApp{XR_TYPE_SPACE_LOCATION, nullptr};
    if (!xrCheck(xrLocateSpace(m_xrViewSpace, m_xrAppSpace, predictedDisplayTime, &viewInApp),
                 "xrLocateSpace(VIEW->LOCAL,imgui)")) {
        return;
    }

    const XrSpaceLocationFlags need =
        XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
    if ((viewInApp.locationFlags & need) != need) {
        return;
    }

    const XrVector3f p = viewInApp.pose.position;
    const XrQuaternionf q = viewInApp.pose.orientation;

    // Forward headset: rotation de (0,0,-1) par q (formule vectorielle quaternion)
    const float x = q.x, y = q.y, z = q.z, w = q.w;
    const float vx = 0.0f, vy = 0.0f, vz = -1.0f;

    const float cx1 = y * vz - z * vy;
    const float cy1 = z * vx - x * vz;
    const float cz1 = x * vy - y * vx;
    const float cx2 = y * cz1 - z * cy1;
    const float cy2 = z * cx1 - x * cz1;
    const float cz2 = x * cy1 - y * cx1;

    float fx = vx + 2.0f * (w * cx1 + cx2);
    float fz = vz + 2.0f * (w * cz1 + cz2);

    // Projection XZ (panel “stable” horizontalement)
    float len = std::sqrt(fx * fx + fz * fz);
    if (len < 1e-4f) {
        fx = 0.0f;
        fz = -1.0f;
        len = 1.0f;
    }
    fx /= len;
    fz /= len;

    // Position du panneau
    const float distance = 0.8f;
    const float rightOffset = 0.0f;
    const float downOffset = -1.7f;

    const float rx = fz;
    const float rz = -fx;

    m_imguiPanelPose.position = {
        p.x + fx * distance + rx * rightOffset,
        p.y + downOffset,
        p.z + fz * distance + rz * rightOffset
    };

    // Orientation: yaw vers l'utilisateur + tilt vers le bas
    const float tiltAngle = -45.0f * 3.14159265359f / 180.0f;

    const float dirX = -fx;
    const float dirZ = -fz;
    const float yaw = std::atan2(dirX, dirZ);

    // qYaw (axe Y)
    const float halfYaw = 0.5f * yaw;
    const float sy = std::sin(halfYaw);
    const float cy = std::cos(halfYaw);

    // qTilt (axe X)
    const float halfTilt = 0.5f * tiltAngle;
    const float sx = std::sin(halfTilt);
    const float cx = std::cos(halfTilt);

    // q = qYaw * qTilt
    // (x,y,z,w) Hamilton
    XrQuaternionf out{};
    out.x = cy * sx;
    out.y = sy * cx;
    out.z = -sy * sx;
    out.w = cy * cx;

    // Normalisation
    float qlen = std::sqrt(out.x * out.x + out.y * out.y + out.z * out.z + out.w * out.w);
    if (qlen > 1e-6f) {
        out.x /= qlen;
        out.y /= qlen;
        out.z /= qlen;
        out.w /= qlen;
    } else {
        out = {0.0f, 0.0f, 0.0f, 1.0f};
    }

    m_imguiPanelPose.orientation = out;
    m_imguiPoseValid = true;
}

void VRScreen::buildImguiQuadLayer(XrCompositionLayerQuad& outQuad) const {
    outQuad = {XR_TYPE_COMPOSITION_LAYER_QUAD, nullptr};
    outQuad.space = m_xrAppSpace;
    outQuad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
    outQuad.pose = m_imguiPanelPose;

    const float aspect =
        (m_imguiSwapchain.height > 0) ? (m_imguiSwapchain.width / (float)m_imguiSwapchain.height) : 1.3333f;
    const float panelHeight = 0.5f;
    const float panelWidth = panelHeight * aspect;
    outQuad.size = {panelWidth, panelHeight};

    outQuad.subImage.swapchain = m_imguiSwapchain.handle;
    outQuad.subImage.imageArrayIndex = 0;
    outQuad.subImage.imageRect.offset = {0, 0};
    outQuad.subImage.imageRect.extent = {m_imguiSwapchain.width, m_imguiSwapchain.height};
}