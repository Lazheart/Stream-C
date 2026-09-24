// wasm_bridge.cpp
// Single compilation unit that bridges the C++ search engine to WebAssembly.
// Compiled with emcc; all exported functions are callable from JavaScript
// via Module.ccall / Module.cwrap.

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define WASM_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define WASM_EXPORT
#endif

// ── Project includes (header-only / single-TU style) ────────────────────────
#include "src/algorithm/loaders/CsvMovieLoader.cpp"
#include "src/search/searchEngine.cpp"
#include "src/helpers/userDataManager.cpp"

// ── Global state ────────────────────────────────────────────────────────────
static std::vector<Movie>  g_movies;
static SearchEngine        g_engine;
static bool                g_ready = false;
static std::string         g_json_buffer;   // reusable return buffer

// ── Helpers ─────────────────────────────────────────────────────────────────

// Minimal JSON escaping for a std::string value.
static std::string json_escape(const std::string &s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x",
                             static_cast<unsigned char>(c));
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

// Serialise a Movie to a JSON object string.
static std::string movie_to_json(const Movie &m) {
    std::ostringstream o;
    o << "{";
    o << "\"id\":"       << m.id << ",";
    o << "\"year\":"     << m.year << ",";
    o << "\"title\":\""    << json_escape(m.title) << "\",";
    o << "\"origin\":\""   << json_escape(m.origin) << "\",";
    o << "\"director\":\"" << json_escape(m.director) << "\",";
    o << "\"cast\":\""     << json_escape(m.cast) << "\",";
    o << "\"genre\":\""    << json_escape(m.genre) << "\",";
    o << "\"wiki_url\":\"" << json_escape(m.wiki_url) << "\",";
    o << "\"plot\":\""     << json_escape(m.plot) << "\",";
    o << "\"tags\":[";
    for (size_t i = 0; i < m.tags.size(); ++i) {
        if (i) o << ",";
        o << "\"" << json_escape(m.tags[i]) << "\"";
    }
    o << "]}";
    return o.str();
}

// ── Exported functions ──────────────────────────────────────────────────────
extern "C" {

// Load CSV data from a string (fetched by JavaScript) and build all indexes.
// Returns the number of movies loaded, or -1 on error.
WASM_EXPORT int wasm_load_csv(const char *csv_text) {
    if (!csv_text) return -1;

    try {
        std::vector<Movie> movies;
        const char* p = csv_text;
        bool first_line = true;
        int next_id = 1;
        std::string line;

        while (*p != '\0') {
            const char* end = strchr(p, '\n');
            if (!end) end = p + strlen(p);

            line.assign(p, end - p);
            if (*end == '\n') p = end + 1;
            else p = end;

            if (first_line) {
                first_line = false;
                continue;
            }

            if (trim(line).empty())
                continue;

            std::vector<std::string> fields = parse_csv_row(line);
            while (fields.size() < 8) fields.push_back("");

            Movie m;
            m.id       = next_id++;
            m.year     = parse_year(fields[0]);
            m.title    = trim(fields[1]);
            m.origin   = trim(fields[2]);
            m.director = trim(fields[3]);
            m.cast     = trim(fields[4]);
            m.genre    = trim(fields[5]);
            m.wiki_url = trim(fields[6]);
            m.plot     = trim(fields[7]);

            if (m.title.empty()) continue;

            movies.push_back(std::move(m));
        }

        g_movies = std::move(movies);
    } catch (...) {
        return -1;
    }

    g_engine.build(g_movies);
    g_ready = true;

    return static_cast<int>(g_movies.size());
}

// Returns true if the engine is ready (data loaded and indexes built).
WASM_EXPORT bool wasm_is_ready() {
    return g_ready;
}

// Returns the total number of loaded movies.
WASM_EXPORT int wasm_movie_count() {
    return static_cast<int>(g_movies.size());
}

// Search the engine. Returns a JSON array of movie objects.
// Parameters mirror SearchQuery fields.
WASM_EXPORT const char *wasm_search(const char *text,
                                     const char *genre,
                                     const char *tag,
                                     int year_from,
                                     int year_to,
                                     int offset,
                                     int limit) {
    if (!g_ready) {
        g_json_buffer = "[]";
        return g_json_buffer.c_str();
    }

    SearchQuery q;
    if (text)  q.text  = text;
    if (genre) q.genre = genre;
    if (tag)   q.tag   = tag;
    if (year_from > 0) q.year_from = year_from;
    if (year_to   > 0) q.year_to   = year_to;
    q.offset = offset;
    q.limit  = (limit > 0) ? limit : 20;

    auto results = g_engine.search(q);

    std::ostringstream out;
    out << "[";
    for (size_t i = 0; i < results.size(); ++i) {
        if (i) out << ",";
        out << movie_to_json(*results[i]);
    }
    out << "]";

    g_json_buffer = out.str();
    return g_json_buffer.c_str();
}

// Get a movie by id. Returns JSON object or "null".
WASM_EXPORT const char *wasm_get_movie(int id) {
    if (!g_ready) {
        g_json_buffer = "null";
        return g_json_buffer.c_str();
    }
    // Linear scan (hash_index is private inside SearchEngine)
    for (const Movie &m : g_movies) {
        if (m.id == id) {
            g_json_buffer = movie_to_json(m);
            return g_json_buffer.c_str();
        }
    }
    g_json_buffer = "null";
    return g_json_buffer.c_str();
}

// Get all unique genres as a JSON array of strings.
WASM_EXPORT const char *wasm_get_genres() {
    std::vector<std::string> genres;
    std::unordered_set<std::string> seen;
    for (const Movie &m : g_movies) {
        std::string g = m.genre;
        // normalize
        std::transform(g.begin(), g.end(), g.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (!g.empty() && g != "unknown" && seen.insert(g).second)
            genres.push_back(m.genre); // keep original casing
    }
    std::sort(genres.begin(), genres.end());

    std::ostringstream out;
    out << "[";
    for (size_t i = 0; i < genres.size(); ++i) {
        if (i) out << ",";
        out << "\"" << json_escape(genres[i]) << "\"";
    }
    out << "]";

    g_json_buffer = out.str();
    return g_json_buffer.c_str();
}

// Get featured / random movies for the home page.
// Returns `count` movies starting from a pseudo-random offset.
WASM_EXPORT const char *wasm_get_featured(int count) {
    if (!g_ready || g_movies.empty()) {
        g_json_buffer = "[]";
        return g_json_buffer.c_str();
    }

    if (count <= 0) count = 12;
    if (count > static_cast<int>(g_movies.size()))
        count = static_cast<int>(g_movies.size());

    // Simple deterministic "featured" selection: pick movies spread across the catalog
    std::ostringstream out;
    out << "[";
    int step = std::max(1, static_cast<int>(g_movies.size()) / count);
    int emitted = 0;
    for (int i = 0; emitted < count && i < static_cast<int>(g_movies.size()); i += step) {
        if (emitted) out << ",";
        out << movie_to_json(g_movies[i]);
        ++emitted;
    }
    out << "]";

    g_json_buffer = out.str();
    return g_json_buffer.c_str();
}

// Get movies by genre for browsing. Returns JSON array.
WASM_EXPORT const char *wasm_browse_genre(const char *genre, int offset, int limit) {
    if (!g_ready || !genre) {
        g_json_buffer = "[]";
        return g_json_buffer.c_str();
    }

    SearchQuery q;
    q.genre  = genre;
    q.offset = offset;
    q.limit  = (limit > 0) ? limit : 20;

    auto results = g_engine.search(q);

    std::ostringstream out;
    out << "[";
    for (size_t i = 0; i < results.size(); ++i) {
        if (i) out << ",";
        out << movie_to_json(*results[i]);
    }
    out << "]";

    g_json_buffer = out.str();
    return g_json_buffer.c_str();
}

} // extern "C"
