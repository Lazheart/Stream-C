#include "../algorithm/hashing/MovieHashIndex.cpp"
#include "../algorithm/trees/MovieSuffixArray.cpp"
#include "../algorithm/trees/MovieTree.cpp"
#include <optional>
#include <string>
#include <vector>

// User request. Every field is optional.
struct SearchQuery {
    std::string text;
    std::string genre;
    std::optional<int> year_from;
    std::optional<int> year_to;
};

class SearchEngine {
  public:
    SearchEngine() = default;
    ~SearchEngine() = default;

    // Builds all indexes from the same collection of movies.
    void build(const std::vector<Movie> &movies) {
        hash_index.build(movies);
        tree_index.build(movies);
        suffix_array.build(movies);
    }

    std::vector<const Movie *> search(const SearchQuery &q) const {
        if (!q.text.empty())
            return search_by_text(q);
        return search_by_filters(q);
    }

  private:
    MovieHashIndex hash_index;
    MovieTree tree_index;
    MovieSuffixArray suffix_array;

    // Text search: suffix array finds ids, hash index resolves them, then filters apply.
    std::vector<const Movie *> search_by_text(const SearchQuery &q) const {
        std::vector<const Movie *> results;

        for (int id : suffix_array.search_phrase(q.text)) {
            const Movie *m = hash_index.get_by_id(id);
            if (m && matches(*m, q))
                results.push_back(m);
        }

        return results;
    }

    // Search without text: only genre and/or year range.
    std::vector<const Movie *> search_by_filters(const SearchQuery &q) const {
        bool has_years = q.year_from || q.year_to;
        int from = q.year_from.value_or(INT_MIN);
        int to = q.year_to.value_or(INT_MAX);

        // Genre + years: range query on the tree.
        if (!q.genre.empty() && has_years)
            return resolve_ids(tree_index.range_query(q.genre, from, to));

        // Only genre: direct hash lookup.
        if (!q.genre.empty())
            return hash_index.get_by_genre(q.genre);

        // Only years: needs both limits, then one hash lookup per year.
        std::vector<const Movie *> results;
        if (q.year_from && q.year_to) {
            for (int y = from; y <= to; y++) {
                auto movies = hash_index.get_by_year(y);
                results.insert(results.end(), movies.begin(), movies.end());
            }
        }

        return results;
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
};