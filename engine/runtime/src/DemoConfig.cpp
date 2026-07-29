#include <papagedon/runtime/DemoConfig.h>

#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace papagedon::runtime {
namespace {

// A parsed scalar value from the flat config object.
struct Scalar final {
    enum class Type { String, Number, Bool, Null } type = Type::Null;
    std::string str;
    double num = 0.0;
    bool boolean = false;
};

// Minimal flat-JSON reader: parses a top-level object whose values are strings,
// numbers, booleans, or null.  Nested objects/arrays are tolerated but skipped.
// Deliberately tiny and dependency-free — the config schema is flat.
class FlatJson final {
public:
    explicit FlatJson(std::string_view s) noexcept : s_{s} {}

    bool Parse(std::vector<std::pair<std::string, Scalar>>& out) {
        Skip();
        if (Eof() || Peek() != '{') return false;
        ++i_;
        Skip();
        if (!Eof() && Peek() == '}') return true;
        while (!Eof()) {
            Skip();
            if (Eof() || Peek() != '"') return false;
            std::string key = ParseString();
            Skip();
            if (Eof() || Peek() != ':') return false;
            ++i_;
            Scalar value;
            if (!ParseValue(value)) return false;
            out.emplace_back(std::move(key), std::move(value));
            Skip();
            if (Eof()) return false;
            const char c = Peek();
            if (c == ',') { ++i_; continue; }
            if (c == '}') return true;
            return false;
        }
        return false;
    }

private:
    std::string_view s_;
    std::size_t i_ = 0;

    [[nodiscard]] bool Eof() const noexcept { return i_ >= s_.size(); }
    [[nodiscard]] char Peek() const noexcept { return s_[i_]; }

    void Skip() noexcept {
        while (!Eof()) {
            const char c = Peek();
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') ++i_;
            else break;
        }
    }

    std::string ParseString() {
        ++i_; // opening quote
        std::string r;
        while (!Eof()) {
            const char c = s_[i_++];
            if (c == '"') break;
            if (c == '\\' && !Eof()) {
                const char e = s_[i_++];
                switch (e) {
                case 'n': r += '\n'; break;
                case 't': r += '\t'; break;
                case 'r': r += '\r'; break;
                case '"': r += '"';  break;
                case '\\': r += '\\'; break;
                case '/': r += '/';  break;
                default:  r += e;    break;
                }
            } else {
                r += c;
            }
        }
        return r;
    }

    bool ParseValue(Scalar& v) {
        Skip();
        if (Eof()) return false;
        const char c = Peek();
        if (c == '"') { v.type = Scalar::Type::String; v.str = ParseString(); return true; }
        if (c == '{' || c == '[') { SkipContainer(); v.type = Scalar::Type::Null; return true; }
        if (c == 't') { if (s_.substr(i_, 4) == "true")  { i_ += 4; v.type = Scalar::Type::Bool; v.boolean = true;  return true; } return false; }
        if (c == 'f') { if (s_.substr(i_, 5) == "false") { i_ += 5; v.type = Scalar::Type::Bool; v.boolean = false; return true; } return false; }
        if (c == 'n') { if (s_.substr(i_, 4) == "null")  { i_ += 4; v.type = Scalar::Type::Null; return true; } return false; }
        if (c == '-' || (c >= '0' && c <= '9')) {
            const std::size_t start = i_;
            if (Peek() == '-') ++i_;
            while (!Eof()) {
                const char d = Peek();
                if ((d >= '0' && d <= '9') || d == '.' || d == 'e' || d == 'E' || d == '+' || d == '-') ++i_;
                else break;
            }
            try { v.num = std::stod(std::string(s_.substr(start, i_ - start))); }
            catch (...) { return false; }
            v.type = Scalar::Type::Number;
            return true;
        }
        return false;
    }

    void SkipContainer() {
        const char open = Peek();
        const char close = open == '{' ? '}' : ']';
        int depth = 0;
        while (!Eof()) {
            const char c = s_[i_++];
            if (c == '"') {
                while (!Eof()) { const char d = s_[i_++]; if (d == '\\' && !Eof()) ++i_; else if (d == '"') break; }
            } else if (c == open) {
                ++depth;
            } else if (c == close) {
                if (--depth == 0) break;
            }
        }
    }
};

} // namespace

bool DemoConfig::Load(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    const std::string text = contents.str();

    std::vector<std::pair<std::string, Scalar>> kv;
    FlatJson parser(text);
    if (!parser.Parse(kv)) {
        return false;
    }

    for (const auto& [key, v] : kv) {
        const auto asFloat = [&](float& dst) { if (v.type == Scalar::Type::Number) dst = static_cast<float>(v.num); };
        const auto asInt   = [&](int& dst)   { if (v.type == Scalar::Type::Number) dst = static_cast<int>(v.num); };
        const auto asBool  = [&](bool& dst)  { if (v.type == Scalar::Type::Bool)   dst = v.boolean; };
        const auto asStr   = [&](std::string& dst) { if (v.type == Scalar::Type::String) dst = v.str; };

        if      (key == "demoMode")         asBool(demoMode);
        else if (key == "fullscreen")       asBool(fullscreen);
        else if (key == "vsync")            asBool(vsync);
        else if (key == "targetFPS")        asInt(targetFPS);
        else if (key == "theme")            asStr(theme);
        else if (key == "audioFile")        asStr(audioFile);
        else if (key == "audioSource")      asStr(audioSource);
        else if (key == "captureDevice")    asInt(captureDevice);
        else if (key == "showDebugOverlay") asBool(showDebugOverlay);
        else if (key == "masterBrightness") asFloat(masterBrightness);
        else if (key == "masterGlow")       asFloat(masterGlow);
        else if (key == "masterExposure")   asFloat(masterExposure);
    }
    return true;
}

bool DemoConfig::Save(const std::string& path) const {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    const auto quote = [](const std::string& s) {
        std::string o = "\"";
        for (const char c : s) { if (c == '"' || c == '\\') o += '\\'; o += c; }
        o += '"';
        return o;
    };
    const auto boolean = [](bool v) { return v ? "true" : "false"; };

    file << "{\n";
    file << "  \"demoMode\": "         << boolean(demoMode)        << ",\n";
    file << "  \"fullscreen\": "       << boolean(fullscreen)      << ",\n";
    file << "  \"vsync\": "            << boolean(vsync)           << ",\n";
    file << "  \"targetFPS\": "        << targetFPS                << ",\n";
    file << "  \"theme\": "            << quote(theme)             << ",\n";
    file << "  \"audioFile\": "        << quote(audioFile)         << ",\n";
    file << "  \"audioSource\": "      << quote(audioSource)       << ",\n";
    file << "  \"captureDevice\": "    << captureDevice            << ",\n";
    file << "  \"showDebugOverlay\": " << boolean(showDebugOverlay) << ",\n";
    file << "  \"masterBrightness\": " << masterBrightness         << ",\n";
    file << "  \"masterGlow\": "       << masterGlow               << ",\n";
    file << "  \"masterExposure\": "   << masterExposure           << "\n";
    file << "}\n";
    return static_cast<bool>(file);
}

} // namespace papagedon::runtime
