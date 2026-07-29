#include "DebugOverlayRenderer.h"
#include "font8x8_basic.h"

#include <glad/glad.h>
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
        void main() {
            float r = texture(textTexture, TexCoord).r;
            if (r < 0.5) discard;
            FragColor = vec4(1.0, 1.0, 1.0, 1.0);
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

void DebugOverlayRenderer::RenderSplash(const char* version, const char* status,
                                        float progress, int windowWidth, int windowHeight) {
    if (!initialized_) {
        return;
    }
    progress = progress < 0.0F ? 0.0F : (progress > 1.0F ? 1.0F : progress);

    const auto centeredX = [&](int chars, float scale) {
        return (static_cast<float>(windowWidth) - static_cast<float>(chars) * 8.0F * scale) * 0.5F;
    };

    // Logo (placeholder wordmark).
    const char* kLogo = "PAPAGEDON";
    const float logoScale = 6.0F;
    RenderText(kLogo, centeredX(9, logoScale),
               static_cast<float>(windowHeight) * 0.60F, logoScale, windowWidth, windowHeight);

    // Version.
    const float verScale = 2.0F;
    RenderText(version, centeredX(static_cast<int>(std::string(version).size()), verScale),
               static_cast<float>(windowHeight) * 0.50F, verScale, windowWidth, windowHeight);

    // Text progress bar: [########      ]
    constexpr int kCells = 24;
    const int filled = static_cast<int>(progress * kCells + 0.5F);
    std::string bar = "[";
    for (int i = 0; i < kCells; ++i) bar += (i < filled) ? '#' : ' ';
    bar += "]";
    const float barScale = 2.0F;
    RenderText(bar.c_str(), centeredX(static_cast<int>(bar.size()), barScale),
               static_cast<float>(windowHeight) * 0.40F, barScale, windowWidth, windowHeight);

    // Status line.
    const float statusScale = 2.0F;
    RenderText(status, centeredX(static_cast<int>(std::string(status).size()), statusScale),
               static_cast<float>(windowHeight) * 0.34F, statusScale, windowWidth, windowHeight);
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

void DebugOverlayRenderer::RenderText(const char* text, float x, float y, float scale, int windowWidth, int windowHeight) {
    glUseProgram(shaderProgram_);
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
