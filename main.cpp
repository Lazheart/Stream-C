#include <iostream>
#include <string>
#include <vector>

#include "src/algorithm/loaders/CsvMovieLoader.cpp"
#include "tests/MockMovieLoader.cpp"
#include "src/search/searchEngine.cpp"

// Prints a ranked results page to stdout.
static void print_results(const std::vector<const Movie *> &results,
                           int page_offset) {
    if (results.empty()) {
        std::cout << "  (no results)\n";
        return;
    }
    for (size_t i = 0; i < results.size(); ++i) {
        const Movie &m = *results[i];
        std::cout << "  " << (page_offset + static_cast<int>(i) + 1) << ". "
                  << m.title << " (" << m.year << ")  [" << m.genre << "]\n";
        if (!m.plot.empty()) {
            std::string preview = m.plot.substr(0, 100);
            if (m.plot.size() > 100)
                preview += "...";
            std::cout << "     " << preview << "\n";
        }
        std::cout << "\n";
    }
}

int main(int argc, char *argv[]) {
    // -------------------------------------------------------------------------
    // 1. Load movies: prefer CSV if path given, else fall back to mock data.
    // -------------------------------------------------------------------------
    std::vector<Movie> movies;
    if (argc > 1) {
        std::string csv_path = argv[1];
        std::cout << "Loading from CSV: " << csv_path << "\n";
        try {
            CsvMovieLoader loader(csv_path);
            movies = loader.load();
        } catch (const std::exception &e) {
            std::cerr << "Warning: " << e.what() << " — falling back to mock data.\n";
            MockMovieLoader mock;
            movies = mock.load();
        }
    } else {
        std::cout << "No CSV path given — using built-in mock data.\n";
        MockMovieLoader mock;
        movies = mock.load();
    }

    std::cout << "Loaded " << movies.size() << " movie(s).\n\n";

    // -------------------------------------------------------------------------
    // 2. Build all indexes (tags are generated inside build()).
    // -------------------------------------------------------------------------
    SearchEngine engine;
    engine.build(movies);

    // -------------------------------------------------------------------------
    // 3. Demo queries.
    // -------------------------------------------------------------------------

    // Query A: sub-word "train", western tag, 1900-1910, top 5
    {
        SearchQuery q;
        q.text      = "train";
        q.tag       = "western";
        q.year_from = 1900;
        q.year_to   = 1910;
        q.offset    = 0;
        q.limit     = 5;

        std::cout << "=== Query A: text=\"train\" | tag=\"western\" | years=1900-1910 ===\n";
        auto results = engine.search(q);
        print_results(results, q.offset);
    }

    // Query B: phrase "train robbery", no filters, first page
    {
        SearchQuery q;
        q.text  = "train robbery";
        q.limit = 5;

        std::cout << "=== Query B: text=\"train robbery\" (top 5) ===\n";
        auto results = engine.search(q);
        print_results(results, q.offset);
    }

    // Query C: genre "comedy", no text
    {
        SearchQuery q;
        q.genre = "comedy";
        q.limit = 5;

        std::cout << "=== Query C: genre=\"comedy\" (top 5) ===\n";
        auto results = engine.search(q);
        print_results(results, q.offset);
    }

    // Query D: second page of Query B (offset 5)
    {
        SearchQuery q;
        q.text   = "train robbery";
        q.offset = 5;
        q.limit  = 5;

        std::cout << "=== Query D: text=\"train robbery\" (page 2, offset 5) ===\n";
        auto results = engine.search(q);
        print_results(results, q.offset);
    }

    return 0;
}
