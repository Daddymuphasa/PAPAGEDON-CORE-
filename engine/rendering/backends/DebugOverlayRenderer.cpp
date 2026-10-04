#include "DebugOverlayRenderer.h"
#include "font8x8_basic.h"

#include <glad/glad.h>
#include <algorithm>
#include <cmath>
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
        uniform int   uSolid;
        void main() {
            if (uSolid == 0) {
                float r = texture(textTexture, TexCoord).r;
                if (r < 0.5) discard;
            }
            FragColor = vec4(uColor, uAlpha);
        }
    )";

    const char* bannerFragmentSource = R"(
        #version 410 core
        out vec4 FragColor;
        in vec2 TexCoord;
        uniform sampler2D textTexture;
        uniform vec3  uColor;
        uniform float uAlpha;
        void main() {
            float d = texture(textTexture, TexCoord).r;
            float a = smoothstep(0.30, 0.60, d) * uAlpha;
            if (a < 0.01) discard;
            FragColor = vec4(uColor, a);
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
    solidLoc_ = glGetUniformLocation(shaderProgram_, "uSolid");

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

    // ── Banner SDF font atlas (smooth, anti-aliased text for the brand banner) ──
    {
        constexpr int kG = 48;
        constexpr int kCols = 16;
        constexpr int kRows = 8;
        constexpr int kAW = kG * kCols;
        constexpr int kAH = kG * kRows;
        std::vector<unsigned char> sdf(kAW * kAH);

        for (int ch = 0; ch < 128; ++ch) {
            const int bx = (ch % kCols) * kG;
            const int by = (ch / kCols) * kG;
            for (int py = 0; py < kG; ++py) {
                for (int px = 0; px < kG; ++px) {
                    const int sx = px * 8 / kG;
                    const int sy = py * 8 / kG;
                    const bool inside = (font8x8_basic[ch][sy] & (1 << sx)) != 0;
                    float minD = 99.0F;
                    for (int ey = 0; ey < 8; ++ey) {
                        for (int ex = 0; ex < 8; ++ex) {
                            if (((font8x8_basic[ch][ey] & (1 << ex)) != 0) != inside) {
                                const float dx = (px + 0.5F) * 8.0F / kG - (ex + 0.5F);
                                const float dy = (py + 0.5F) * 8.0F / kG - (ey + 0.5F);
                                const float d = std::sqrt(dx * dx + dy * dy);
                                if (d < minD) minD = d;
                            }
                        }
                    }
                    float norm = minD / 3.0F;
                    if (norm > 1.0F) norm = 1.0F;
                    float val = inside ? 0.5F + norm * 0.5F : 0.5F - norm * 0.5F;
                    sdf[static_cast<std::size_t>((by + py) * kAW + (bx + px))] =
                        static_cast<unsigned char>(val * 255.0F);
                }
            }
        }

        glGenTextures(1, &bannerTexture_);
        glBindTexture(GL_TEXTURE_2D, bannerTexture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, kAW, kAH, 0,
                     GL_RED, GL_UNSIGNED_BYTE, sdf.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    {
        unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
        unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, bannerFragmentSource);
        bannerProgram_ = glCreateProgram();
        glAttachShader(bannerProgram_, vs);
        glAttachShader(bannerProgram_, fs);
        glLinkProgram(bannerProgram_);
        glDeleteShader(vs);
        glDeleteShader(fs);
        bannerColorLoc_ = glGetUniformLocation(bannerProgram_, "uColor");
        bannerAlphaLoc_ = glGetUniformLocation(bannerProgram_, "uAlpha");
    }

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
        {"   I         cycle live audio input", false},
        {"   F8        reload shaders & theme", false},
        {"", false},
        {" COLOUR", true},
        {"   T         TRANCE - romantic / elegant", false},
        {"   B         BADMAN - Amapiano after party", false},
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

void DebugOverlayRenderer::RenderRect(float x, float y, float w, float h,
                                      int windowWidth, int windowHeight,
                                      float r, float g, float b, float a) {
    glUseProgram(shaderProgram_);
    if (colorLoc_ >= 0) glUniform3f(colorLoc_, r, g, b);
    if (alphaLoc_ >= 0) glUniform1f(alphaLoc_, a);
    if (solidLoc_ >= 0) glUniform1i(solidLoc_, 1);
    glBindVertexArray(vao_);

    const float ww = static_cast<float>(windowWidth);
    const float wh = static_cast<float>(windowHeight);
    const float x1 = (x / ww) * 2.0F - 1.0F;
    const float x2 = ((x + w) / ww) * 2.0F - 1.0F;
    const float y1 = (y / wh) * 2.0F - 1.0F;
    const float y2 = ((y + h) / wh) * 2.0F - 1.0F;

    float verts[] = {
        x1, y2, 0, 0,   x1, y1, 0, 1,   x2, y1, 1, 1,
        x1, y2, 0, 0,   x2, y1, 1, 1,   x2, y2, 1, 0,
    };
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    if (solidLoc_ >= 0) glUniform1i(solidLoc_, 0);
}

void DebugOverlayRenderer::RenderBannerText(const char* text, float x, float y, float scale,
                                            int windowWidth, int windowHeight,
                                            float r, float g, float b, float a) {
    glUseProgram(bannerProgram_);
    if (bannerColorLoc_ >= 0) glUniform3f(bannerColorLoc_, r, g, b);
    if (bannerAlphaLoc_ >= 0) glUniform1f(bannerAlphaLoc_, a);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bannerTexture_);
    glBindVertexArray(vao_);

    std::vector<float> vertices;
    const float charW = 8.0F * scale;
    const float charH = 8.0F * scale;
    float startX = x;

    while (*text) {
        char c = *text;
        if (c == '\n') { y -= charH * 1.5F; x = startX; }
        else if (c >= 0 && c < 128) {
            float x1 = (x / windowWidth) * 2.0F - 1.0F;
            float x2 = ((x + charW) / windowWidth) * 2.0F - 1.0F;
            float y1 = (y / windowHeight) * 2.0F - 1.0F;
            float y2 = ((y + charH) / windowHeight) * 2.0F - 1.0F;
            float u1 = static_cast<float>(c % 16) / 16.0F;
            float u2 = u1 + 1.0F / 16.0F;
            float v1 = static_cast<float>(c / 16) / 8.0F;
            float v2 = v1 + 1.0F / 8.0F;
            float vd[] = {x1,y2,u1,v1, x1,y1,u1,v2, x2,y1,u2,v2,
                          x1,y2,u1,v1, x2,y1,u2,v2, x2,y2,u2,v1};
            for (float f : vd) vertices.push_back(f);
            x += charW;
        }
        ++text;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data());
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 4));
}

void DebugOverlayRenderer::RenderTranceWordmark(
    int windowWidth, int windowHeight, float time, float alpha) {
    if (!initialized_ || alpha <= 0.0F) {
        return;
    }
    const float w = static_cast<float>(windowWidth);
    const float h = static_cast<float>(windowHeight);

    const float scale = std::max(5.0F, w / 180.0F);
    const float width = 6.0F * 8.0F * scale;
    const float x = (w - width) * 0.5F;
    const float travel = std::clamp(time / 2.4F, 0.0F, 1.0F);
    const float easedTravel = travel * travel * (3.0F - 2.0F * travel);
    const float y = h * (0.48F + 0.40F * easedTravel) +
                    std::sin(time * 0.72F) * (h * 0.008F);
    const float breath = 0.76F + 0.16F * std::sin(time * 0.54F);
    RenderBannerText("TRANCE", x, y, scale * 1.025F,
                     windowWidth, windowHeight, 0.30F, 0.38F, 0.82F,
                     alpha * breath * 0.28F);
    RenderBannerText("TRANCE", x, y, scale,
                     windowWidth, windowHeight, 0.88F, 0.76F, 1.0F,
                     alpha * breath);
}

void DebugOverlayRenderer::RenderBadmanWordmark(
    int windowWidth, int windowHeight, float time, float alpha) {
    if (!initialized_ || alpha <= 0.0F) {
        return;
    }
    const float w = static_cast<float>(windowWidth);
    const float h = static_cast<float>(windowHeight);
    const float scale = std::max(5.0F, w / 190.0F);
    const float subScale = std::max(2.0F, w / 420.0F);
    const float y = h * 0.48F + std::sin(time * 1.1F) * (h * 0.006F);
    const float breath = 0.80F + 0.20F * std::sin(time * 0.8F);
    const float titleWidth = 6.0F * 8.0F * scale;
    const float subtitleWidth = 20.0F * 8.0F * subScale;
    RenderBannerText("BADMAN", (w - titleWidth) * 0.5F, y + 8.0F * scale,
                     scale * 1.02F, windowWidth, windowHeight,
                     0.92F, 0.12F, 0.24F, alpha * breath * 0.30F);
    RenderBannerText("BADMAN", (w - titleWidth) * 0.5F, y + 8.0F * scale,
                     scale, windowWidth, windowHeight,
                     1.0F, 0.68F, 0.56F, alpha * breath);
    RenderBannerText("AMAPIANO AFTER PARTY", (w - subtitleWidth) * 0.5F, y,
                     subScale, windowWidth, windowHeight,
                     1.0F, 0.82F, 0.72F, alpha * breath * 0.9F);
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
    if (solidLoc_ >= 0) glUniform1i(solidLoc_, 0);
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
    if (bannerProgram_) glDeleteProgram(bannerProgram_);
    if (bannerTexture_) glDeleteTextures(1, &bannerTexture_);
    initialized_ = false;
}

} // namespace papagedon
