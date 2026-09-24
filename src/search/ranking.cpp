#pragma once
#include "../algorithm/models/Movie.h"
#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

// Weighted relevance score for a movie against a query text and an optional tag.
// Weights:
//   title match  : 5.0
//   tag match    : 3.0
//   plot match   : 1.0
//   recency bias : year / 10000.0  (light tie-breaker, never dominates)
struct Ranker {
    static constexpr double W_TITLE = 5.0;
    static constexpr double W_TAG   = 3.0;
    static constexpr double W_PLOT  = 1.0;

    // Returns a relevance score >= 0 for movie m given query text and tag.
    static double score(const Movie &m,
                        const std::string &query_text,
                        const std::string &query_tag) {
        double s = 0.0;

        if (!query_text.empty()) {
            std::string q = to_lower(query_text);
            if (contains(to_lower(m.title), q))
                s += W_TITLE;
            if (contains(to_lower(m.plot), q))
                s += W_PLOT;
        }

        if (!query_tag.empty()) {
            std::string t = to_lower(query_tag);
            // Check both the tags vector and raw genre/origin fields.
            bool tag_hit = std::find(m.tags.begin(), m.tags.end(), t) != m.tags.end();
            if (!tag_hit)
                tag_hit = contains(to_lower(m.genre), t) ||
                          contains(to_lower(m.origin), t) ||
                          contains(to_lower(m.director), t) ||
                          contains(to_lower(m.cast), t);
            if (tag_hit)
                s += W_TAG;
        }

        // Light recency tie-breaker (0 < year/10000 < ~0.3 for movies up to 2025)
        if (m.year > 0)
            s += static_cast<double>(m.year) / 10000.0;

        return s;
    }

    // Sorts candidates by (score DESC, year DESC, id ASC) and returns the
    // sub-page [offset, offset+limit).  limit == 0 means no limit.
    static std::vector<const Movie *>
    rank_and_page(std::vector<const Movie *> candidates,
                  const std::string &query_text,
                  const std::string &query_tag,
                  int offset,
                  int limit) {
        // Compute scores once.
        std::vector<std::pair<double, const Movie *>> scored;
        scored.reserve(candidates.size());
        for (const Movie *m : candidates)
            scored.emplace_back(score(*m, query_text, query_tag), m);

        // Deterministic order: score DESC → year DESC → id ASC
        std::sort(scored.begin(), scored.end(),
                  [](const auto &a, const auto &b) {
                      if (a.first != b.first)
                          return a.first > b.first;
                      if (a.second->year != b.second->year)
                          return a.second->year > b.second->year;
                      return a.second->id < b.second->id;
                  });

        // Page slice.
        std::vector<const Movie *> result;
        int n = static_cast<int>(scored.size());
        int start = std::min(offset, n);
        int end   = (limit > 0) ? std::min(start + limit, n) : n;
        for (int i = start; i < end; ++i)
            result.push_back(scored[i].second);
        return result;
    }

  private:
    static std::string to_lower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    static bool contains(const std::string &haystack, const std::string &needle) {
        return haystack.find(needle) != std::string::npos;
    }
};
