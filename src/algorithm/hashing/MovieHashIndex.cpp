#include "../models/Movie.h"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// Secondary index: attribute value -> ids of matching movies.
template <typename Key>
using HashIndex = std::unordered_map<Key, std::vector<int>>;

// Hash-based index for O(1) average-time lookups.
class MovieHashIndex {
  public:
    MovieHashIndex() = default;

    ~MovieHashIndex() = default;

    // Returns the total number of indexed movies.
    size_t get_size() const {
        return master_table.size();
    }

    // Returns pointers to every indexed movie for full-catalog filters.
    std::vector<const Movie *> get_all() const {
        std::vector<const Movie *> movies;
        movies.reserve(master_table.size());
        for (const auto &entry : master_table)
            movies.push_back(&entry.second);
        return movies;
    }

    // Returns the movie with the given id, or nullptr if not found.
    const Movie *get_by_id(int id) const {
        auto it = master_table.find(id);

        if (it == master_table.end())
            return nullptr;

        return &it->second;
    }

    // Returns all movies with the given title (case-insensitive).
    std::vector<const Movie *> get_by_title(const std::string &title) const {
        return resolve(index_by_title, normalize(title));
    }

    // Returns all movies released in the given year.
    std::vector<const Movie *> get_by_year(int year) const {
        return resolve(index_by_year, year);
    }

    // Returns all movies with the given origin (case-insensitive).
    std::vector<const Movie *> get_by_origin(const std::string &origin) const {
        return resolve(index_by_origin, normalize(origin));
    }

    // Returns all movies with the given director (case-insensitive).
    std::vector<const Movie *> get_by_director(const std::string &director) const {
        return resolve(index_by_director, normalize(director));
    }

    // Returns all movies with the given genre (case-insensitive).
    std::vector<const Movie *> get_by_genre(const std::string &genre) const {
        return resolve(index_by_genre, normalize(genre));
    }

    // Finds movies whose plot contains at least one query term, ignoring case.
    std::vector<const Movie *> get_by_plot_terms(const std::string &query) const {
        std::vector<std::string> terms;
        std::string term;
        for (unsigned char c : query) {
            if (std::isspace(c)) {
                if (!term.empty()) { terms.push_back(std::move(term)); term.clear(); }
            } else {
                term += static_cast<char>(std::tolower(c));
            }
        }
        if (!term.empty()) terms.push_back(std::move(term));
        if (terms.empty()) return {};

        std::unordered_set<int> found;
        for (const std::string &needle : terms) {
            if (needle.size() < 3) {
                for (const auto &entry : master_table)
                    if (contains_case_insensitive(entry.second.plot, needle))
                        found.insert(entry.first);
                continue;
            }

            const std::vector<int> *shortest = nullptr;
            for (size_t i = 0; i + 3 <= needle.size(); ++i) {
                auto it = index_by_plot_trigram.find(needle.substr(i, 3));
                if (it == index_by_plot_trigram.end()) { shortest = nullptr; break; }
                if (!shortest || it->second.size() < shortest->size()) shortest = &it->second;
            }
            if (!shortest) continue;
            for (int id : *shortest) {
                const Movie *movie = get_by_id(id);
                if (movie && contains_case_insensitive(movie->plot, needle)) found.insert(id);
            }
        }

        std::vector<const Movie *> results;
        results.reserve(found.size());
        for (int id : found)
            if (const Movie *movie = get_by_id(id)) results.push_back(movie);
        return results;
    }

    // Inserts a movie into all indexes. Ignored if the id already exists.
    void insert(const Movie &m) {
        if (master_table.count(m.id))
            return;

        master_table[m.id] = m;
        index_by_title[normalize(m.title)].push_back(m.id);
        index_by_year[m.year].push_back(m.id);
        index_by_origin[normalize(m.origin)].push_back(m.id);
        index_by_director[normalize(m.director)].push_back(m.id);
        index_by_genre[normalize(m.genre)].push_back(m.id);

        const std::string plot = normalize(m.plot);
        std::unordered_set<std::string> trigrams;
        for (size_t i = 0; i + 3 <= plot.size(); ++i)
            trigrams.insert(plot.substr(i, 3));
        for (const auto &trigram : trigrams)
            index_by_plot_trigram[trigram].push_back(m.id);
    }

    // Builds the index from a full collection of movies.
    void build(const std::vector<Movie> &movies) {
        std::for_each(movies.begin(), movies.end(), [&](const Movie &movie) { insert(movie); });
    }

  private:
    std::unordered_map<int, Movie> master_table;

    HashIndex<std::string> index_by_title;
    HashIndex<int> index_by_year;
    HashIndex<std::string> index_by_origin;
    HashIndex<std::string> index_by_director;
    HashIndex<std::string> index_by_genre;
    HashIndex<std::string> index_by_plot_trigram;

    // Converts a string to lowercase for case-insensitive comparison.
    std::string normalize(const std::string &s) const {
        auto result = s;

        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        return result;
    }

    bool contains_case_insensitive(const std::string &text, const std::string &needle) const {
        return std::search(text.begin(), text.end(), needle.begin(), needle.end(),
                           [](unsigned char a, unsigned char b) {
                               return std::tolower(a) == std::tolower(b);
                           }) != text.end();
    }

    // Resolves a key into the corresponding list of movie pointers.
    template <typename Key>
    std::vector<const Movie *> resolve(const HashIndex<Key> &index, const Key &key) const {
        std::vector<const Movie *> results;

        auto it = index.find(key);
        if (it == index.end())
            return results;

        std::for_each(it->second.begin(), it->second.end(),
                      [&](int id) { results.push_back(get_by_id(id)); });

        return results;
    }
};
