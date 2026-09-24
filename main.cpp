#include <iostream>
#include "src/algorithm/loaders/MockMovieLoader.cpp"
#include "src/search/searchEngine.cpp"

int main() {
    MockMovieLoader loader;
    SearchEngine engine;
    engine.build(loader.load());

    SearchQuery query;
    query.text = "train";
    query.limit = 5;
    auto results = engine.search(query);

    std::cout << "Resultados: " << results.size() << '\n';
    for (const Movie *movie : results)
        std::cout << "- " << movie->title << " (" << movie->year << ")\n";

    return 0;
}

