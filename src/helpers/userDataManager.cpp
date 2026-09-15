#ifndef USER_DATA_MANAGER_CPP
#define USER_DATA_MANAGER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <cctype>
#include <utility>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define WASM_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define WASM_EXPORT
#endif
namespace SimpleJson {

enum class Type { Null, Boolean, Number, String, Array, Object };

struct Value {
    Type type = Type::Null;
    bool boolValue = false;
    double numberValue = 0.0;
    std::string stringValue;
    std::vector<Value> arrayValue;
    std::vector<std::pair<std::string, Value>> objectValue;

    Value() : type(Type::Null) {}
    Value(bool b) : type(Type::Boolean), boolValue(b) {}
    Value(int n) : type(Type::Number), numberValue(static_cast<double>(n)) {}
    Value(long n) : type(Type::Number), numberValue(static_cast<double>(n)) {}
    Value(long long n) : type(Type::Number), numberValue(static_cast<double>(n)) {}
    Value(size_t n) : type(Type::Number), numberValue(static_cast<double>(n)) {}
    Value(double n) : type(Type::Number), numberValue(n) {}
    Value(const char* s) : type(Type::String), stringValue(s ? s : "") {}
    Value(const std::string& s) : type(Type::String), stringValue(s) {}
    Value(std::string&& s) : type(Type::String), stringValue(std::move(s)) {}
    Value(Type t) : type(t) {}

    static Value makeObject() {
        Value v;
        v.type = Type::Object;
        return v;
    }

    static Value makeArray() {
        Value v;
        v.type = Type::Array;
        return v;
    }

    bool isNull() const { return type == Type::Null; }
    bool isBool() const { return type == Type::Boolean; }
    bool isNumber() const { return type == Type::Number; }
    bool isString() const { return type == Type::String; }
    bool isArray() const { return type == Type::Array; }
    bool isObject() const { return type == Type::Object; }

    const Value* get(const std::string& key) const {
        if (type != Type::Object) return nullptr;
        for (const auto& kv : objectValue) {
            if (kv.first == key) return &kv.second;
        }
        return nullptr;
    }

    Value* get(const std::string& key) {
        if (type != Type::Object) return nullptr;
        for (auto& kv : objectValue) {
            if (kv.first == key) return &kv.second;
        }
        return nullptr;
    }

    void set(const std::string& key, Value val) {
        if (type != Type::Object) {
            type = Type::Object;
            objectValue.clear();
        }
        for (auto& kv : objectValue) {
            if (kv.first == key) {
                kv.second = std::move(val);
                return;
            }
        }
        objectValue.emplace_back(key, std::move(val));
    }

    void push_back(Value val) {
        if (type != Type::Array) {
            type = Type::Array;
            arrayValue.clear();
        }
        arrayValue.push_back(std::move(val));
    }

    std::string asString(const std::string& def = "") const {
        return (type == Type::String) ? stringValue : def;
    }

    int asInt(int def = 0) const {
        return (type == Type::Number) ? static_cast<int>(numberValue) : def;
    }

    double asDouble(double def = 0.0) const {
        return (type == Type::Number) ? numberValue : def;
    }

    bool asBool(bool def = false) const {
        return (type == Type::Boolean) ? boolValue : def;
    }

    static std::string escapeString(const std::string& str) {
        std::string out = "\"";
        for (char c : str) {
            switch (c) {
                case '\"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        char buf[7];
                        snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                        out += buf;
                    } else {
                        out += c;
                    }
                    break;
            }
        }
        out += "\"";
        return out;
    }

    std::string serialize(int indent = 2, int currentIndent = 0) const {
        std::string indentStr(currentIndent, ' ');
        std::string nextIndentStr(currentIndent + indent, ' ');

        switch (type) {
            case Type::Null:
                return "null";
            case Type::Boolean:
                return boolValue ? "true" : "false";
            case Type::Number: {
                if (numberValue == static_cast<long long>(numberValue)) {
                    return std::to_string(static_cast<long long>(numberValue));
                }
                std::ostringstream ss;
                ss << std::setprecision(10) << numberValue;
                return ss.str();
            }
            case Type::String:
                return escapeString(stringValue);
            case Type::Array: {
                if (arrayValue.empty()) return "[]";
                std::string res = "[\n";
                for (size_t i = 0; i < arrayValue.size(); ++i) {
                    res += nextIndentStr + arrayValue[i].serialize(indent, currentIndent + indent);
                    if (i + 1 < arrayValue.size()) res += ",";
                    res += "\n";
                }
                res += indentStr + "]";
                return res;
            }
            case Type::Object: {
                if (objectValue.empty()) return "{}";
                std::string res = "{\n";
                for (size_t i = 0; i < objectValue.size(); ++i) {
                    res += nextIndentStr + escapeString(objectValue[i].first) + ": " +
                           objectValue[i].second.serialize(indent, currentIndent + indent);
                    if (i + 1 < objectValue.size()) res += ",";
                    res += "\n";
                }
                res += indentStr + "}";
                return res;
            }
        }
        return "null";
    }
};

class Parser {
    const std::string& src;
    size_t pos = 0;

    void skipWhitespace() {
        while (pos < src.size()) {
            char c = src[pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                pos++;
            } else if (c == '/' && pos + 1 < src.size() && src[pos + 1] == '/') {
                pos += 2;
                while (pos < src.size() && src[pos] != '\n') pos++;
            } else {
                break;
            }
        }
    }

    char peek() {
        skipWhitespace();
        return (pos < src.size()) ? src[pos] : '\0';
    }

    char get() {
        skipWhitespace();
        return (pos < src.size()) ? src[pos++] : '\0';
    }

    bool parseString(std::string& out) {
        if (get() != '\"') return false;
        out.clear();
        while (pos < src.size()) {
            char c = src[pos++];
            if (c == '\"') {
                return true;
            } else if (c == '\\') {
                if (pos >= src.size()) return false;
                char esc = src[pos++];
                switch (esc) {
                    case '\"': out += '\"'; break;
                    case '\\': out += '\\'; break;
                    case '/':  out += '/'; break;
                    case 'b':  out += '\b'; break;
                    case 'f':  out += '\f'; break;
                    case 'n':  out += '\n'; break;
                    case 'r':  out += '\r'; break;
                    case 't':  out += '\t'; break;
                    case 'u': {
                        if (pos + 4 > src.size()) return false;
                        std::string hex = src.substr(pos, 4);
                        pos += 4;
                        unsigned int codepoint = 0;
                        std::stringstream ss;
                        ss << std::hex << hex;
                        ss >> codepoint;
                        if (codepoint < 0x80) {
                            out += static_cast<char>(codepoint);
                        } else if (codepoint < 0x800) {
                            out += static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
                            out += static_cast<char>(0x80 | (codepoint & 0x3F));
                        } else {
                            out += static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
                            out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                            out += static_cast<char>(0x80 | (codepoint & 0x3F));
                        }
                        break;
                    }
                    default: out += esc; break;
                }
            } else {
                out += c;
            }
        }
        return false;
    }

    bool parseNumber(double& out) {
        skipWhitespace();
        size_t start = pos;
        if (pos < src.size() && (src[pos] == '-' || src[pos] == '+')) pos++;
        while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) pos++;
        if (pos < src.size() && src[pos] == '.') {
            pos++;
            while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) pos++;
        }
        if (pos < src.size() && (src[pos] == 'e' || src[pos] == 'E')) {
            pos++;
            if (pos < src.size() && (src[pos] == '-' || src[pos] == '+')) pos++;
            while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) pos++;
        }
        if (pos == start) return false;
        try {
            out = std::stod(src.substr(start, pos - start));
            return true;
        } catch (...) {
            return false;
        }
    }

    bool parseValue(Value& out) {
        skipWhitespace();
        char c = peek();
        if (c == '\"') {
            std::string s;
            if (!parseString(s)) return false;
            out = Value(std::move(s));
            return true;
        } else if (c == '{') {
            return parseObject(out);
        } else if (c == '[') {
            return parseArray(out);
        } else if (c == 't' || c == 'f') {
            if (src.compare(pos, 4, "true") == 0) {
                pos += 4;
                out = Value(true);
                return true;
            } else if (src.compare(pos, 5, "false") == 0) {
                pos += 5;
                out = Value(false);
                return true;
            }
            return false;
        } else if (c == 'n') {
            if (src.compare(pos, 4, "null") == 0) {
                pos += 4;
                out = Value();
                return true;
            }
            return false;
        } else if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            double num = 0;
            if (!parseNumber(num)) return false;
            out = Value(num);
            return true;
        }
        return false;
    }

    bool parseArray(Value& out) {
        if (get() != '[') return false;
        out = Value::makeArray();
        skipWhitespace();
        if (peek() == ']') {
            get();
            return true;
        }
        while (pos < src.size()) {
            Value elem;
            if (!parseValue(elem)) return false;
            out.push_back(std::move(elem));
            skipWhitespace();
            char c = get();
            if (c == ']') return true;
            if (c != ',') return false;
        }
        return false;
    }

    bool parseObject(Value& out) {
        if (get() != '{') return false;
        out = Value::makeObject();
        skipWhitespace();
        if (peek() == '}') {
            get();
            return true;
        }
        while (pos < src.size()) {
            std::string key;
            if (!parseString(key)) return false;
            skipWhitespace();
            if (get() != ':') return false;
            Value val;
            if (!parseValue(val)) return false;
            out.set(key, std::move(val));
            skipWhitespace();
            char c = get();
            if (c == '}') return true;
            if (c != ',') return false;
        }
        return false;
    }

public:
    Parser(const std::string& input) : src(input), pos(0) {}

    bool parse(Value& root) {
        skipWhitespace();
        if (!parseValue(root)) return false;
        skipWhitespace();
        return true;
    }
};

inline bool parse(const std::string& json, Value& root) {
    Parser p(json);
    return p.parse(root);
}

} // namespace SimpleJson

struct MovieData {
    std::string id;
    std::string title;
    int releaseYear = 0;
    std::string origin;
    std::string director;
    std::string cast;
    std::string genre;
    std::string wikiPage;
    std::string plot;
    std::string addedAt; // YYYY-MM-DD HH:MM:SS

    MovieData() = default;

    MovieData(std::string pTitle, int pYear = 0, std::string pGenre = "", std::string pDirector = "", std::string pPlot = "")
        : id(pTitle), title(std::move(pTitle)), releaseYear(pYear),
          director(std::move(pDirector)), genre(std::move(pGenre)), plot(std::move(pPlot)) {}

    SimpleJson::Value toJsonValue() const {
        SimpleJson::Value obj = SimpleJson::Value::makeObject();
        obj.set("id", id.empty() ? title : id);
        obj.set("title", title);
        obj.set("releaseYear", releaseYear);
        obj.set("origin", origin);
        obj.set("director", director);
        obj.set("cast", cast);
        obj.set("genre", genre);
        obj.set("wikiPage", wikiPage);
        obj.set("plot", plot);
        obj.set("addedAt", addedAt);
        return obj;
    }

    static MovieData fromJsonValue(const SimpleJson::Value& val) {
        MovieData m;
        if (!val.isObject()) return m;
        if (auto p = val.get("id")) m.id = p->asString();
        if (auto p = val.get("title")) m.title = p->asString();
        if (auto p = val.get("releaseYear")) m.releaseYear = p->asInt();
        if (auto p = val.get("origin")) m.origin = p->asString();
        if (auto p = val.get("director")) m.director = p->asString();
        if (auto p = val.get("cast")) m.cast = p->asString();
        if (auto p = val.get("genre")) m.genre = p->asString();
        if (auto p = val.get("wikiPage")) m.wikiPage = p->asString();
        if (auto p = val.get("plot")) m.plot = p->asString();
        if (auto p = val.get("addedAt")) m.addedAt = p->asString();

        if (m.id.empty()) m.id = m.title;
        return m;
    }
};

class UserDataManager {
private:
    std::vector<MovieData> watchLaterMovies;
    std::vector<MovieData> likedMovies;
    std::string defaultStoragePath;

    static std::string toLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        return s;
    }

    static std::string getCurrentTimestamp() {
        std::time_t now = std::time(nullptr);
        std::tm tm_buf{};
#if defined(_WIN32)
        localtime_s(&tm_buf, &now);
#else
        localtime_r(&now, &tm_buf);
#endif
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
        return std::string(buf);
    }

    auto findInList(std::vector<MovieData>& list, const std::string& identifier) {
        std::string target = toLower(identifier);
        return std::find_if(list.begin(), list.end(), [&](const MovieData& m) {
            return toLower(m.id) == target || toLower(m.title) == target;
        });
    }

    auto findInList(const std::vector<MovieData>& list, const std::string& identifier) const {
        std::string target = toLower(identifier);
        return std::find_if(list.begin(), list.end(), [&](const MovieData& m) {
            return toLower(m.id) == target || toLower(m.title) == target;
        });
    }

public:
    explicit UserDataManager(std::string storagePath = "userData.json", bool autoLoad = false)
        : defaultStoragePath(std::move(storagePath)) {
        if (autoLoad) {
            loadFromFile();
        }
    }

    // Instancia global/singleton para llamadas desde WebAssembly
    static UserDataManager& getInstance() {
        static UserDataManager instance("userData.json", false);
        return instance;
    }

    // --- Métodos para "Ver más tarde" ---

    bool addWatchLater(MovieData movie) {
        if (movie.title.empty() && movie.id.empty()) return false;
        std::string searchKey = movie.id.empty() ? movie.title : movie.id;
        if (hasWatchLater(searchKey)) {
            return false;
        }
        if (movie.id.empty()) movie.id = movie.title;
        if (movie.addedAt.empty()) movie.addedAt = getCurrentTimestamp();
        watchLaterMovies.push_back(std::move(movie));
        return true;
    }

    bool removeWatchLater(const std::string& identifier) {
        auto it = findInList(watchLaterMovies, identifier);
        if (it != watchLaterMovies.end()) {
            watchLaterMovies.erase(it);
            return true;
        }
        return false;
    }

    bool hasWatchLater(const std::string& identifier) const {
        return findInList(watchLaterMovies, identifier) != watchLaterMovies.end();
    }

    bool toggleWatchLater(const MovieData& movie) {
        std::string key = movie.id.empty() ? movie.title : movie.id;
        if (hasWatchLater(key)) {
            removeWatchLater(key);
            return false;
        } else {
            addWatchLater(movie);
            return true;
        }
    }

    const std::vector<MovieData>& getWatchLater() const {
        return watchLaterMovies;
    }

    size_t getWatchLaterCount() const {
        return watchLaterMovies.size();
    }

    void clearWatchLater() {
        watchLaterMovies.clear();
    }

    // --- Métodos para "Likes" ---

    bool addLiked(MovieData movie) {
        if (movie.title.empty() && movie.id.empty()) return false;
        std::string searchKey = movie.id.empty() ? movie.title : movie.id;
        if (hasLiked(searchKey)) {
            return false;
        }
        if (movie.id.empty()) movie.id = movie.title;
        if (movie.addedAt.empty()) movie.addedAt = getCurrentTimestamp();
        likedMovies.push_back(std::move(movie));
        return true;
    }

    bool removeLiked(const std::string& identifier) {
        auto it = findInList(likedMovies, identifier);
        if (it != likedMovies.end()) {
            likedMovies.erase(it);
            return true;
        }
        return false;
    }

    bool hasLiked(const std::string& identifier) const {
        return findInList(likedMovies, identifier) != likedMovies.end();
    }

    bool toggleLiked(const MovieData& movie) {
        std::string key = movie.id.empty() ? movie.title : movie.id;
        if (hasLiked(key)) {
            removeLiked(key);
            return false;
        } else {
            addLiked(movie);
            return true;
        }
    }

    const std::vector<MovieData>& getLiked() const {
        return likedMovies;
    }

    size_t getLikedCount() const {
        return likedMovies.size();
    }

    void clearLiked() {
        likedMovies.clear();
    }

    void clearAll() {
        clearWatchLater();
        clearLiked();
    }

    // --- Visualización al Iniciar el Programa ---

    void displayStartupSummary(std::ostream& out = std::cout) const {
        out << "\n";
        out << "================================================================================\n";
        out << "                 STREAM-C : RESUMEN DE SESION DEL USUARIO                      \n";
        out << "================================================================================\n";

        // 1. Ver más tarde
        out << "\n[ VER MAS TARDE ] (Total: " << watchLaterMovies.size() << " pelicula(s))\n";
        out << "--------------------------------------------------------------------------------\n";
        if (watchLaterMovies.empty()) {
            out << "  (No hay peliculas anadidas a 'Ver mas tarde')\n";
        } else {
            for (size_t i = 0; i < watchLaterMovies.size(); ++i) {
                const auto& m = watchLaterMovies[i];
                out << "  " << (i + 1) << ". " << m.title;
                if (m.releaseYear > 0) out << " (" << m.releaseYear << ")";
                if (!m.genre.empty() && m.genre != "unknown") out << " | Genero: " << m.genre;
                if (!m.director.empty() && m.director != "Unknown") out << " | Director: " << m.director;
                out << "\n";

                if (!m.plot.empty()) {
                    std::string preview = m.plot.substr(0, 110);
                    if (m.plot.length() > 110) preview += "...";
                    out << "     Sinopsis: " << preview << "\n";
                }
                if (!m.addedAt.empty()) {
                    out << "     Agregada el: " << m.addedAt << "\n";
                }
                out << "\n";
            }
        }

        // 2. Películas con Like
        out << "--------------------------------------------------------------------------------\n";
        out << "[ PELICULAS CON LIKE ] (Total: " << likedMovies.size() << " pelicula(s))\n";
        out << "--------------------------------------------------------------------------------\n";
        if (likedMovies.empty()) {
            out << "  (No has dado 'Like' a ninguna pelicula todavia)\n";
        } else {
            for (size_t i = 0; i < likedMovies.size(); ++i) {
                const auto& m = likedMovies[i];
                out << "  " << (i + 1) << ". [LIKE] " << m.title;
                if (m.releaseYear > 0) out << " (" << m.releaseYear << ")";
                if (!m.genre.empty() && m.genre != "unknown") out << " | Genero: " << m.genre;
                if (!m.director.empty() && m.director != "Unknown") out << " | Director: " << m.director;
                out << "\n";

                if (!m.plot.empty()) {
                    std::string preview = m.plot.substr(0, 110);
                    if (m.plot.length() > 110) preview += "...";
                    out << "     Sinopsis: " << preview << "\n";
                }
                if (!m.addedAt.empty()) {
                    out << "     Guardado el: " << m.addedAt << "\n";
                }
                out << "\n";
            }
        }

        out << "================================================================================\n\n";
    }

    // --- Serialización y Deserialización JSON ---

    std::string toJson(int indent = 2) const {
        SimpleJson::Value root = SimpleJson::Value::makeObject();
        root.set("updatedAt", getCurrentTimestamp());
        root.set("totalWatchLater", watchLaterMovies.size());
        root.set("totalLiked", likedMovies.size());

        // Array watchLater
        SimpleJson::Value wlArray = SimpleJson::Value::makeArray();
        for (const auto& m : watchLaterMovies) {
            wlArray.push_back(m.toJsonValue());
        }
        root.set("watchLater", std::move(wlArray));

        // Array liked
        SimpleJson::Value lkArray = SimpleJson::Value::makeArray();
        for (const auto& m : likedMovies) {
            lkArray.push_back(m.toJsonValue());
        }
        root.set("liked", std::move(lkArray));

        return root.serialize(indent);
    }

    bool loadFromJson(const std::string& jsonString) {
        SimpleJson::Value root;
        if (!SimpleJson::parse(jsonString, root)) {
            return false;
        }
        if (!root.isObject()) {
            return false;
        }

        std::vector<MovieData> newWatchLater;
        if (const auto* pWl = root.get("watchLater")) {
            if (pWl->isArray()) {
                for (const auto& item : pWl->arrayValue) {
                    newWatchLater.push_back(MovieData::fromJsonValue(item));
                }
            }
        }

        std::vector<MovieData> newLiked;
        if (const auto* pLk = root.get("liked")) {
            if (pLk->isArray()) {
                for (const auto& item : pLk->arrayValue) {
                    newLiked.push_back(MovieData::fromJsonValue(item));
                }
            }
        }

        watchLaterMovies = std::move(newWatchLater);
        likedMovies = std::move(newLiked);
        return true;
    }

    bool saveToFile(const std::string& customPath = "") const {
        const std::string& targetPath = customPath.empty() ? defaultStoragePath : customPath;
        std::ofstream file(targetPath);
        if (!file.is_open()) {
            return false;
        }
        file << toJson(2) << "\n";
        return true;
    }

    bool loadFromFile(const std::string& customPath = "") {
        const std::string& targetPath = customPath.empty() ? defaultStoragePath : customPath;
        std::ifstream file(targetPath);
        if (!file.is_open()) {
            return false;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return loadFromJson(buffer.str());
    }
};

// ============================================================================
// Funciones C / WebAssembly Exportables
// Permite llamar directamente desde JavaScript / WebAssembly
// ============================================================================
static std::string g_wasm_json_buffer;

extern "C" {

// Retorna el JSON completo en formato string de las listas actuales
WASM_EXPORT const char* wasm_get_user_data_json() {
    g_wasm_json_buffer = UserDataManager::getInstance().toJson(2);
    return g_wasm_json_buffer.c_str();
}

// Carga el estado de las listas a partir de una cadena JSON proporcionada por el frontend
WASM_EXPORT bool wasm_load_user_data_json(const char* json_str) {
    if (!json_str) return false;
    return UserDataManager::getInstance().loadFromJson(std::string(json_str));
}

// Agrega una película a Ver más tarde
WASM_EXPORT bool wasm_add_watch_later(const char* title, int year, const char* genre,
                                      const char* director, const char* plot, const char* wikiPage) {
    if (!title) return false;
    MovieData m(title, year, genre ? genre : "", director ? director : "", plot ? plot : "");
    if (wikiPage) m.wikiPage = wikiPage;
    return UserDataManager::getInstance().addWatchLater(std::move(m));
}

// Elimina una película de Ver más tarde
WASM_EXPORT bool wasm_remove_watch_later(const char* identifier) {
    if (!identifier) return false;
    return UserDataManager::getInstance().removeWatchLater(identifier);
}

// Verifica si una película está en Ver más tarde
WASM_EXPORT bool wasm_has_watch_later(const char* identifier) {
    if (!identifier) return false;
    return UserDataManager::getInstance().hasWatchLater(identifier);
}

// Alterna (toggle) el estado en Ver más tarde
WASM_EXPORT bool wasm_toggle_watch_later(const char* title, int year, const char* genre,
                                         const char* director, const char* plot, const char* wikiPage) {
    if (!title) return false;
    MovieData m(title, year, genre ? genre : "", director ? director : "", plot ? plot : "");
    if (wikiPage) m.wikiPage = wikiPage;
    return UserDataManager::getInstance().toggleWatchLater(m);
}

// Agrega una película a Likes
WASM_EXPORT bool wasm_add_liked(const char* title, int year, const char* genre,
                                const char* director, const char* plot, const char* wikiPage) {
    if (!title) return false;
    MovieData m(title, year, genre ? genre : "", director ? director : "", plot ? plot : "");
    if (wikiPage) m.wikiPage = wikiPage;
    return UserDataManager::getInstance().addLiked(std::move(m));
}

// Elimina una película de Likes
WASM_EXPORT bool wasm_remove_liked(const char* identifier) {
    if (!identifier) return false;
    return UserDataManager::getInstance().removeLiked(identifier);
}

// Verifica si una película tiene Like
WASM_EXPORT bool wasm_has_liked(const char* identifier) {
    if (!identifier) return false;
    return UserDataManager::getInstance().hasLiked(identifier);
}

// Alterna (toggle) el estado de Like
WASM_EXPORT bool wasm_toggle_liked(const char* title, int year, const char* genre,
                                   const char* director, const char* plot, const char* wikiPage) {
    if (!title) return false;
    MovieData m(title, year, genre ? genre : "", director ? director : "", plot ? plot : "");
    if (wikiPage) m.wikiPage = wikiPage;
    return UserDataManager::getInstance().toggleLiked(m);
}

// Muestra el resumen por consola
WASM_EXPORT void wasm_display_startup_summary() {
    UserDataManager::getInstance().displayStartupSummary();
}

// Limpia todas las listas
WASM_EXPORT void wasm_clear_user_data() {
    UserDataManager::getInstance().clearAll();
}

} // extern "C"

#endif // USER_DATA_MANAGER_CPP
