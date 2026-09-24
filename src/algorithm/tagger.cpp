#include "models/Movie.h"
#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_set>
#include <vector>

namespace movie_tagger {

static std::string normalize(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

static void add_tokens(const std::string &text, std::unordered_set<std::string> &out) {
    std::string token;
    std::string normalized = normalize(text);

    for (char c : normalized) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '+' || c == '#') {
            token += c;
        } else if (!token.empty()) {
            out.insert(token);
            token.clear();
        }
    }

    if (!token.empty())
        out.insert(token);
}

std::vector<std::string> build_tags(const Movie &movie) {
    std::unordered_set<std::string> unique;
    add_tokens(movie.genre, unique);
    add_tokens(movie.origin, unique);
    add_tokens(movie.director, unique);
    add_tokens(movie.cast, unique);

    std::vector<std::string> tags(unique.begin(), unique.end());
    std::sort(tags.begin(), tags.end());
    return tags;
}

bool has_tag(const Movie &movie, const std::string &tag) {
    std::string normalized_tag = normalize(tag);
    if (normalized_tag.empty())
        return true;

    for (const auto &existing : movie.tags)
        if (normalize(existing) == normalized_tag)
            return true;

    std::string bag = normalize(movie.genre + " " + movie.origin + " " + movie.director + " " + movie.cast);
    return bag.find(normalized_tag) != std::string::npos;
}

} // namespace movie_tagger