#include "OpenGLRenderer.h"
#include "../scene/SceneState.h"
#include "../ShaderUniforms.h"
#include "DebugOverlayRenderer.h"

#include <Theme.h>

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace papagedon {

// ──────────────────────────────────────────────────────────────────────────────
// Preset-colour helpers
//
// The active ExperiencePreset supplies every colour the shader uses.  These free
// functions adapt the experience layer's PresetColor into the renderer's Color3
// and ease one colour toward another so preset switches cross-fade on screen.
// No colours are hardcoded here — they all originate from the preset table.
// ──────────────────────────────────────────────────────────────────────────────
namespace {

Color3 ToColor3(const visual::ThemeColor& c) noexcept {
    return Color3{c.r, c.g, c.b};
}

// Fullscreen black quad used to fade the frame in / out during a shader
// transition — works over any shader, needs no render targets.
constexpr const char* kFadeFragment = R"GLSL(
#version 460 core
in  vec2 vUV;
out vec4 fragColor;
uniform float uAlpha;
void main() { fragColor = vec4(0.0, 0.0, 0.0, uAlpha); }
)GLSL";

// ──────────────────────────────────────────────────────────────────────────────
// Cinematic startup logo — procedural gold "waveform → P" mark on deep green.
// A luxury brand reveal: the audio waveform draws in from the left, flows into
// the P (which draws bottom-to-top), a gold gleam sweeps across, and it glows,
// then fades into the show.  uTime is seconds since the intro began.
// ──────────────────────────────────────────────────────────────────────────────
constexpr const char* kIntroFragment = R"GLSL(
#version 460 core
in  vec2 vUV;
out vec4 fragColor;
uniform float uTime;
uniform vec2  uResolution;

float sdSeg(vec2 p, vec2 a, vec2 b){
    vec2 pa = p - a, ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}
float hash(vec2 p){ p = fract(p * vec2(123.34, 345.45)); p += dot(p, p + 34.345); return fract(p.x * p.y); }

void main(){
    vec2 R = uResolution;
    vec2 uv = vUV * 2.0 - 1.0;
    uv.x *= R.x / R.y;
    float T = uTime;

    // ── deep forest-green backdrop, soft centre light + vignette + grain ──
    float vig = smoothstep(1.7, 0.15, length(uv));
    vec3 col  = mix(vec3(0.018, 0.050, 0.032), vec3(0.055, 0.115, 0.082), vig);
    col += (hash(vUV * R) - 0.5) * 0.015;

    // logo space (slight settle from large → resting)
    vec2 p = uv * 1.35;
    p /= mix(1.07, 1.0, smoothstep(0.0, 1.3, T));

    float sx = -0.16;                 // stem x
    float th = 0.028;                 // stroke half-thickness
    float aa = 2.4 * 1.35 / R.y;      // ~2px anti-alias in this space

    // ── P mark: vertical stem + right-half bowl ──
    float dStem = sdSeg(p, vec2(sx, -0.62), vec2(sx, 0.60));
    vec2  c  = vec2(sx, 0.24);
    float Rb = 0.36;
    float dBowl = (p.x < sx) ? 1e3 : abs(length(p - c) - Rb);
    float dP = min(dStem, dBowl);

    // ── waveform: audio burst entering from the left into the stem ──
    float xw = p.x;
    float wob = 0.30 * sin(xw * 10.0) * exp(-pow((xw + 0.55) / 0.28, 2.0))
              + 0.44 * sin(xw * 16.0) * exp(-pow((xw + 0.30) / 0.20, 2.0))
              + 0.22 * sin(xw * 22.0) * exp(-pow((xw + 0.05) / 0.16, 2.0));
    float baseline = -0.03 + wob;
    float inSpan = step(-1.30, p.x) * step(p.x, sx + 0.01);
    float dWave = mix(1e3, abs(p.y - baseline), inSpan);

    // ── time-driven draw-on reveals ──
    float waveProg  = smoothstep(0.35, 1.9, T);
    float waveFront = mix(-1.34, sx + 0.02, waveProg);
    float waveMask  = 1.0 - smoothstep(waveFront - 0.05, waveFront, p.x);

    float pProg   = smoothstep(1.5, 3.0, T);
    float pFrontY = mix(-0.72, 0.66, pProg);
    float pMask   = 1.0 - smoothstep(pFrontY - 0.06, pFrontY, p.y);

    float waveLine = (1.0 - smoothstep(th - aa, th + aa, dWave)) * waveMask;
    float pLine    = (1.0 - smoothstep(th - aa, th + aa, dP))    * pMask;
    float mark     = max(waveLine, pLine);
    float dMark    = min(dWave, dP);

    // ── gold shading (vertical gradient) ──
    float grad = clamp(p.y * 0.55 + 0.5, 0.0, 1.0);
    vec3 gold  = mix(vec3(0.45, 0.30, 0.07), vec3(1.0, 0.87, 0.53), grad);

    // gleam sweep across the mark
    float sweepEnv = smoothstep(2.2, 3.4, T) - smoothstep(3.4, 4.3, T);
    float proj     = dot(p, normalize(vec2(0.9, 0.5)));
    float sweepPos = mix(-0.9, 0.95, smoothstep(2.2, 3.7, T));
    float gleam    = exp(-pow((proj - sweepPos) / 0.10, 2.0)) * sweepEnv;
    gold = mix(gold, vec3(1.0, 0.97, 0.86), gleam * 0.9);

    // ── glow halo + bright leading "pen" tips ──
    float glow = exp(-dMark * 10.0) * max(waveMask, pMask);
    float waveTip = exp(-pow((p.x - waveFront) / 0.02, 2.0))
                  * exp(-pow((p.y - baseline) / 0.05, 2.0)) * (1.0 - step(1.85, T));
    float pTip = exp(-pow((p.y - pFrontY) / 0.02, 2.0))
               * (1.0 - smoothstep(th * 3.0, th * 6.0, dP))
               * step(1.5, T) * (1.0 - step(3.0, T));

    // ── compose ──
    col += vec3(0.9, 0.65, 0.25) * glow * 0.45;
    col  = mix(col, gold, clamp(mark, 0.0, 1.0));
    col += vec3(1.0, 0.95, 0.8) * (waveTip + pTip) * 0.85;
    col += gold * gleam * 0.5;

    // ── global fades ──
    float inFade  = smoothstep(0.0, 0.5, T);
    float outFade = 1.0 - smoothstep(4.35, 4.9, T);
    col *= inFade * outFade;
    col *= mix(0.62, 1.0, vig);

    fragColor = vec4(col, 1.0);
}
)GLSL";

// Compiles a vertex + fragment pair into a linked program (0 on failure).
[[nodiscard]] unsigned int CompileGLProgram(const char* vs, const char* fs) noexcept {
    const auto stage = [](GLenum type, const char* src) -> unsigned int {
        const unsigned int sh = glCreateShader(type);
        glShaderSource(sh, 1, &src, nullptr);
        glCompileShader(sh);
        int ok = 0;
        glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (ok == 0) {
            char log[512];
            glGetShaderInfoLog(sh, 512, nullptr, log);
            std::fprintf(stderr, "[fade] shader error: %s\n", log);
            glDeleteShader(sh);
            return 0u;
        }
        return sh;
    };
    const unsigned int v = stage(GL_VERTEX_SHADER, vs);
    if (v == 0u) return 0u;
    const unsigned int f = stage(GL_FRAGMENT_SHADER, fs);
    if (f == 0u) { glDeleteShader(v); return 0u; }
    const unsigned int p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v);
    glDeleteShader(f);
    int ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (ok == 0) { glDeleteProgram(p); return 0u; }
    return p;
}

Color3 LerpColor(const Color3& a, const Color3& b, const float t) noexcept {
    return Color3{
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
    };
}

// One-pole follower with asymmetric attack/release, frame-rate independent.
// A fast attack keeps visual onsets in sync with the audio (low latency); a
// slower release smooths the decay so nothing strobes between frames.
float Follow(const float current, const float target, const float dt,
             const float attackRate, const float releaseRate) noexcept {
    const float rate  = target > current ? attackRate : releaseRate;
    const float alpha = 1.0f - std::exp(-dt * rate);
    return current + (target - current) * alpha;
}

} // namespace

// ──────────────────────────────────────────────────────────────────────────────
// Implementation (pimpl)
// ──────────────────────────────────────────────────────────────────────────────
class OpenGLRenderer::Implementation final {
public:
    GLFWwindow* window              = nullptr;
    double      lastFpsUpdateTime   = 0.0;
    unsigned int renderedFrameCount = 0;
    double      currentFps          = 0.0;
    double      lastFrameTime       = 0.0;
    bool        showDebugOverlay    = false;
    bool        debugKeyWasPressed  = false;
    DebugOverlayRenderer debugOverlay;
    ShaderUniforms smoothedUniforms;

    // ── Shader library ──────────────────────────────────────────────────────────
    // Slot 0 is the built-in reactive shader (its own 12 forms via Auto-VJ); the
    // rest are premium pack shaders (e.g. Badman Experience) compiled from .frag
    // files at startup. '[' / ']' cycle the active shader live for a VJ set.
    std::vector<std::unique_ptr<ShaderManager>> shaderLib;   // extra shaders (slot 1..N)
    std::vector<std::string>                    shaderNames; // names for every slot (0..N)
    int  activeShader        = 0;
    bool prevShaderKeyPressed = false;
    bool nextShaderKeyPressed = false;
    // Brief on-screen name toast after a switch.
    std::string toastText;
    double      toastUntil = 0.0;

    // ── Shader transition (fade through black) ──────────────────────────────────
    unsigned int fadeProgram  = 0;
    int          fadeAlphaLoc = -1;
    bool         transitioning = false;
    int          pendingShader = -1;
    float        fade      = 1.0f;   // 1 = fully visible, 0 = black
    bool         fadingOut = false;

    // ── Cinematic startup intro (procedural gold logo) ──────────────────────────
    unsigned int introProgram = 0;
    int          introTimeLoc = -1;
    int          introResLoc  = -1;
    bool         introActive  = true;   // plays once at startup
    double       introStart   = -1.0;

    // ── Live controls menu (H, and auto-shown at startup) ───────────────────────
    bool   showMenu          = false;
    bool   menuKeyWasPressed = false;   // 'H'
    double menuVisibleUntil  = 0.0;     // auto-hide time when shown non-sticky

    // ── Auto-shader director (music-driven transitions) ─────────────────────────
    bool         autoShader     = false;
    double       lastSwitchTime = 0.0;
    bool         prevBeat       = false;
    float        energyBaseline = 0.0f;
    bool         autoShaderKeyWasPressed = false; // 'V'

    // ── Pattern cross-fade state ───────────────────────────────────────────────
    // Tracks the signature form the shader is drawing.  When the preset's pattern
    // changes we snapshot the outgoing form and ease patternBlend 0 → 1, so the
    // visual form morphs across the switch rather than popping.
    int   activePatternMode   = 0;
    int   previousPatternMode = 0;
    float patternBlend        = 1.0f;
    bool  patternInitialized  = false;

    // ── Live controls (Task 7) ──────────────────────────────────────────────────
    // F1..F7 select a theme slot; Space toggles play/pause; R reloads the theme
    // JSON; A toggles Auto-VJ.  Rising edges are latched in EndFrame (where events
    // are polled) and drained by the Runtime via the Consume* methods.  F12
    // (overlay) and ESC (exit) are handled inside EndFrame directly.
    static constexpr int kThemeKeyCount = 7;
    bool themeKeyWasPressed[kThemeKeyCount] = {};
    int  pendingThemeRequest = -1;

    // 'B' snaps straight back to the show's home theme (Badman red), which loads
    // past the F1..F7 slots and so is otherwise unreachable from the keyboard.
    bool homeKeyWasPressed = false;
    bool pendingHomeRequest = false;

    bool spaceWasPressed  = false;
    bool pendingPlayPause = false;

    bool reloadKeyWasPressed = false;
    bool pendingReload       = false;

    bool autoKeyWasPressed = false;
    bool pendingAutoToggle = false;

    // ── Demo Mode state ─────────────────────────────────────────────────────────
    float masterBrightness = 1.0f;
    float masterGlow       = 1.0f;
    float masterExposure   = 1.0f;
    bool  fxEnabled        = true;   // F10 toggles visual FX (glow/bloom/noise)
    bool  demoMode         = false;  // F9 toggles presentation mode

    // Fullscreen toggle bookkeeping — windowed geometry to restore.
    bool isFullscreen = false;
    int  windowedX = 100, windowedY = 100, windowedW = 1280, windowedH = 720;

    // Cursor auto-hide (demo/fullscreen): hide after a few idle seconds.
    double lastActivityTime = 0.0;
    double lastCursorX = 0.0, lastCursorY = 0.0;
    bool   cursorHidden = false;

    // Extra control edges.
    bool demoKeyWasPressed       = false; // F9
    bool fxKeyWasPressed         = false; // F10
    bool fullscreenKeyWasPressed = false; // F11

    // Soundcheck input-level meter (M).
    bool showMeter        = false;
    bool meterKeyWasPressed = false;
};

// ──────────────────────────────────────────────────────────────────────────────
// Lifetime
// ──────────────────────────────────────────────────────────────────────────────
OpenGLRenderer::OpenGLRenderer(const bool vsyncEnabled)
    : implementation_{std::make_unique<Implementation>()},
      vsyncEnabled_{vsyncEnabled} {}

OpenGLRenderer::~OpenGLRenderer() {
    Shutdown();
}

// ──────────────────────────────────────────────────────────────────────────────
// Initialize
// ──────────────────────────────────────────────────────────────────────────────
bool OpenGLRenderer::Initialize() {
    if (initialized_) {
        return true;
    }

    // ── Window & context ────────────────────────────────────────────────────
    if (glfwInit() != GLFW_TRUE) {
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    // Fullscreen when requested (falls back to a window on any failure), so a
    // bad display setup never stops the app from starting.
    GLFWmonitor* windowMonitor = nullptr;
    int createW = 1280;
    int createH = 720;
    if (fullscreenRequested_) {
        if (GLFWmonitor* const monitor = glfwGetPrimaryMonitor(); monitor != nullptr) {
            if (const GLFWvidmode* const mode = glfwGetVideoMode(monitor); mode != nullptr) {
                createW = mode->width;
                createH = mode->height;
                windowMonitor = monitor;
            }
        }
    }

    implementation_->window = glfwCreateWindow(
        createW, createH, "PAPAGEDON Core v0.0.1", windowMonitor, nullptr);

    if (implementation_->window == nullptr && windowMonitor != nullptr) {
        // Fullscreen creation failed — retry windowed.
        implementation_->window =
            glfwCreateWindow(1280, 720, "PAPAGEDON Core v0.0.1", nullptr, nullptr);
        windowMonitor = nullptr;
    }
    if (implementation_->window == nullptr) {
        glfwTerminate();
        return false;
    }
    implementation_->isFullscreen = (windowMonitor != nullptr);

    glfwMakeContextCurrent(implementation_->window);
    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
        glfwDestroyWindow(implementation_->window);
        implementation_->window = nullptr;
        glfwTerminate();
        return false;
    }

    // Allow disabling vsync (PAPAGEDON_VSYNC=0) to uncap the frame rate on
    // high-refresh displays — useful for hitting the 120 FPS desktop target.
    if (const char* const vsyncEnv = std::getenv("PAPAGEDON_VSYNC")) {
        vsyncEnabled_ = vsyncEnv[0] != '0';
    }
    glfwSwapInterval(vsyncEnabled_ ? 1 : 0);

    // ── Fullscreen VAO ──────────────────────────────────────────────────────
    // No vertex data is needed — the vertex shader generates positions from
    // gl_VertexID.  An empty VAO is still required by the OpenGL core profile.
    glGenVertexArrays(1, &fullscreenVAO_);

    // ── Shader library ────────────────────────────────────────────────────────
    // Slot 0: the built-in reactive shader (12 Auto-VJ forms).
    if (!shaderManager_.Compile(
            ShaderManager::DefaultVertexSource(),
            ShaderManager::DefaultFragmentSource())) {
        glDeleteVertexArrays(1, &fullscreenVAO_);
        fullscreenVAO_ = 0u;
        glfwDestroyWindow(implementation_->window);
        implementation_->window = nullptr;
        glfwTerminate();
        return false;
    }
    implementation_->shaderNames.push_back("Signature (Auto-VJ forms)");

    // Slots 1..N: premium pack shaders compiled from .frag files (Badman
    // Experience by default). A file that fails to compile is skipped so a bad
    // shader never stops the show. Override the folder with PAPAGEDON_SHADER_DIR.
    {
        namespace fs = std::filesystem;
        const char* const dirEnv = std::getenv("PAPAGEDON_SHADER_DIR");
        // Scan the whole shader tree recursively so every pack (Signature,
        // Badman, and any new packs) loads into one library.
        const std::string dir = dirEnv != nullptr
            ? std::string(dirEnv)
            : std::string("engine/rendering/shaders");
        std::error_code ec;
        if (fs::is_directory(dir, ec)) {
            std::vector<std::string> files;
            for (const auto& entry : fs::recursive_directory_iterator(dir, ec)) {
                if (!ec && entry.is_regular_file() && entry.path().extension() == ".frag") {
                    files.push_back(entry.path().string());
                }
            }
            std::sort(files.begin(), files.end());
            for (const std::string& file : files) {
                std::ifstream in(file, std::ios::binary);
                if (!in) continue;
                std::ostringstream ss; ss << in.rdbuf();
                auto sm = std::make_unique<ShaderManager>();
                if (sm->Compile(ShaderManager::DefaultVertexSource(), ss.str().c_str())) {
                    std::string name = fs::path(file).stem().string();
                    if (name.size() > 3 && name[2] == '_' &&
                        std::isdigit(static_cast<unsigned char>(name[0]))) {
                        name = name.substr(3);                 // strip "NN_"
                    }
                    for (char& ch : name) if (ch == '_') ch = ' ';
                    if (!name.empty()) name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
                    implementation_->shaderLib.push_back(std::move(sm));
                    implementation_->shaderNames.push_back(name);
                } else {
                    std::fprintf(stderr, "[ShaderLib] skipped (compile failed): %s\n", file.c_str());
                }
            }
        }
    }

    // Fade-transition program (black overlay quad) for smooth shader switches.
    implementation_->fadeProgram = CompileGLProgram(ShaderManager::DefaultVertexSource(), kFadeFragment);
    implementation_->fadeAlphaLoc = implementation_->fadeProgram != 0
        ? glGetUniformLocation(implementation_->fadeProgram, "uAlpha") : -1;
    implementation_->lastSwitchTime = glfwGetTime();

    // Cinematic startup logo program. PAPAGEDON_NO_INTRO=1 skips it (fast relaunch).
    implementation_->introProgram = CompileGLProgram(ShaderManager::DefaultVertexSource(), kIntroFragment);
    implementation_->introTimeLoc = implementation_->introProgram != 0
        ? glGetUniformLocation(implementation_->introProgram, "uTime") : -1;
    implementation_->introResLoc = implementation_->introProgram != 0
        ? glGetUniformLocation(implementation_->introProgram, "uResolution") : -1;
    if (const char* const noIntro = std::getenv("PAPAGEDON_NO_INTRO")) {
        if (noIntro[0] != '0') {
            implementation_->introActive = false;
        }
    }
    if (implementation_->introProgram == 0) {
        implementation_->introActive = false; // no program → straight to the show
    }

    // Auto-shader director: transition through the library with the music.
    if (const char* const autoEnv = std::getenv("PAPAGEDON_AUTO_SHADER")) {
        implementation_->autoShader = autoEnv[0] != '0';
    }

    // Launch straight into the soundcheck level meter when requested.
    if (const char* const meterEnv = std::getenv("PAPAGEDON_METER")) {
        implementation_->showMeter = meterEnv[0] != '0';
    }

    // Optional starting shader (index into the library: 0 = signature, 1..N = pack).
    if (const char* const startShader = std::getenv("PAPAGEDON_START_SHADER")) {
        const int idx = std::atoi(startShader);
        const int count = 1 + static_cast<int>(implementation_->shaderLib.size());
        if (idx >= 0 && idx < count) {
            implementation_->activeShader = idx;
        }
    }

    // ── Debug overlay ────────────────────────────────────────────────────────
    implementation_->lastFpsUpdateTime  = glfwGetTime();
    implementation_->lastFrameTime      = glfwGetTime();
    implementation_->renderedFrameCount = 0;
    implementation_->debugOverlay.Initialize();

    // Cursor-activity baseline for demo-mode auto-hide.
    implementation_->lastActivityTime = glfwGetTime();
    glfwGetCursorPos(implementation_->window,
                     &implementation_->lastCursorX, &implementation_->lastCursorY);

    initialized_ = true;
    return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// Demo-mode configuration
// ──────────────────────────────────────────────────────────────────────────────
void OpenGLRenderer::Configure(const bool fullscreen, const bool vsync) {
    fullscreenRequested_ = fullscreen;
    vsyncEnabled_        = vsync;
}

void OpenGLRenderer::SetMasterControls(const float brightness, const float glow,
                                       const float exposure) {
    implementation_->masterBrightness = brightness;
    implementation_->masterGlow       = glow;
    implementation_->masterExposure   = exposure;
}

void OpenGLRenderer::SetDemoMode(const bool enabled) {
    implementation_->demoMode = enabled;
    if (implementation_->window == nullptr) {
        return;
    }
    // Demo mode goes fullscreen and suppresses the overlay; leaving it restores
    // the cursor and a window.
    if (enabled) {
        if (!implementation_->isFullscreen) {
            SetFullscreen(true);
        }
        implementation_->showDebugOverlay = false;
    } else {
        glfwSetInputMode(implementation_->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        implementation_->cursorHidden = false;
    }
    implementation_->lastActivityTime = glfwGetTime();
}

void OpenGLRenderer::SetDebugOverlay(const bool visible) {
    implementation_->showDebugOverlay = visible;
}

const char* OpenGLRenderer::BackendName() const noexcept {
    return "OpenGL 4.6 Core";
}

// ──────────────────────────────────────────────────────────────────────────────
// PresentSplash — one branded loading frame
// ──────────────────────────────────────────────────────────────────────────────
void OpenGLRenderer::PresentSplash(const std::string& status, const float progress) {
    if (!initialized_ || implementation_->window == nullptr) {
        return;
    }
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(implementation_->window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.018F, 0.050F, 0.032F, 1.0F); // deep forest green (brand)
    glClear(GL_COLOR_BUFFER_BIT);

    implementation_->debugOverlay.RenderSplash(
        "PAPAGEDON Core  v0.0.1", status.c_str(), progress, width, height);

    glfwSwapBuffers(implementation_->window);
    glfwPollEvents();
}

// ──────────────────────────────────────────────────────────────────────────────
// SetFullscreen
// ──────────────────────────────────────────────────────────────────────────────
void OpenGLRenderer::SetFullscreen(const bool enable) {
    if (implementation_->window == nullptr || enable == implementation_->isFullscreen) {
        return;
    }
    if (enable) {
        glfwGetWindowPos(implementation_->window,
                         &implementation_->windowedX, &implementation_->windowedY);
        glfwGetWindowSize(implementation_->window,
                          &implementation_->windowedW, &implementation_->windowedH);
        GLFWmonitor* const monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* const mode = monitor != nullptr ? glfwGetVideoMode(monitor) : nullptr;
        if (mode == nullptr) {
            return; // no display info — stay windowed rather than fail
        }
        glfwSetWindowMonitor(implementation_->window, monitor, 0, 0,
                             mode->width, mode->height, mode->refreshRate);
        implementation_->isFullscreen = true;
    } else {
        glfwSetWindowMonitor(implementation_->window, nullptr,
                             implementation_->windowedX, implementation_->windowedY,
                             implementation_->windowedW, implementation_->windowedH, 0);
        implementation_->isFullscreen = false;
    }
    // Changing the monitor can reset the swap interval — re-apply it.
    glfwSwapInterval(vsyncEnabled_ ? 1 : 0);
}

// ──────────────────────────────────────────────────────────────────────────────
// BeginShaderTransition — fade the current shader out to black, swap, fade in
// ──────────────────────────────────────────────────────────────────────────────
void OpenGLRenderer::BeginShaderTransition(const int target) {
    const int count = 1 + static_cast<int>(implementation_->shaderLib.size());
    if (target < 0 || target >= count || implementation_->transitioning) {
        return;
    }
    if (target == implementation_->activeShader) {
        return;
    }
    implementation_->pendingShader = target;
    implementation_->transitioning = true;
    implementation_->fadingOut     = true;
    implementation_->toastText  = implementation_->shaderNames[static_cast<std::size_t>(target)];
    implementation_->toastUntil = glfwGetTime() + 2.2;
}

// ──────────────────────────────────────────────────────────────────────────────
// BeginFrame
// ──────────────────────────────────────────────────────────────────────────────
void OpenGLRenderer::BeginFrame() {
    // Reserved for future pre-frame GPU work (e.g. fence waits, UBO updates).
}

// ──────────────────────────────────────────────────────────────────────────────
// Render
// ──────────────────────────────────────────────────────────────────────────────
void OpenGLRenderer::Render(
    const SceneState&     state,
    const DebugState&     debugState,
    const audio::ExperienceSignals& signals,
    const ExperiencePreset& preset,
    const visual::Theme&  theme) {

    if (!initialized_) {
        return;
    }

    // ── Cinematic startup intro ─────────────────────────────────────────────────
    // Plays once before the show: the animated gold logo + wordmark on deep green.
    // Draws to the back buffer; EndFrame swaps and still polls input (so ESC quits
    // and ENTER can skip).  Returns early so the show doesn't render underneath.
    if (implementation_->introActive) {
        const double now = glfwGetTime();
        if (implementation_->introStart < 0.0) {
            implementation_->introStart = now;
        }
        const float introT = static_cast<float>(now - implementation_->introStart);
        constexpr float kIntroDuration = 4.9f;

        int iw = 0;
        int ih = 0;
        glfwGetFramebufferSize(implementation_->window, &iw, &ih);
        glViewport(0, 0, iw, ih);
        glClearColor(0.018F, 0.050F, 0.032F, 1.0F); // deep forest green
        glClear(GL_COLOR_BUFFER_BIT);

        if (implementation_->introProgram != 0) {
            glUseProgram(implementation_->introProgram);
            if (implementation_->introTimeLoc >= 0) {
                glUniform1f(implementation_->introTimeLoc, introT);
            }
            if (implementation_->introResLoc >= 0) {
                glUniform2f(implementation_->introResLoc, static_cast<float>(iw), static_cast<float>(ih));
            }
            glBindVertexArray(fullscreenVAO_);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0u);
        }

        // Wordmark fades in under the mark, then out with the whole intro.
        const float wordIn  = std::clamp((introT - 2.7f) / 1.0f, 0.0f, 1.0f);
        const float wordOut = 1.0f - std::clamp((introT - 4.35f) / 0.55f, 0.0f, 1.0f);
        implementation_->debugOverlay.RenderBrandWordmark(iw, ih, wordIn * wordOut);

        // ENTER skips the intro.
        const bool skip = glfwGetKey(implementation_->window, GLFW_KEY_ENTER) == GLFW_PRESS;
        if (introT >= kIntroDuration || skip) {
            implementation_->introActive = false;
            implementation_->lastFrameTime = now;       // avoid a huge first dt
            implementation_->lastSwitchTime = now;      // don't auto-switch instantly
            implementation_->menuVisibleUntil = now + 9.0; // auto-show controls once
        }
        return;
    }

    const double currentTime = glfwGetTime();
    const float deltaTime = static_cast<float>(currentTime - implementation_->lastFrameTime);
    implementation_->lastFrameTime = currentTime;

    auto& smoothed = implementation_->smoothedUniforms;

    // ── Audio-signal following ─────────────────────────────────────────────────
    // Low visual latency is a primary target, so transients use a fast attack
    // (~18 ms — an onset lands within ~2 frames) and a slower release (~110 ms)
    // to keep the decay smooth.  This keeps the image locked to the beat instead
    // of trailing it, while still avoiding per-frame strobing.
    constexpr float kAttack  = 55.0f; // rise time constant ~18 ms
    constexpr float kRelease = 9.0f;  // fall time constant ~110 ms

    // The preset scales how much energy drives brightness.
    const float targetEnergy = std::clamp(signals.energy * preset.energyMultiplier, 0.0f, 1.0f);
    smoothed.energy    = Follow(smoothed.energy,    targetEnergy,      deltaTime, kAttack, kRelease);
    smoothed.intensity = Follow(smoothed.intensity, signals.intensity, deltaTime, kAttack, kRelease);
    smoothed.bass      = Follow(smoothed.bass,      signals.bass,      deltaTime, kAttack, kRelease);
    smoothed.mid       = Follow(smoothed.mid,       signals.mid,       deltaTime, kAttack, kRelease);
    smoothed.treble    = Follow(smoothed.treble,    signals.treble,    deltaTime, kAttack, kRelease);

    // Beat pulse: instant on the beat frame (zero latency), then a fast decay so
    // it reads as a punch.  Peaks at the preset's beat response.
    if (signals.beat) {
        smoothed.beat = preset.beatResponse;
    } else {
        const float beatDecayRate = 10.0f; // exp(-dt * 10) gives ~37% after 100 ms
        smoothed.beat *= std::exp(-deltaTime * beatDecayRate);
    }

    // Mood is a slow scene property, not a transient — follow it gently.
    smoothed.mood = Follow(smoothed.mood, state.mood, deltaTime, 6.0f, 6.0f);

    // ── Theme colour identity & look ────────────────────────────────────────────
    // Colour, glow, bloom, motion, noise and distortion all come from the active
    // visual::Theme — this is what makes switching themes transform the whole
    // visual identity while the same music plays.  The palette maps
    // secondary → primary → accent across the pattern's dark → bright ramp, with
    // `background` filling the darkest regions.  The theme's transitionSpeed sets
    // the ease rate, so a switch cross-fades the whole image.  Every update is a
    // few float lerps on the persistent smoothed uniforms — a theme switch
    // performs no heap allocation.  All easing is frame-rate independent.
    const float themeAlpha =
        1.0f - std::exp(-deltaTime * std::max(theme.transitionSpeed, 0.0f));

    smoothed.primaryColor   = LerpColor(smoothed.primaryColor,   ToColor3(theme.palette.primary),    themeAlpha);
    smoothed.secondaryColor = LerpColor(smoothed.secondaryColor, ToColor3(theme.palette.secondary),  themeAlpha);
    smoothed.accentColor    = LerpColor(smoothed.accentColor,    ToColor3(theme.palette.accent),     themeAlpha);
    smoothed.background     = LerpColor(smoothed.background,     ToColor3(theme.palette.background), themeAlpha);
    // Visual FX (F10) gate glow/bloom/noise; when off they ease smoothly to zero,
    // leaving the clean base pattern.  Motion and distortion are core animation,
    // not "FX", so they are unaffected.
    const float fx = implementation_->fxEnabled ? 1.0f : 0.0f;
    smoothed.glow       += (theme.glow  * fx    - smoothed.glow)       * themeAlpha;
    smoothed.bloom      += (theme.bloom * fx    - smoothed.bloom)      * themeAlpha;
    smoothed.motion     += (theme.motion        - smoothed.motion)     * themeAlpha;
    smoothed.noise      += (theme.noise * fx    - smoothed.noise)      * themeAlpha;
    smoothed.distortion += (theme.distortion    - smoothed.distortion) * themeAlpha;

    // Master output trims (Demo Mode operator globals) — applied directly.
    smoothed.masterBrightness = implementation_->masterBrightness;
    smoothed.masterGlow       = implementation_->masterGlow;
    smoothed.masterExposure   = implementation_->masterExposure;

    // ── Preset form & behaviour ─────────────────────────────────────────────────
    // The preset owns the *form* axis — which signature pattern is drawn and its
    // saturation / detail — orthogonal to the theme's colour identity.  Its
    // transitionSpeed eases these form parameters across a preset switch.
    const float presetAlpha =
        1.0f - std::exp(-deltaTime * std::max(preset.transitionSpeed, 0.0f));

    smoothed.saturationBase  += (preset.saturationBase  - smoothed.saturationBase)  * presetAlpha;
    smoothed.saturationScale += (preset.saturationScale - smoothed.saturationScale) * presetAlpha;
    smoothed.detail          += (preset.detail          - smoothed.detail)          * presetAlpha;

    // ── Signature form cross-fade ──────────────────────────────────────────────
    // The pattern is a discrete choice, so it can't be lerped like a colour.
    // Instead we snapshot the outgoing form and ease patternBlend 0 → 1; the
    // shader mixes the two forms while the blend runs, then draws only the active
    // form once it completes.  This is state juggling only — no allocation.
    const int incomingMode = static_cast<int>(preset.patternMode);
    if (!implementation_->patternInitialized) {
        implementation_->activePatternMode   = incomingMode;
        implementation_->previousPatternMode = incomingMode;
        implementation_->patternBlend        = 1.0f;
        implementation_->patternInitialized  = true;
    } else if (incomingMode != implementation_->activePatternMode) {
        implementation_->previousPatternMode = implementation_->activePatternMode;
        implementation_->activePatternMode   = incomingMode;
        implementation_->patternBlend        = 0.0f;
    }
    implementation_->patternBlend += (1.0f - implementation_->patternBlend) * presetAlpha;
    if (implementation_->patternBlend > 0.999f) {
        implementation_->patternBlend        = 1.0f;
        implementation_->previousPatternMode = implementation_->activePatternMode;
    }
    smoothed.patternMode         = implementation_->activePatternMode;
    smoothed.previousPatternMode = implementation_->previousPatternMode;
    smoothed.patternBlend        = implementation_->patternBlend;

    // Query framebuffer size for the uResolution uniform and viewport.
    int width  = 0;
    int height = 0;
    glfwGetFramebufferSize(implementation_->window, &width, &height);
    glViewport(0, 0, width, height);

    // Clear to black — the fullscreen triangle overwrites this, but the clear
    // is kept for correctness on framebuffers with a depth/stencil attachment.
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    // ── Shader transition + auto-director ──────────────────────────────────
    // A switch fades the frame out to black, swaps shader, and fades back in.
    constexpr float kFadeDuration = 0.30f;
    if (implementation_->transitioning) {
        if (implementation_->fadingOut) {
            implementation_->fade -= deltaTime / kFadeDuration;
            if (implementation_->fade <= 0.0f) {
                implementation_->fade = 0.0f;
                implementation_->activeShader = implementation_->pendingShader;
                implementation_->fadingOut = false;
            }
        } else {
            implementation_->fade += deltaTime / kFadeDuration;
            if (implementation_->fade >= 1.0f) {
                implementation_->fade = 1.0f;
                implementation_->transitioning = false;
            }
        }
    }

    // Auto-shader: move through the library with the music — on a big energy
    // surge (a drop) or, failing that, on a steady phrase timer.
    if (implementation_->autoShader) {
        implementation_->prevBeat = signals.beat;
        const float ba = 1.0f - std::exp(-deltaTime * 0.4f);
        implementation_->energyBaseline += (signals.energy - implementation_->energyBaseline) * ba;
        const double since  = currentTime - implementation_->lastSwitchTime;
        const bool   drop   = (signals.energy - implementation_->energyBaseline) > 0.28f && signals.energy > 0.45f;
        const bool   timeUp = since > 14.0;
        if (!implementation_->transitioning && since > 5.0 && (drop || timeUp)) {
            const int count = 1 + static_cast<int>(implementation_->shaderLib.size());
            BeginShaderTransition((implementation_->activeShader + 1) % count);
            implementation_->lastSwitchTime = currentTime;
        }
    }

    // ── Fullscreen shader pass ────────────────────────────────────────────
    // The active library slot draws; the same ShaderUniforms feed every shader
    // (each uses whichever uniforms it declares), so audio + theme reactivity is
    // identical across the whole pack.
    const float time = static_cast<float>(currentTime);

    ShaderManager& active = (implementation_->activeShader == 0)
        ? shaderManager_
        : *implementation_->shaderLib[implementation_->activeShader - 1];
    active.Bind();
    active.SetUniforms(smoothed, time, width, height);

    glBindVertexArray(fullscreenVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0u);

    // Fade overlay (black quad) blended on top during a transition.
    if (implementation_->fade < 1.0f && implementation_->fadeProgram != 0) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(implementation_->fadeProgram);
        if (implementation_->fadeAlphaLoc >= 0) {
            glUniform1f(implementation_->fadeAlphaLoc, 1.0f - implementation_->fade);
        }
        glBindVertexArray(fullscreenVAO_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0u);
        glDisable(GL_BLEND);
    }

    // ── Debug overlay (rendered on top, uses its own program internally) ──
    if (implementation_->showDebugOverlay) {
        DebugState stateWithFps = debugState;
        stateWithFps.fps             = static_cast<float>(implementation_->currentFps);
        stateWithFps.rendererBackend = BackendName();
        stateWithFps.windowWidth     = width;
        stateWithFps.windowHeight    = height;
        stateWithFps.currentShader   = implementation_->shaderNames[
                                          static_cast<std::size_t>(implementation_->activeShader)].c_str();
        implementation_->debugOverlay.Render(stateWithFps, width, height);
    }

    // Brief shader-name toast after a switch (shown even with the overlay off).
    if (currentTime < implementation_->toastUntil) {
        implementation_->debugOverlay.RenderToast(
            implementation_->toastText.c_str(), width, height);
    }

    // Soundcheck input-level meter.
    if (implementation_->showMeter) {
        implementation_->debugOverlay.RenderMeter(debugState, width, height);
    }

    // Live-controls menu: sticky while toggled on (H), or auto-shown at startup
    // for a few seconds (fading out over the last second).
    float menuAlpha = 0.0f;
    if (implementation_->showMenu) {
        menuAlpha = 1.0f;
    } else if (currentTime < implementation_->menuVisibleUntil) {
        menuAlpha = std::clamp(
            static_cast<float>(implementation_->menuVisibleUntil - currentTime), 0.0f, 1.0f);
    }
    if (menuAlpha > 0.0f) {
        implementation_->debugOverlay.RenderMenu(width, height, menuAlpha);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// EndFrame
// ──────────────────────────────────────────────────────────────────────────────
bool OpenGLRenderer::EndFrame() {
    if (!initialized_) {
        return false;
    }

    glfwSwapBuffers(implementation_->window);
    glfwPollEvents();

    // ── Live controls (Task 7) ──────────────────────────────────────────────────
    // F1..F7 request theme slots 0..6.  Latch the rising edge here (events were
    // just polled) and let the Runtime drain it via ConsumeThemeRequest.
    constexpr int kThemeKeys[Implementation::kThemeKeyCount] = {
        GLFW_KEY_F1, GLFW_KEY_F2, GLFW_KEY_F3, GLFW_KEY_F4,
        GLFW_KEY_F5, GLFW_KEY_F6, GLFW_KEY_F7,
    };
    for (int i = 0; i < Implementation::kThemeKeyCount; ++i) {
        const bool pressed =
            glfwGetKey(implementation_->window, kThemeKeys[i]) == GLFW_PRESS;
        if (pressed && !implementation_->themeKeyWasPressed[i]) {
            implementation_->pendingThemeRequest = i;
        }
        implementation_->themeKeyWasPressed[i] = pressed;
    }

    // 'B' returns to the home theme (Badman red) — the show's brand colour.
    const bool homeIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_B) == GLFW_PRESS;
    if (homeIsPressed && !implementation_->homeKeyWasPressed) {
        implementation_->pendingHomeRequest = true;
    }
    implementation_->homeKeyWasPressed = homeIsPressed;

    // Space toggles audio play/pause.
    const bool spaceIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (spaceIsPressed && !implementation_->spaceWasPressed) {
        implementation_->pendingPlayPause = true;
    }
    implementation_->spaceWasPressed = spaceIsPressed;

    // F8 reloads the current theme's JSON from disk (no restart).
    const bool reloadIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_F8) == GLFW_PRESS;
    if (reloadIsPressed && !implementation_->reloadKeyWasPressed) {
        implementation_->pendingReload = true;
    }
    implementation_->reloadKeyWasPressed = reloadIsPressed;

    // 'A' toggles Auto-VJ (automatic preset/form selection).
    const bool autoKeyIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_A) == GLFW_PRESS;
    if (autoKeyIsPressed && !implementation_->autoKeyWasPressed) {
        implementation_->pendingAutoToggle = true;
    }
    implementation_->autoKeyWasPressed = autoKeyIsPressed;

    // F9 toggles demo presentation mode (fullscreen + cursor hide + no overlay).
    const bool demoKeyIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_F9) == GLFW_PRESS;
    if (demoKeyIsPressed && !implementation_->demoKeyWasPressed) {
        SetDemoMode(!implementation_->demoMode);
    }
    implementation_->demoKeyWasPressed = demoKeyIsPressed;

    // F10 toggles visual FX (glow / bloom / noise).
    const bool fxKeyIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_F10) == GLFW_PRESS;
    if (fxKeyIsPressed && !implementation_->fxKeyWasPressed) {
        implementation_->fxEnabled = !implementation_->fxEnabled;
    }
    implementation_->fxKeyWasPressed = fxKeyIsPressed;

    // F11 toggles fullscreen.
    const bool fsKeyIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_F11) == GLFW_PRESS;
    if (fsKeyIsPressed && !implementation_->fullscreenKeyWasPressed) {
        SetFullscreen(!implementation_->isFullscreen);
    }
    implementation_->fullscreenKeyWasPressed = fsKeyIsPressed;

    // '[' / ']' cycle the active shader through the library (signature + pack).
    const int shaderCount = 1 + static_cast<int>(implementation_->shaderLib.size());
    const auto switchShader = [&](int delta) {
        BeginShaderTransition((implementation_->activeShader + delta + shaderCount) % shaderCount);
    };
    const bool prevSh = glfwGetKey(implementation_->window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS;
    if (prevSh && !implementation_->prevShaderKeyPressed && shaderCount > 1) switchShader(-1);
    implementation_->prevShaderKeyPressed = prevSh;
    const bool nextSh = glfwGetKey(implementation_->window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS;
    if (nextSh && !implementation_->nextShaderKeyPressed && shaderCount > 1) switchShader(1);
    implementation_->nextShaderKeyPressed = nextSh;

    // 'V' toggles the music-driven auto-shader director.
    const bool vKey = glfwGetKey(implementation_->window, GLFW_KEY_V) == GLFW_PRESS;
    if (vKey && !implementation_->autoShaderKeyWasPressed) {
        implementation_->autoShader = !implementation_->autoShader;
        implementation_->lastSwitchTime = glfwGetTime();
        implementation_->toastText  = implementation_->autoShader ? "Auto-shader: ON" : "Auto-shader: OFF";
        implementation_->toastUntil = glfwGetTime() + 2.0;
    }
    implementation_->autoShaderKeyWasPressed = vKey;

    // 'M' toggles the soundcheck input-level meter.
    const bool meterKeyIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_M) == GLFW_PRESS;
    if (meterKeyIsPressed && !implementation_->meterKeyWasPressed) {
        implementation_->showMeter = !implementation_->showMeter;
    }
    implementation_->meterKeyWasPressed = meterKeyIsPressed;

    // 'H' toggles the live-controls menu (all keys + features).
    const bool menuKeyIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_H) == GLFW_PRESS;
    if (menuKeyIsPressed && !implementation_->menuKeyWasPressed) {
        implementation_->showMenu = !implementation_->showMenu;
        implementation_->menuVisibleUntil = 0.0; // cancel any startup auto-show
    }
    implementation_->menuKeyWasPressed = menuKeyIsPressed;

    // F12 toggles the debug overlay.
    const bool debugKeyIsPressed =
        glfwGetKey(implementation_->window, GLFW_KEY_F12) == GLFW_PRESS;
    if (debugKeyIsPressed && !implementation_->debugKeyWasPressed) {
        implementation_->showDebugOverlay = !implementation_->showDebugOverlay;
    }
    implementation_->debugKeyWasPressed = debugKeyIsPressed;

    // ESC requests application exit (EndFrame then reports the close).
    if (glfwGetKey(implementation_->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(implementation_->window, GLFW_TRUE);
    }

    // Cursor auto-hide: hide after a few idle seconds in demo/fullscreen, and
    // restore the moment the mouse moves.
    {
        double cx = 0.0;
        double cy = 0.0;
        glfwGetCursorPos(implementation_->window, &cx, &cy);
        const double now = glfwGetTime();
        const double dx = cx - implementation_->lastCursorX;
        const double dy = cy - implementation_->lastCursorY;
        if (dx * dx + dy * dy > 4.0) { // moved more than ~2 px
            implementation_->lastActivityTime = now;
            implementation_->lastCursorX = cx;
            implementation_->lastCursorY = cy;
            if (implementation_->cursorHidden) {
                glfwSetInputMode(implementation_->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                implementation_->cursorHidden = false;
            }
        } else if (!implementation_->cursorHidden &&
                   (implementation_->demoMode || implementation_->isFullscreen) &&
                   now - implementation_->lastActivityTime > 3.0) {
            glfwSetInputMode(implementation_->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            implementation_->cursorHidden = true;
        }
    }

    // FPS counter and window title update.
    ++implementation_->renderedFrameCount;
    const double currentTime  = glfwGetTime();
    const double elapsedTime  = currentTime - implementation_->lastFpsUpdateTime;
    if (elapsedTime >= 1.0) {
        implementation_->currentFps =
            static_cast<double>(implementation_->renderedFrameCount) / elapsedTime;

        std::ostringstream title;
        title << "PAPAGEDON Core v0.0.1 | FPS: "
              << std::lround(implementation_->currentFps);
        glfwSetWindowTitle(implementation_->window, title.str().c_str());

        implementation_->renderedFrameCount = 0;
        implementation_->lastFpsUpdateTime  = currentTime;
    }

    return glfwWindowShouldClose(implementation_->window) == GLFW_FALSE;
}

// ──────────────────────────────────────────────────────────────────────────────
// ConsumePresetRequest
// ──────────────────────────────────────────────────────────────────────────────
int OpenGLRenderer::ConsumeThemeRequest() noexcept {
    if (implementation_ == nullptr) {
        return -1;
    }
    const int request = implementation_->pendingThemeRequest;
    implementation_->pendingThemeRequest = -1;
    return request;
}

bool OpenGLRenderer::ConsumeHomeThemeRequest() noexcept {
    if (implementation_ == nullptr) {
        return false;
    }
    const bool requested = implementation_->pendingHomeRequest;
    implementation_->pendingHomeRequest = false;
    return requested;
}

bool OpenGLRenderer::ConsumePlayPauseToggle() noexcept {
    if (implementation_ == nullptr) {
        return false;
    }
    const bool toggled = implementation_->pendingPlayPause;
    implementation_->pendingPlayPause = false;
    return toggled;
}

bool OpenGLRenderer::ConsumeReloadRequest() noexcept {
    if (implementation_ == nullptr) {
        return false;
    }
    const bool requested = implementation_->pendingReload;
    implementation_->pendingReload = false;
    return requested;
}

bool OpenGLRenderer::ConsumeAutoToggle() noexcept {
    if (implementation_ == nullptr) {
        return false;
    }
    const bool toggled = implementation_->pendingAutoToggle;
    implementation_->pendingAutoToggle = false;
    return toggled;
}

// ──────────────────────────────────────────────────────────────────────────────
// Shutdown
// ──────────────────────────────────────────────────────────────────────────────
void OpenGLRenderer::Shutdown() noexcept {
    if (implementation_ == nullptr || !initialized_) {
        return;
    }

    implementation_->debugOverlay.Shutdown();

    shaderManager_.Shutdown();
    for (auto& sm : implementation_->shaderLib) {
        if (sm) sm->Shutdown();
    }
    implementation_->shaderLib.clear();
    if (implementation_->fadeProgram != 0) {
        glDeleteProgram(implementation_->fadeProgram);
        implementation_->fadeProgram = 0;
    }
    if (implementation_->introProgram != 0) {
        glDeleteProgram(implementation_->introProgram);
        implementation_->introProgram = 0;
    }

    if (fullscreenVAO_ != 0u) {
        glDeleteVertexArrays(1, &fullscreenVAO_);
        fullscreenVAO_ = 0u;
    }

    glfwDestroyWindow(implementation_->window);
    implementation_->window = nullptr;
    glfwTerminate();
    initialized_ = false;
}

} // namespace papagedon
