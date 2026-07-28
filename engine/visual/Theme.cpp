#include "Theme.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <utility>
#include <vector>

namespace papagedon::visual {
namespace {

// ──────────────────────────────────────────────────────────────────────────────
// Enum ⇄ string tables
// ──────────────────────────────────────────────────────────────────────────────
template <typename E>
struct NameEntry final {
    E value;
    std::string_view name;
};

constexpr std::array kParticleNames{
    NameEntry{ParticleStyle::None,     "None"},
    NameEntry{ParticleStyle::Sparks,   "Sparks"},
    NameEntry{ParticleStyle::Embers,   "Embers"},
    NameEntry{ParticleStyle::Dust,     "Dust"},
    NameEntry{ParticleStyle::Snow,     "Snow"},
    NameEntry{ParticleStyle::Rain,     "Rain"},
    NameEntry{ParticleStyle::Bokeh,    "Bokeh"},
    NameEntry{ParticleStyle::Confetti, "Confetti"},
};

constexpr std::array kMotionNames{
    NameEntry{MotionStyle::Smooth,     "Smooth"},
    NameEntry{MotionStyle::Flowing,    "Flowing"},
    NameEntry{MotionStyle::Pulsing,    "Pulsing"},
    NameEntry{MotionStyle::Aggressive, "Aggressive"},
    NameEntry{MotionStyle::Strobing,   "Strobing"},
    NameEntry{MotionStyle::Hypnotic,   "Hypnotic"},
};

constexpr std::array kGeometryNames{
    NameEntry{GeometryStyle::Organic, "Organic"},
    NameEntry{GeometryStyle::Grid,    "Grid"},
    NameEntry{GeometryStyle::Radial,  "Radial"},
    NameEntry{GeometryStyle::Fractal, "Fractal"},
    NameEntry{GeometryStyle::Tunnel,  "Tunnel"},
    NameEntry{GeometryStyle::Waves,   "Waves"},
};

constexpr std::array kNoiseNames{
    NameEntry{NoiseStyle::None,      "None"},
    NameEntry{NoiseStyle::Film,      "Film"},
    NameEntry{NoiseStyle::Digital,   "Digital"},
    NameEntry{NoiseStyle::Turbulent, "Turbulent"},
    NameEntry{NoiseStyle::Scanline,  "Scanline"},
};

constexpr std::array kTransitionNames{
    NameEntry{TransitionStyle::Cut,      "Cut"},
    NameEntry{TransitionStyle::Fade,     "Fade"},
    NameEntry{TransitionStyle::Dissolve, "Dissolve"},
    NameEntry{TransitionStyle::Wipe,     "Wipe"},
    NameEntry{TransitionStyle::Glitch,   "Glitch"},
};

char Lower(const char c) noexcept {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

bool IEquals(const std::string_view a, const std::string_view b) noexcept {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (Lower(a[i]) != Lower(b[i])) {
            return false;
        }
    }
    return true;
}

template <typename Table, typename E>
std::string_view NameOf(const Table& table, const E value) noexcept {
    for (const auto& entry : table) {
        if (entry.value == value) {
            return entry.name;
        }
    }
    return table.front().name;
}

template <typename Table, typename E>
bool ParseName(const Table& table, const std::string_view text, E& out) noexcept {
    for (const auto& entry : table) {
        if (IEquals(entry.name, text)) {
            out = entry.value;
            return true;
        }
    }
    return false;
}

// ──────────────────────────────────────────────────────────────────────────────
// Hex colours
// ──────────────────────────────────────────────────────────────────────────────
int HexDigit(const char c) noexcept {
    if (c >= '0' && c <= '9') return c - '0';
    const char l = Lower(c);
    if (l >= 'a' && l <= 'f') return l - 'a' + 10;
    return -1;
}

bool ParseHexColor(std::string_view s, ThemeColor& out) noexcept {
    if (!s.empty() && s.front() == '#') {
        s.remove_prefix(1);
    }
    if (s.size() != 6) {
        return false;
    }
    std::array<int, 6> d{};
    for (std::size_t i = 0; i < d.size(); ++i) {
        d[i] = HexDigit(s[i]);
        if (d[i] < 0) {
            return false;
        }
    }
    out = ThemeColor{
        static_cast<float>(d[0] * 16 + d[1]) / 255.0F,
        static_cast<float>(d[2] * 16 + d[3]) / 255.0F,
        static_cast<float>(d[4] * 16 + d[5]) / 255.0F,
    };
    return true;
}

std::string ToHexColor(const ThemeColor& c) {
    const auto q = [](const float x) {
        const float clamped = x < 0.0F ? 0.0F : (x > 1.0F ? 1.0F : x);
        return static_cast<int>(std::lround(clamped * 255.0F));
    };
    std::array<char, 8> buf{};
    std::snprintf(buf.data(), buf.size(), "#%02X%02X%02X", q(c.r), q(c.g), q(c.b));
    return std::string(buf.data());
}

// ──────────────────────────────────────────────────────────────────────────────
// Minimal, self-contained JSON value model + recursive-descent parser
//
// Scoped to the theme schema (objects, arrays, strings, numbers, booleans, null).
// It exists so the visual layer can round-trip themes without pulling in a
// third-party JSON dependency.
// ──────────────────────────────────────────────────────────────────────────────
struct JsonValue final {
    enum class Type { Null, Bool, Number, String, Array, Object };
    Type type = Type::Null;
    bool   boolean = false;
    double number  = 0.0;
    std::string string;
    std::vector<JsonValue> array;
    std::vector<std::pair<std::string, JsonValue>> object;

    [[nodiscard]] const JsonValue* Find(std::string_view key) const noexcept {
        for (const auto& kv : object) {
            if (kv.first == key) {
                return &kv.second;
            }
        }
        return nullptr;
    }
};

class JsonParser final {
public:
    explicit JsonParser(std::string_view src) noexcept : s_{src} {}

    bool Parse(JsonValue& out, std::string& error) {
        SkipWs();
        if (!ParseValue(out)) {
            error = error_.empty() ? "invalid JSON" : error_;
            return false;
        }
        return true;
    }

private:
    std::string_view s_;
    std::size_t pos_ = 0;
    std::string error_;

    [[nodiscard]] bool Eof() const noexcept { return pos_ >= s_.size(); }
    [[nodiscard]] char Peek() const noexcept { return s_[pos_]; }

    void SkipWs() noexcept {
        while (!Eof()) {
            const char c = Peek();
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++pos_;
            } else {
                break;
            }
        }
    }

    bool Fail(std::string msg) {
        if (error_.empty()) {
            error_ = std::move(msg);
        }
        return false;
    }

    bool ParseValue(JsonValue& v) {
        SkipWs();
        if (Eof()) {
            return Fail("unexpected end of input");
        }
        const char c = Peek();
        switch (c) {
        case '{': return ParseObject(v);
        case '[': return ParseArray(v);
        case '"': v.type = JsonValue::Type::String; return ParseString(v.string);
        case 't':
        case 'f': return ParseBool(v);
        case 'n': return ParseNull(v);
        default:
            if (c == '-' || (c >= '0' && c <= '9')) {
                return ParseNumber(v);
            }
            return Fail("unexpected character");
        }
    }

    bool ParseObject(JsonValue& v) {
        v.type = JsonValue::Type::Object;
        ++pos_; // consume '{'
        SkipWs();
        if (!Eof() && Peek() == '}') {
            ++pos_;
            return true;
        }
        while (true) {
            SkipWs();
            if (Eof() || Peek() != '"') {
                return Fail("expected string key");
            }
            std::string key;
            if (!ParseString(key)) {
                return false;
            }
            SkipWs();
            if (Eof() || Peek() != ':') {
                return Fail("expected ':'");
            }
            ++pos_; // consume ':'
            JsonValue child;
            if (!ParseValue(child)) {
                return false;
            }
            v.object.emplace_back(std::move(key), std::move(child));
            SkipWs();
            if (Eof()) {
                return Fail("unterminated object");
            }
            const char c = Peek();
            if (c == ',') { ++pos_; continue; }
            if (c == '}') { ++pos_; return true; }
            return Fail("expected ',' or '}'");
        }
    }

    bool ParseArray(JsonValue& v) {
        v.type = JsonValue::Type::Array;
        ++pos_; // consume '['
        SkipWs();
        if (!Eof() && Peek() == ']') {
            ++pos_;
            return true;
        }
        while (true) {
            JsonValue child;
            if (!ParseValue(child)) {
                return false;
            }
            v.array.push_back(std::move(child));
            SkipWs();
            if (Eof()) {
                return Fail("unterminated array");
            }
            const char c = Peek();
            if (c == ',') { ++pos_; continue; }
            if (c == ']') { ++pos_; return true; }
            return Fail("expected ',' or ']'");
        }
    }

    bool ParseString(std::string& out) {
        ++pos_; // consume opening quote
        std::string result;
        while (!Eof()) {
            const char c = s_[pos_++];
            if (c == '"') {
                out = std::move(result);
                return true;
            }
            if (c == '\\') {
                if (Eof()) {
                    return Fail("unterminated escape");
                }
                const char e = s_[pos_++];
                switch (e) {
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case '/':  result += '/';  break;
                case 'n':  result += '\n'; break;
                case 't':  result += '\t'; break;
                case 'r':  result += '\r'; break;
                case 'b':  result += '\b'; break;
                case 'f':  result += '\f'; break;
                case 'u': {
                    if (pos_ + 4 > s_.size()) {
                        return Fail("bad \\u escape");
                    }
                    int cp = 0;
                    for (int k = 0; k < 4; ++k) {
                        const int h = HexDigit(s_[pos_++]);
                        if (h < 0) {
                            return Fail("bad \\u escape");
                        }
                        cp = cp * 16 + h;
                    }
                    AppendUtf8(result, cp);
                    break;
                }
                default: return Fail("bad escape");
                }
            } else {
                result += c;
            }
        }
        return Fail("unterminated string");
    }

    bool ParseNumber(JsonValue& v) {
        const std::size_t start = pos_;
        if (!Eof() && Peek() == '-') {
            ++pos_;
        }
        while (!Eof()) {
            const char c = Peek();
            if ((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E' ||
                c == '+' || c == '-') {
                ++pos_;
            } else {
                break;
            }
        }
        const std::string token(s_.substr(start, pos_ - start));
        try {
            v.number = std::stod(token);
        } catch (...) {
            return Fail("bad number");
        }
        v.type = JsonValue::Type::Number;
        return true;
    }

    bool ParseBool(JsonValue& v) {
        if (s_.substr(pos_, 4) == "true") {
            pos_ += 4;
            v.type = JsonValue::Type::Bool;
            v.boolean = true;
            return true;
        }
        if (s_.substr(pos_, 5) == "false") {
            pos_ += 5;
            v.type = JsonValue::Type::Bool;
            v.boolean = false;
            return true;
        }
        return Fail("bad literal");
    }

    bool ParseNull(JsonValue& v) {
        if (s_.substr(pos_, 4) == "null") {
            pos_ += 4;
            v.type = JsonValue::Type::Null;
            return true;
        }
        return Fail("bad literal");
    }

    static void AppendUtf8(std::string& s, const int cp) {
        if (cp < 0x80) {
            s += static_cast<char>(cp);
        } else if (cp < 0x800) {
            s += static_cast<char>(0xC0 | (cp >> 6));
            s += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            s += static_cast<char>(0xE0 | (cp >> 12));
            s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            s += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }
};

void ReadColor(const JsonValue& value, ThemeColor& out) {
    if (value.type == JsonValue::Type::String) {
        ParseHexColor(value.string, out); // leaves `out` unchanged on failure
    } else if (value.type == JsonValue::Type::Array && value.array.size() >= 3) {
        const auto num = [](const JsonValue& v) {
            return v.type == JsonValue::Type::Number ? static_cast<float>(v.number) : 0.0F;
        };
        out = ThemeColor{num(value.array[0]), num(value.array[1]), num(value.array[2])};
    }
}

// ── JSON writer helpers ─────────────────────────────────────────────────────
std::string Quote(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 2);
    out += '"';
    for (const char c : s) {
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\t': out += "\\t";  break;
        case '\r': out += "\\r";  break;
        default:   out += c;      break;
        }
    }
    out += '"';
    return out;
}

std::string Num(const float value) {
    std::array<char, 32> buf{};
    std::snprintf(buf.data(), buf.size(), "%g", static_cast<double>(value));
    return std::string(buf.data());
}

} // namespace

// ──────────────────────────────────────────────────────────────────────────────
// Enum ⇄ string — public
// ──────────────────────────────────────────────────────────────────────────────
std::string_view ToString(const ParticleStyle v) noexcept   { return NameOf(kParticleNames, v); }
std::string_view ToString(const MotionStyle v) noexcept     { return NameOf(kMotionNames, v); }
std::string_view ToString(const GeometryStyle v) noexcept   { return NameOf(kGeometryNames, v); }
std::string_view ToString(const NoiseStyle v) noexcept      { return NameOf(kNoiseNames, v); }
std::string_view ToString(const TransitionStyle v) noexcept { return NameOf(kTransitionNames, v); }

bool FromString(const std::string_view s, ParticleStyle& out) noexcept   { return ParseName(kParticleNames, s, out); }
bool FromString(const std::string_view s, MotionStyle& out) noexcept     { return ParseName(kMotionNames, s, out); }
bool FromString(const std::string_view s, GeometryStyle& out) noexcept   { return ParseName(kGeometryNames, s, out); }
bool FromString(const std::string_view s, NoiseStyle& out) noexcept      { return ParseName(kNoiseNames, s, out); }
bool FromString(const std::string_view s, TransitionStyle& out) noexcept { return ParseName(kTransitionNames, s, out); }

// ──────────────────────────────────────────────────────────────────────────────
// Serialization — public
// ──────────────────────────────────────────────────────────────────────────────
std::string ToJson(const Theme& t) {
    std::ostringstream o;
    o << "{\n";
    o << "  \"id\": "   << Quote(t.id)   << ",\n";
    o << "  \"name\": " << Quote(t.name) << ",\n";
    o << "  \"palette\": {\n";
    o << "    \"primary\": \""    << ToHexColor(t.palette.primary)    << "\",\n";
    o << "    \"secondary\": \""  << ToHexColor(t.palette.secondary)  << "\",\n";
    o << "    \"accent\": \""     << ToHexColor(t.palette.accent)     << "\",\n";
    o << "    \"background\": \""  << ToHexColor(t.palette.background) << "\"\n";
    o << "  },\n";
    o << "  \"contrast\": "      << Num(t.contrast)      << ",\n";
    o << "  \"glowStrength\": "  << Num(t.glowStrength)  << ",\n";
    o << "  \"bloomStrength\": " << Num(t.bloomStrength) << ",\n";
    o << "  \"particleStyle\": "   << Quote(ToString(t.particleStyle))   << ",\n";
    o << "  \"motionStyle\": "     << Quote(ToString(t.motionStyle))     << ",\n";
    o << "  \"geometryStyle\": "   << Quote(ToString(t.geometryStyle))   << ",\n";
    o << "  \"noiseStyle\": "      << Quote(ToString(t.noiseStyle))      << ",\n";
    o << "  \"transitionStyle\": " << Quote(ToString(t.transitionStyle));

    if (!t.shaderParameters.empty()) {
        // Sort keys so serialization is deterministic across runs.
        std::vector<std::pair<std::string, float>> params(
            t.shaderParameters.begin(), t.shaderParameters.end());
        std::sort(params.begin(), params.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        o << ",\n  \"shaderParameters\": {\n";
        for (std::size_t i = 0; i < params.size(); ++i) {
            o << "    " << Quote(params[i].first) << ": " << Num(params[i].second)
              << (i + 1 < params.size() ? ",\n" : "\n");
        }
        o << "  }";
    }

    o << "\n}\n";
    return o.str();
}

bool FromJson(const std::string_view json, Theme& out, std::string* error) {
    JsonValue root;
    std::string err;
    JsonParser parser(json);
    if (!parser.Parse(root, err)) {
        if (error != nullptr) {
            *error = err;
        }
        return false;
    }
    if (root.type != JsonValue::Type::Object) {
        if (error != nullptr) {
            *error = "root value is not a JSON object";
        }
        return false;
    }

    Theme theme; // begins at defaults; JSON overrides only what it provides

    const auto readString = [&](const char* key, std::string& dst) {
        if (const JsonValue* v = root.Find(key); v != nullptr &&
            v->type == JsonValue::Type::String) {
            dst = v->string;
        }
    };
    const auto readNumber = [&](const char* key, float& dst) {
        if (const JsonValue* v = root.Find(key); v != nullptr &&
            v->type == JsonValue::Type::Number) {
            dst = static_cast<float>(v->number);
        }
    };
    const auto readEnum = [&](const char* key, auto& dst) {
        if (const JsonValue* v = root.Find(key); v != nullptr &&
            v->type == JsonValue::Type::String) {
            // Unknown values leave `dst` at its default — that is intentional, so
            // the [[nodiscard]] result is deliberately discarded.
            static_cast<void>(FromString(v->string, dst));
        }
    };

    readString("id", theme.id);
    readString("name", theme.name);
    readNumber("contrast", theme.contrast);
    readNumber("glowStrength", theme.glowStrength);
    readNumber("bloomStrength", theme.bloomStrength);

    if (const JsonValue* pal = root.Find("palette");
        pal != nullptr && pal->type == JsonValue::Type::Object) {
        if (const JsonValue* c = pal->Find("primary"))    { ReadColor(*c, theme.palette.primary); }
        if (const JsonValue* c = pal->Find("secondary"))  { ReadColor(*c, theme.palette.secondary); }
        if (const JsonValue* c = pal->Find("accent"))     { ReadColor(*c, theme.palette.accent); }
        if (const JsonValue* c = pal->Find("background")) { ReadColor(*c, theme.palette.background); }
    }

    readEnum("particleStyle", theme.particleStyle);
    readEnum("motionStyle", theme.motionStyle);
    readEnum("geometryStyle", theme.geometryStyle);
    readEnum("noiseStyle", theme.noiseStyle);
    readEnum("transitionStyle", theme.transitionStyle);

    if (const JsonValue* sp = root.Find("shaderParameters");
        sp != nullptr && sp->type == JsonValue::Type::Object) {
        for (const auto& kv : sp->object) {
            if (kv.second.type == JsonValue::Type::Number) {
                theme.shaderParameters[kv.first] = static_cast<float>(kv.second.number);
            }
        }
    }

    out = std::move(theme);
    return true;
}

} // namespace papagedon::visual
