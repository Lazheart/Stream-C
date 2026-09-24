#include "../models/Movie.h"
#include <algorithm>
#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>

// Suffix array O(nlogn) over movie titles for substring search.
class MovieSuffixArray {
  public:
    MovieSuffixArray() = default;
    ~MovieSuffixArray() = default;

    // Builds the suffix array from a full collection of movies.
    void build(const std::vector<Movie> &movies) {
        for (const Movie &m : movies) {
            // std::string text = normalize(m.title + " " + m.plot);
            std::string text = normalize(m.title);

            std::for_each(text.begin(), text.end(), [&](char c) {
                corpus += c;
                position_to_id.push_back(m.id);
            });

            corpus += '\x01';
            position_to_id.push_back(m.id);
        }

        suffixes.resize(corpus.size());
        for (size_t i = 0; i < corpus.size(); i++)
            suffixes[i] = i;

        std::sort(suffixes.begin(), suffixes.end(), [&](size_t a, size_t b) {
            return corpus.compare(a, std::string::npos, corpus, b, std::string::npos) < 0;
        });
    }

    // Returns ids of movies whose title contains the query (case-insensitive).
    std::vector<int> search(const std::string &query) const {
        std::string q = normalize(query);
        if (q.empty() || corpus.empty())
            return {};

        size_t lo = binary_search_bound(q, true);
        size_t hi = binary_search_bound(q, false);

        std::unordered_set<int> found;
        std::for_each(suffixes.begin() + lo, suffixes.begin() + hi,
                      [&](size_t pos) { found.insert(position_to_id[pos]); });

        return std::vector<int>(found.begin(), found.end());
    }

    // Returns ids of movies matching any word of the phrase.
    std::vector<int> search_phrase(const std::string &phrase) const {
        std::unordered_set<int> found;
        std::string normalized = normalize(phrase);
        std::string word;

        for (char c : normalized) {
            if (c == ' ') {
                if (!word.empty()) {
                    for (int id : search(word))
                        found.insert(id);
                    word.clear();
                }
            } else {
                word += c;
            }
        }

        if (!word.empty())
            for (int id : search(word))
                found.insert(id);

        return std::vector<int>(found.begin(), found.end());
    }

  private:
    std::string corpus;
    std::vector<size_t> suffixes;
    std::vector<int> position_to_id;

    // Returns a lowercase copy of the string.
    std::string normalize(const std::string &s) const {
        auto result = s;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    // Compares the first query.size() chars of the suffix at pos with the query.
    int compare_prefix(size_t pos, const std::string &query) const {
        size_t len = std::min(query.size(), corpus.size() - pos);
        int cmp = corpus.compare(pos, len, query, 0, len);
        if (cmp != 0)
            return cmp;
        return len < query.size() ? -1 : 0;
    }

    // Binary search over the suffix array.
    size_t binary_search_bound(const std::string &query, bool strict) const {
        size_t left = 0, right = suffixes.size();
        while (left < right) {
            size_t mid = left + (right - left) / 2;
            int cmp = compare_prefix(suffixes[mid], query);
            bool go_right = strict ? (cmp < 0) : (cmp <= 0);

            if (go_right)
                left = mid + 1;
            else
                right = mid;
        }
        return left;
    }
};