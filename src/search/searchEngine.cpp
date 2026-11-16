#include "../algorithm/hashing/MovieHashIndex.cpp"
#include "../algorithm/tagger.cpp"
#include "../algorithm/trees/MovieSuffixArray.cpp"
#include "../algorithm/trees/MovieTree.cpp"
#include "ranking.cpp"
#include <algorithm>
#include <climits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

// User request. Every field is optional except that at least one should be set.
struct SearchQuery {
    std::string text;       // word / phrase / sub-word to search in title+plot
    std::string genre;      // exact genre filter (case-insensitive)
    std::string tag;        // tag filter: matches any of genre/origin/director/cast tokens
    std::optional<int> year_from;
    std::optional<int> year_to;
    int offset = 0;         // first result index (0-based, for pagination)
    int limit  = 5;         // max results returned (0 = unlimited)
};

// Builder pattern: keeps query construction readable as the query grows.
class SearchQueryBuilder {
  public:
    SearchQueryBuilder &text(std::string value) { query.text = std::move(value); return *this; }
    SearchQueryBuilder &genre(std::string value) { query.genre = std::move(value); return *this; }
    SearchQueryBuilder &tag(std::string value) { query.tag = std::move(value); return *this; }
    SearchQueryBuilder &years(std::optional<int> from, std::optional<int> to) {
        query.year_from = from; query.year_to = to; return *this;
    }
    SearchQueryBuilder &page(int offset, int limit) { query.offset = offset; query.limit = limit; return *this; }
    SearchQuery build() const { return query; }
  private:
    SearchQuery query;
};

class SearchEngine {
  public:
    SearchEngine() = default;
    ~SearchEngine() = default;

    // Builds all indexes from the same collection of movies.
    // Tags are generated here so callers only need to call build() once.
    void build(std::vector<Movie> &movies) {
        tagger.tag_all(movies);
        hash_index.build(movies);
        tree_index.build(movies);
        suffix_array.build(movies);
    }

    // Overload for const collections (tags must already be set).
    void build(const std::vector<Movie> &movies) {
        catalog = movies;
        tagger.tag_all(catalog);
        hash_index.build(catalog);
        tree_index.build(catalog);
        suffix_array.build(catalog);
    }

    // Returns ranked, paginated results for the query.
    std::vector<const Movie *> search(const SearchQuery &q) const {
        // Facade pattern: callers use one operation for index lookup,
        // filtering, de-duplication, ranking, and pagination.
        auto strategy = SearchStrategyFactory::create(q);
        std::vector<const Movie *> candidates = strategy->execute(*this, q);

        // Deduplicate (suffix array + filter paths may yield duplicates).
        candidates = deduplicate(candidates);

        // Apply ranking and return the requested page.
        return Ranker::rank_and_page(std::move(candidates),
                                     q.text, q.tag,
                                     q.offset, q.limit);
    }

  private:
    // Strategy pattern: select candidate-generation behavior by query type.
    class SearchStrategy {
      public:
        virtual ~SearchStrategy() = default;
        virtual std::vector<const Movie *> execute(const SearchEngine &, const SearchQuery &) const = 0;
    };
    class TextSearchStrategy final : public SearchStrategy {
      public:
        std::vector<const Movie *> execute(const SearchEngine &engine, const SearchQuery &q) const override {
            return engine.search_by_text(q);
        }
    };
    class FilterSearchStrategy final : public SearchStrategy {
      public:
        std::vector<const Movie *> execute(const SearchEngine &engine, const SearchQuery &q) const override {
            return engine.search_by_filters(q);
        }
    };
    // Factory pattern: centralizes creation of the matching search strategy.
    class SearchStrategyFactory {
      public:
        static std::unique_ptr<SearchStrategy> create(const SearchQuery &q) {
            if (!q.text.empty()) return std::make_unique<TextSearchStrategy>();
            return std::make_unique<FilterSearchStrategy>();
        }
    };

    std::vector<Movie> catalog;
    MovieHashIndex hash_index;
    MovieTree      tree_index;
    MovieSuffixArray suffix_array;
    Tagger         tagger;

    // Text search: suffix array finds ids, hash index resolves them, then all
    // filters (genre, year, tag) are applied.
    std::vector<const Movie *> search_by_text(const SearchQuery &q) const {
        std::vector<const Movie *> results = resolve_ids(suffix_array.search_phrase(q.text));
        auto plot_matches = hash_index.get_by_plot_terms(q.text);
        results.insert(results.end(), plot_matches.begin(), plot_matches.end());
        results.erase(std::remove_if(results.begin(), results.end(), [&](const Movie *m) {
            return !m || !matches(*m, q);
        }), results.end());
        return results;
    }

    // Filter-only search (no text): genre, year range, and/or tag.
    std::vector<const Movie *> search_by_filters(const SearchQuery &q) const {
        bool has_years = q.year_from || q.year_to;
        int from = q.year_from.value_or(INT_MIN);
        int to   = q.year_to.value_or(INT_MAX);

        std::vector<const Movie *> pool;

        // Genre + years: range query on the tree (normalized at query time).
        if (!q.genre.empty() && has_years)
            pool = resolve_ids(tree_index.range_query(to_lower(q.genre), from, to));

        // Only genre: direct hash lookup.
        else if (!q.genre.empty())
            pool = hash_index.get_by_genre(q.genre);

        // Only years: iterate over each year in range via hash index.
        else if (q.year_from && q.year_to) {
            for (int y = from; y <= to; y++) {
                auto batch = hash_index.get_by_year(y);
                pool.insert(pool.end(), batch.begin(), batch.end());
            }
        }

        // Tag-only search scans the indexed catalog because tags are generated
        // from multiple metadata fields rather than stored in a dedicated index.
        else if (!q.tag.empty())
            pool = hash_index.get_all();

        // Apply tag filter on top of pool.
        if (!q.tag.empty()) {
            std::vector<const Movie *> tagged;
            for (const Movie *m : pool)
                if (m && tagger.has_tag(*m, q.tag))
                    tagged.push_back(m);
            return tagged;
        }

        return pool;
    }

    // Converts ids into movie pointers using the hash index.
    std::vector<const Movie *> resolve_ids(const std::vector<int> &ids) const {
        std::vector<const Movie *> results;
        for (int id : ids)
            if (const Movie *m = hash_index.get_by_id(id))
                results.push_back(m);
        return results;
    }

    // Removes duplicate movie pointers (same id).
    std::vector<const Movie *> deduplicate(std::vector<const Movie *> v) const {
        std::unordered_set<int> seen;
        std::vector<const Movie *> out;
        for (const Movie *m : v) {
            if (m && seen.insert(m->id).second)
                out.push_back(m);
        }
        return out;
    }

    // Checks all metadata filters of the query against a movie.
    bool matches(const Movie &m, const SearchQuery &q) const {
        if (!q.genre.empty() && to_lower(m.genre) != to_lower(q.genre))
            return false;
        if (!q.tag.empty() && !tagger.has_tag(m, q.tag))
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
