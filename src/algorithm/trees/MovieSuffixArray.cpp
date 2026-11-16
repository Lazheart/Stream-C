#include "../models/Movie.h"
#include <algorithm>
#include <cstddef>
#ifndef __EMSCRIPTEN__
#include <future>
#include <thread>
#endif
#include <numeric>
#include <string>
#include <utility>
#include <unordered_set>
#include <vector>

// Prefix-doubling suffix array over movie titles for substring search.
class MovieSuffixArray {
  public:
    MovieSuffixArray() = default;
    ~MovieSuffixArray() = default;

    // Builds the suffix array from a full collection of movies.
    void build(const std::vector<Movie> &movies) {
        corpus.clear();
        position_to_id.clear();
        suffixes.clear();
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
        std::iota(suffixes.begin(), suffixes.end(), size_t{0});
        if (suffixes.empty()) return;

        std::vector<int> rank(corpus.size());
        std::vector<int> next_rank(corpus.size());
        for (size_t i = 0; i < corpus.size(); ++i)
            rank[i] = static_cast<unsigned char>(corpus[i]);

        for (size_t width = 1; width < corpus.size();) {
            std::sort(suffixes.begin(), suffixes.end(), [&](size_t a, size_t b) {
                if (rank[a] != rank[b]) return rank[a] < rank[b];
                const int rank_a = a + width < corpus.size() ? rank[a + width] : -1;
                const int rank_b = b + width < corpus.size() ? rank[b + width] : -1;
                return rank_a < rank_b;
            });

            next_rank[suffixes[0]] = 0;
            for (size_t i = 1; i < suffixes.size(); ++i) {
                const size_t previous = suffixes[i - 1];
                const size_t current = suffixes[i];
                const int previous_second = previous + width < corpus.size() ? rank[previous + width] : -1;
                const int current_second = current + width < corpus.size() ? rank[current + width] : -1;
                next_rank[current] = next_rank[previous] +
                    (rank[previous] != rank[current] || previous_second != current_second);
            }
            rank.swap(next_rank);
            if (rank[suffixes.back()] == static_cast<int>(suffixes.size() - 1)) break;
            if (width > corpus.size() / 2) break;
            width *= 2;
        }
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
        std::string normalized = normalize(phrase);
        std::vector<std::string> words;
        std::string word;
        for (char c : normalized) {
            if (c == ' ') {
                if (!word.empty()) { words.push_back(std::move(word)); word.clear(); }
            } else {
                word += c;
            }
        }
        if (!word.empty()) words.push_back(std::move(word));

        // Queries with multiple terms are independent over the immutable suffix
        // array. Run sufficiently large queries concurrently; short queries
        // stay inline to avoid paying thread-launch overhead.
        std::vector<std::vector<int>> matches(1);
#ifndef __EMSCRIPTEN__
        if (words.size() >= 2) {
            std::vector<std::future<std::vector<int>>> tasks;
            const size_t available = std::max(1u, std::thread::hardware_concurrency());
            const size_t workers = std::min({words.size(), available, size_t{4}});
            tasks.reserve(workers);
            for (size_t worker = 0; worker < workers; ++worker) {
                tasks.emplace_back(std::async(std::launch::async, [this, &words, worker, workers] {
                    std::vector<int> ids;
                    for (size_t i = worker; i < words.size(); i += workers) {
                        auto found = search(words[i]);
                        ids.insert(ids.end(), found.begin(), found.end());
                    }
                    return ids;
                }));
            }
            for (auto &task : tasks) {
                auto ids = task.get();
                matches.front().insert(matches.front().end(), ids.begin(), ids.end());
            }
        }
#endif
        if (words.size() < 2) {
            if (!words.empty()) matches[0] = search(words[0]);
        }
#ifdef __EMSCRIPTEN__
        else {
            // The WebAssembly target is built without pthreads.
            for (const auto &term : words) {
                auto ids = search(term);
                matches[0].insert(matches[0].end(), ids.begin(), ids.end());
            }
        }
#endif

        std::unordered_set<int> found;
        for (const auto &ids : matches) found.insert(ids.begin(), ids.end());
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
