#pragma once
#include "models/Movie.h"
#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_set>
#include <vector>

// Builds and checks tags derived from a movie's metadata fields
// (genre, origin, director, cast). Tags are normalized lowercase tokens.
class Tagger {
  public:
    Tagger() = default;

    // Generates the tag list for a movie and stores it in m.tags.
    // Call this after loading movies but before building indexes.
    void tag(Movie &m) const {
        std::unordered_set<std::string> seen;
        auto add_field = [&](const std::string &field) {
            for (const std::string &tok : tokenize(field)) {
                if (seen.insert(tok).second)
                    m.tags.push_back(tok);
            }
        };

        add_field(m.genre);
        add_field(m.origin);
        add_field(m.director);
        add_field(m.cast);
    }

    // Tags every movie in the collection in-place.
    void tag_all(std::vector<Movie> &movies) const {
        for (Movie &m : movies)
            tag(m);
    }

    // Returns true when the movie carries the given tag (case-insensitive).
    bool has_tag(const Movie &m, const std::string &raw_tag) const {
        std::string t = normalize(raw_tag);
        return std::find(m.tags.begin(), m.tags.end(), t) != m.tags.end();
    }

  private:
    // Converts a string to lowercase.
    std::string normalize(const std::string &s) const {
        std::string out = s;
        std::transform(out.begin(), out.end(), out.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return out;
    }

    // Splits a field on whitespace, commas, and forward-slashes, normalizes
    // each token, and discards empty / very-short results.
    std::vector<std::string> tokenize(const std::string &field) const {
        std::vector<std::string> tokens;
        if (field.empty())
            return tokens;

        std::string buf;
        for (unsigned char c : field) {
            if (c == ',' || c == '/' || c == ' ' || c == '\t') {
                if (!buf.empty()) {
                    std::string tok = normalize(buf);
                    if (tok.size() > 1)
                        tokens.push_back(tok);
                    buf.clear();
                }
            } else {
                buf += static_cast<char>(c);
            }
        }
        if (!buf.empty()) {
            std::string tok = normalize(buf);
            if (tok.size() > 1)
                tokens.push_back(tok);
        }
        return tokens;
    }
};
