#include "../algorithm/hashing/MovieHashIndex.cpp"
#include "../algorithm/tagger.cpp"
#include "../algorithm/trees/MovieSuffixArray.cpp"
#include "../algorithm/trees/MovieTree.cpp"
#include "ranking.cpp"
#include <algorithm>
#include <cctype>
#include <climits>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

// User request. Every field is optional.
struct SearchQuery {
    std::string text;
    std::string tag;
    std::string genre;
    std::optional<int> year_from;
    std::optional<int> year_to;
    size_t offset = 0;
    size_t limit = 5;
};

class SearchEngine {
  public:
    SearchEngine() = default;
    ~SearchEngine() = default;

    // Builds all indexes from the same collection of movies.
    void build(const std::vector<Movie> &movies) {
        catalog = movies;
        for (auto &movie : catalog)
            if (movie.tags.empty())
                movie.tags = movie_tagger::build_tags(movie);

        hash_index.build(catalog);
        tree_index.build(catalog);
        suffix_array.build(catalog);
    }

    std::vector<const Movie *> search(const SearchQuery &q) const {
        std::vector<const Movie *> candidates = !q.text.empty() ? search_by_text(q) : search_by_filters(q);
        std::string ranking_query = !q.text.empty() ? q.text : q.tag;
        return movie_ranking::rank(candidates, ranking_query, q.offset, q.limit);
    }

  private:
    std::vector<Movie> catalog;
    MovieHashIndex hash_index;
    MovieTree tree_index;
    MovieSuffixArray suffix_array;

    // Text search: suffix array finds ids, hash index resolves them, then filters apply.
    std::vector<const Movie *> search_by_text(const SearchQuery &q) const {
        std::vector<const Movie *> results;
        std::unordered_set<int> seen;

        for (int id : suffix_array.search_phrase(q.text)) {
            const Movie *m = hash_index.get_by_id(id);
            if (m && matches(*m, q) && seen.insert(id).second)
                results.push_back(m);
        }

        return results;
    }

    // Search without text: only genre and/or year range.
    std::vector<const Movie *> search_by_filters(const SearchQuery &q) const {
        bool has_years = q.year_from || q.year_to;
        int from = q.year_from.value_or(INT_MIN);
        int to = q.year_to.value_or(INT_MAX);
        if (from > to)
            std::swap(from, to);

        // Genre + years: range query on the tree.
        if (!q.genre.empty() && has_years)
            return filter_results(resolve_ids(tree_index.range_query(q.genre, from, to)), q);

        // Only genre: direct hash lookup.
        if (!q.genre.empty())
            return filter_results(hash_index.get_by_genre(q.genre), q);

        if (!has_years)
            return filter_results(all_movies(), q);

        // Only years.
        std::vector<const Movie *> results;
        std::unordered_set<int> seen;
        for (int y = from; y <= to; y++) {
            auto movies = hash_index.get_by_year(y);
            for (const Movie *movie : movies) {
                if (movie && seen.insert(movie->id).second)
                    results.push_back(movie);
            }
        }

        return filter_results(results, q);
    }

    // Converts ids into movie pointers using the hash index.
    std::vector<const Movie *> resolve_ids(const std::vector<int> &ids) const {
        std::vector<const Movie *> results;
        for (int id : ids)
            if (const Movie *m = hash_index.get_by_id(id))
                results.push_back(m);
        return results;
    }

    // Checks the genre and year filters of the query against a movie.
    bool matches(const Movie &m, const SearchQuery &q) const {
        if (!q.genre.empty() && to_lower(m.genre) != to_lower(q.genre))
            return false;
        if (!q.tag.empty() && !movie_tagger::has_tag(m, q.tag))
            return false;
        if (q.year_from && m.year < *q.year_from)
            return false;
        if (q.year_to && m.year > *q.year_to)
            return false;
        return true;
    }

    // Returns a lowercase copy of the string.
    std::string to_lower(std::string s) const {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    std::vector<const Movie *> all_movies() const {
        std::vector<const Movie *> results;
        results.reserve(catalog.size());
        for (const auto &movie : catalog)
            if (const Movie *resolved = hash_index.get_by_id(movie.id))
                results.push_back(resolved);
        return results;
    }

    std::vector<const Movie *> filter_results(const std::vector<const Movie *> &source,
                                              const SearchQuery &q) const {
        std::vector<const Movie *> filtered;
        std::unordered_set<int> seen;
        for (const Movie *movie : source) {
            if (!movie || !matches(*movie, q))
                continue;
            if (seen.insert(movie->id).second)
                filtered.push_back(movie);
        }
        return filtered;
    }
};