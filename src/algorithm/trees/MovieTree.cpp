#include "../models/Movie.h"
#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// year -> ids, kept sorted automatically (std::map is a balanced BST)
using YearIndex = std::map<int, std::vector<int>>;

// Tree-based index for range queries by year within a genre. Stores ids only.
class MovieTree {
  public:
    MovieTree() = default;
    ~MovieTree() = default;

    // Returns the total number of indexed movies.
    size_t get_size() const {
        return indexed_ids.size();
    }

    // Returns the movie with the given id, or nullptr if not found.
    /*const Movie *get_by_id(int id) const {
        auto it = master_table.find(id);

        if (it == master_table.end())
            return nullptr;

        return &it->second;
    }*/

    // Inserts a movie into the tree. Ignored if the id already exists.
    void insert(const Movie &m) {
        if (!indexed_ids.insert(m.id).second)
            return;

        by_genre[normalize(m.genre)][m.year].push_back(m.id);
    }

    // Builds the tree from a full collection of movies.
    void build(const std::vector<Movie> &movies) {
        std::for_each(movies.begin(), movies.end(), [&](const Movie &movie) { insert(movie); });
    }

    // Returns ids of movies of the given genre released between start_year and end_year, inclusive.
    // The genre lookup is case-insensitive.
    std::vector<int> range_query(const std::string &genre, int start_year, int end_year) const {
        std::vector<int> results;

        auto genre_it = by_genre.find(normalize(genre));
        if (genre_it == by_genre.end())
            return results;

        const YearIndex &years = genre_it->second;
        auto lo = years.lower_bound(start_year);
        auto hi = years.upper_bound(end_year);

        std::for_each(lo, hi, [&](const auto &pair) {
            results.insert(results.end(), pair.second.begin(), pair.second.end());
        });
        return results;
    }

  private:
    std::unordered_set<int> indexed_ids;
    std::unordered_map<std::string, YearIndex> by_genre;

    // Converts a string to lowercase for case-insensitive comparison.
    std::string normalize(const std::string &s) const {
        auto result = s;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return result;
    }
};