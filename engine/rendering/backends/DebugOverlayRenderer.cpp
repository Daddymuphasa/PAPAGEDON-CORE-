#include "DebugOverlayRenderer.h"
#include "font8x8_basic.h"

#include <glad/glad.h>
#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace papagedon {

namespace {
    const char* vertexShaderSource = R"(
        #version 410 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            TexCoord = aTexCoord;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 410 core
        out vec4 FragColor;
        in vec2 TexCoord;
        uniform sampler2D textTexture;
        uniform vec3  uColor;
        uniform float uAlpha;
        void main() {
            float r = texture(textTexture, TexCoord).r;
            if (r < 0.5) discard;
            FragColor = vec4(uColor, uAlpha);
        }
    )";

    unsigned int CompileShader(unsigned int type, const char* source) {
        unsigned int id = glCreateShader(type);
        glShaderSource(id, 1, &source, nullptr);
        glCompileShader(id);
        
        int success;
        glGetShaderiv(id, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(id, 512, nullptr, infoLog);
            std::cerr << "Shader compile error:\n" << infoLog << "\n";
        }
        return id;
    }
}

DebugOverlayRenderer::DebugOverlayRenderer() = default;

DebugOverlayRenderer::~DebugOverlayRenderer() {
    Shutdown();
}

bool DebugOverlayRenderer::Initialize() {
    if (initialized_) return true;

    unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    shaderProgram_ = glCreateProgram();
    glAttachShader(shaderProgram_, vertexShader);
    glAttachShader(shaderProgram_, fragmentShader);
    glLinkProgram(shaderProgram_);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    colorLoc_ = glGetUniformLocation(shaderProgram_, "uColor");
    alphaLoc_ = glGetUniformLocation(shaderProgram_, "uAlpha");

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    // Max 1024 characters per draw, 6 vertices per char, 4 floats per vertex (x,y,u,v)
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4 * 1024, nullptr, GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);

    // Generate texture data
    std::vector<unsigned char> texData;
    texData.resize(128 * 8 * 8); // 128 chars, 8x8 pixels each
    for (int c = 0; c < 128; ++c) {
        for (int y = 0; y < 8; ++y) {
            char row = font8x8_basic[c][y];
            for (int x = 0; x < 8; ++x) {
                // font8x8 uses LSB for left-most pixel
                bool pixel = (row & (1 << x)) != 0;
                texData[(y * 128 * 8) + (c * 8) + x] = pixel ? 255 : 0;
            }
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 128 * 8, 8, 0, GL_RED, GL_UNSIGNED_BYTE, texData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    initialized_ = true;
    return true;
}

void DebugOverlayRenderer::Render(const DebugState& state, int windowWidth, int windowHeight) {
    if (!initialized_) return;

    const float frameTimeMs = state.fps > 0.0F ? 1000.0F / state.fps : 0.0F;

    std::ostringstream ss;
    ss << std::fixed;
    ss << "PAPAGEDON Debug Overlay\n\n";
    ss << std::setprecision(0);
    ss << "FPS:               " << state.fps << "\n";
    ss << std::setprecision(2);
    ss << "Frame Time:        " << frameTimeMs << " ms\n";
    ss << "Current Theme:     " << state.currentTheme << "\n";
    ss << "Current Shader:    " << state.currentShader << "\n";
    ss << std::setprecision(1);
    ss << "BPM:               " << state.bpm << "\n";
    ss << std::setprecision(2);
    ss << "Bass:              " << state.bass << "\n";
    ss << "Mid:               " << state.mid << "\n";
    ss << "Treble:            " << state.treble << "\n";
    ss << "Energy:            " << state.energy << "\n";
    ss << "Beat:              " << (state.beat ? "*" : "-") << "\n";
    ss << "ExperienceState:   " << state.currentExperience << "\n";
    ss << "Audio File:        " << state.currentAudioFile << "\n";
    ss << "Renderer:          " << state.rendererBackend << "\n";
    ss << "Resolution:        " << state.windowWidth << "x" << state.windowHeight << "\n";

    RenderText(ss.str().c_str(), 10.0F, windowHeight - 20.0F, 2.0F, windowWidth, windowHeight);
}

void DebugOverlayRenderer::RenderSplash(const char* /*version*/, const char* status,
                                        float progress, int windowWidth, int windowHeight) {
    if (!initialized_) {
        return;
    }
    progress = progress < 0.0F ? 0.0F : (progress > 1.0F ? 1.0F : progress);

    // Minimal, elegant loading frame: a dim gold wordmark that brightens with
    // progress, plus a small status line.  The animated logo intro follows.
    RenderBrandWordmark(windowWidth, windowHeight, 0.25F + 0.35F * progress);

    const float statusScale = 2.0F;
    const int statusLen = static_cast<int>(std::string(status).size());
    const float sx = (static_cast<float>(windowWidth) - static_cast<float>(statusLen) * 8.0F * statusScale) * 0.5F;
    RenderTextShadowed(status, sx, static_cast<float>(windowHeight) * 0.22F, statusScale,
                       windowWidth, windowHeight, 0.62F, 0.68F, 0.60F, 0.7F);
}

void DebugOverlayRenderer::RenderTextShadowed(const char* text, float x, float y, float scale,
                                              int windowWidth, int windowHeight,
                                              float r, float g, float b, float a) {
    const float off = scale;                      // ~1 texel of shadow offset
    RenderText(text, x + off, y - off, scale, windowWidth, windowHeight,
               0.02F, 0.03F, 0.02F, a * 0.85F);   // dark drop shadow
    RenderText(text, x, y, scale, windowWidth, windowHeight, r, g, b, a);
}

void DebugOverlayRenderer::RenderBrandWordmark(int windowWidth, int windowHeight, float alpha) {
    if (!initialized_ || alpha <= 0.0F) {
        return;
    }
    // Letter-spaced caps evoke the logo's wordmark.
    const char* kWord = "P A P A G E D O N";
    const int len = static_cast<int>(std::string(kWord).size());
    const float scale = std::max(3.0F, static_cast<float>(windowWidth) / 320.0F);
    const float x = (static_cast<float>(windowWidth) - static_cast<float>(len) * 8.0F * scale) * 0.5F;
    const float y = static_cast<float>(windowHeight) * 0.30F;
    RenderTextShadowed(kWord, x, y, scale, windowWidth, windowHeight,
                       0.85F, 0.66F, 0.32F, alpha); // brand gold
}

void DebugOverlayRenderer::RenderMenu(int windowWidth, int windowHeight, float alpha) {
    if (!initialized_ || alpha <= 0.0F) {
        return;
    }
    struct Line { const char* text; bool header; };
    static const Line kLines[] = {
        {"PAPAGEDON   -   LIVE CONTROLS", true},
        {"", false},
        {" VISUALS", true},
        {"   [   ]     prev / next shader", false},
        {"   V         auto-shader  (music-driven)", false},
        {"   SPACE     play / pause audio", false},
        {"   F8        reload shaders & theme", false},
        {"", false},
        {" COLOUR", true},
        {"   B         RED  -  Badman  (home)", false},
        {"   F1 - F7   cyber / techno / industrial /", false},
        {"             rave / aurora / nebula / matrix", false},
        {"   F10       FX on/off  (glow / bloom)", false},
        {"", false},
        {" SHOW", true},
        {"   A         auto-VJ presets", false},
        {"   F9        demo / presentation mode", false},
        {"   F11       fullscreen", false},
        {"   F12       debug overlay", false},
        {"", false},
        {" SOUNDCHECK", true},
        {"   M         input level meter", false},
        {"   H         show / hide this menu", false},
        {"   ESC       quit", false},
    };

    const float scale = std::max(1.5F, static_cast<float>(windowHeight) / 460.0F);
    const float lineH = 8.0F * scale * 1.5F;
    const float x = std::max(40.0F, static_cast<float>(windowWidth) * 0.06F);
    float y = static_cast<float>(windowHeight) * 0.90F;

    for (const Line& ln : kLines) {
        if (ln.text[0] != '\0') {
            if (ln.header) {
                RenderTextShadowed(ln.text, x, y, scale, windowWidth, windowHeight,
                                   0.95F, 0.75F, 0.34F, alpha);          // gold heading
            } else {
                RenderTextShadowed(ln.text, x, y, scale, windowWidth, windowHeight,
                                   0.90F, 0.90F, 0.84F, alpha * 0.95F);  // warm-white entry
            }
        }
        y -= lineH;
    }
}

void DebugOverlayRenderer::RenderToast(const char* text, int windowWidth, int windowHeight) {
    if (!initialized_ || text == nullptr || *text == '\0') {
        return;
    }
    const int len = static_cast<int>(std::string(text).size());
    const float scale = 3.0F;
    const float x = (static_cast<float>(windowWidth) - static_cast<float>(len) * 8.0F * scale) * 0.5F;
    const float y = static_cast<float>(windowHeight) * 0.12F;
    RenderText(text, x, y, scale, windowWidth, windowHeight);
}

void DebugOverlayRenderer::RenderMeter(const DebugState& state, int windowWidth, int windowHeight) {
    if (!initialized_) {
        return;
    }
    const auto bar = [](float v) {
        v = v < 0.0F ? 0.0F : (v > 1.0F ? 1.0F : v);
        constexpr int kCells = 22;
        const int filled = static_cast<int>(v * kCells + 0.5F);
        std::string b = "[";
        for (int i = 0; i < kCells; ++i) b += (i < filled) ? '#' : ' ';
        b += "]";
        return b;
    };
    float level = state.bass;
    level = state.mid > level ? state.mid : level;
    level = state.treble > level ? state.treble : level;
    level = state.energy > level ? state.energy : level;

    std::ostringstream ss;
    ss << "INPUT LEVEL  (M to hide)\n\n";
    ss << "BASS    " << bar(state.bass)   << "\n";
    ss << "MID     " << bar(state.mid)    << "\n";
    ss << "TREBLE  " << bar(state.treble) << "\n";
    ss << "ENERGY  " << bar(state.energy) << "\n\n";
    ss << (level > 0.02F ? "SIGNAL: OK" : "SIGNAL: -- (silent)") << "\n";
    ss << "SOURCE: " << state.currentAudioFile << "\n";

    RenderText(ss.str().c_str(), 40.0F, static_cast<float>(windowHeight) * 0.72F,
               2.5F, windowWidth, windowHeight);
}

void DebugOverlayRenderer::RenderText(const char* text, float x, float y, float scale,
                                      int windowWidth, int windowHeight,
                                      float r, float g, float b, float a) {
    glUseProgram(shaderProgram_);
    if (colorLoc_ >= 0) glUniform3f(colorLoc_, r, g, b);
    if (alphaLoc_ >= 0) glUniform1f(alphaLoc_, a);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glBindVertexArray(vao_);

    std::vector<float> vertices;
    
    float startX = x;
    const float charW = 8.0F * scale;
    const float charH = 8.0F * scale;

    while (*text) {
        char c = *text;
        if (c == '\n') {
            y -= charH * 1.5F;
            x = startX;
        } else if (c >= 0 && c < 128) {
            float xpos = x;
            float ypos = y;

            float w = charW;
            float h = charH;

            // map to NDC
            float x1 = (xpos / windowWidth) * 2.0F - 1.0F;
            float x2 = ((xpos + w) / windowWidth) * 2.0F - 1.0F;
            float y1 = (ypos / windowHeight) * 2.0F - 1.0F;
            float y2 = ((ypos + h) / windowHeight) * 2.0F - 1.0F;

            float u1 = static_cast<float>(c * 8) / (128.0F * 8.0F);
            float u2 = static_cast<float>(c * 8 + 8) / (128.0F * 8.0F);
            float v1 = 0.0F;
            float v2 = 1.0F;

            // Triangle 1
            vertices.push_back(x1); vertices.push_back(y2); vertices.push_back(u1); vertices.push_back(v1); // Top-left
            vertices.push_back(x1); vertices.push_back(y1); vertices.push_back(u1); vertices.push_back(v2); // Bottom-left
            vertices.push_back(x2); vertices.push_back(y1); vertices.push_back(u2); vertices.push_back(v2); // Bottom-right
            
            // Triangle 2
            vertices.push_back(x1); vertices.push_back(y2); vertices.push_back(u1); vertices.push_back(v1); // Top-left
            vertices.push_back(x2); vertices.push_back(y1); vertices.push_back(u2); vertices.push_back(v2); // Bottom-right
            vertices.push_back(x2); vertices.push_back(y2); vertices.push_back(u2); vertices.push_back(v1); // Top-right
            
            x += charW;
        }
        ++text;
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 4));
}

void DebugOverlayRenderer::Shutdown() noexcept {
    if (!initialized_) return;
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
    glDeleteProgram(shaderProgram_);
    glDeleteTextures(1, &texture_);
    initialized_ = false;
}

} // namespace papagedon
