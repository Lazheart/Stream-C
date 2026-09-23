#include "../models/Movie.h"
#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

// year -> ids, kept sorted automatically (std::map is a balanced BST)
using HashIndex = std::map<int, std::vector<int>>;

// Tree-based index for O(log k) lookups and range queries by year.
class MovieTree {
  public:
    MovieTree() = default;
    ~MovieTree() = default;

    // Returns the total number of indexed movies.
    size_t get_size() const {
        return master_table.size();
    }

    // Returns the movie with the given id, or nullptr if not found.
    const Movie *get_by_id(int id) const {
        auto it = master_table.find(id);

        if (it == master_table.end())
            return nullptr;

        return &it->second;
    }

    // Inserts a movie into the tree. Ignored if the id already exists.
    void insert(const Movie &m) {
        if (master_table.count(m.id))
            return;

        master_table[m.id] = m;
        by_genre[m.genre][m.year].push_back(m.id);
    }

    // Builds the tree from a full collection of movies.
    void build(const std::vector<Movie> &movies) {
        std::for_each(movies.begin(), movies.end(), [&](const Movie &movie) { insert(movie); });
    }

    // Returns movies of the given genre released between start_year and end_year, inclusive.
    std::vector<const Movie *> range_query(const std::string &genre, int start_year,
                                           int end_year) const {
        std::vector<const Movie *> results;

        auto genre_it = by_genre.find(genre);
        if (genre_it == by_genre.end())
            return results;

        const HashIndex &years = genre_it->second;
        auto lo = years.lower_bound(start_year);
        auto hi = years.upper_bound(end_year);

        std::for_each(lo, hi, [&](const auto &pair) {
            std::for_each(pair.second.begin(), pair.second.end(),
                          [&](int id) { results.push_back(get_by_id(id)); });
        });

        return results;
    }

  private:
    std::unordered_map<int, Movie> master_table;
    std::unordered_map<std::string, HashIndex> by_genre;
};