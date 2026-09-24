#include "../algorithm/models/Movie.h"
#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

namespace movie_ranking {

static std::string normalize(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

static int count_occurrences(const std::string &text, const std::string &query) {
    if (query.empty())
        return 0;

    std::string normalized_text = normalize(text);
    std::string normalized_query = normalize(query);

    int count = 0;
    size_t pos = 0;
    while ((pos = normalized_text.find(normalized_query, pos)) != std::string::npos) {
        ++count;
        pos += normalized_query.size();
    }
    return count;
}

static int count_tag_hits(const Movie &movie, const std::string &query) {
    int hits = 0;
    for (const auto &tag : movie.tags)
        hits += count_occurrences(tag, query);
    return hits;
}

std::vector<const Movie *> rank(const std::vector<const Movie *> &candidates,
                                const std::string &query, size_t offset = 0, size_t limit = 5) {
    std::vector<std::pair<double, const Movie *>> scored;
    scored.reserve(candidates.size());

    for (const Movie *movie : candidates) {
        if (!movie)
            continue;

        double score = 0.0;
        score += 5.0 * count_occurrences(movie->title, query);
        score += 1.0 * count_occurrences(movie->plot, query);
        score += 3.0 * count_tag_hits(*movie, query);
        score += static_cast<double>(movie->year) / 10000.0;
        scored.push_back({score, movie});
    }

    std::sort(scored.begin(), scored.end(),
              [](const auto &a, const auto &b) {
                  if (a.first != b.first)
                      return a.first > b.first;
                  if (a.second->year != b.second->year)
                      return a.second->year > b.second->year;
                  return a.second->id < b.second->id;
              });

    std::vector<const Movie *> ranked;
    if (offset >= scored.size() || limit == 0)
        return ranked;

    size_t end = std::min(scored.size(), offset + limit);
    ranked.reserve(end - offset);
    for (size_t i = offset; i < end; ++i)
        ranked.push_back(scored[i].second);

    return ranked;
}

} // namespace movie_ranking