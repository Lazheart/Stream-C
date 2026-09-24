#include "../src/algorithm/loaders/CsvMovieLoader.cpp"
#include "../src/algorithm/loaders/MockMovieLoader.cpp"
#include "../src/search/searchEngine.cpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

static void test_csv_loader_reads_rows() {
    const std::string path = "/tmp/stream-c-test-search.csv";
    std::ofstream file(path);
    file << "Release Year,Title,Origin/Ethnicity,Director,Cast,Genre,Wiki Page,Plot\n";
    file << "2001,\"Bar, Movie\",Peru,Ana,Actor,Drama,https://example.org/a,Historia bar\n";
    file << "2002,Simple,USA,Bob,,Comedy,https://example.org/b,Funny\n";
    file.close();

    CsvMovieLoader loader(path);
    auto movies = loader.load();

    assert(movies.size() == 2);
    assert(movies[0].title == "Bar, Movie");
    assert(movies[1].cast == "unknown");
}

static void test_search_by_text_tag_and_pagination() {
    MockMovieLoader loader;
    SearchEngine engine;
    engine.build(loader.load());

    SearchQuery text_query;
    text_query.text = "train";
    auto text_results = engine.search(text_query);
    assert(!text_results.empty());

    SearchQuery tag_query;
    tag_query.tag = "western";
    auto tag_results = engine.search(tag_query);
    assert(!tag_results.empty());

    bool found_train_robbery = false;
    for (const Movie *movie : tag_results)
        if (movie && movie->title == "The Great Train Robbery")
            found_train_robbery = true;
    assert(found_train_robbery);

    SearchQuery page_1;
    page_1.text = "the";
    page_1.limit = 1;
    page_1.offset = 0;
    auto page_1_results = engine.search(page_1);
    assert(page_1_results.size() == 1);

    SearchQuery page_2 = page_1;
    page_2.offset = 1;
    auto page_2_results = engine.search(page_2);
    assert(page_2_results.size() == 1);
    assert(page_1_results[0]->id != page_2_results[0]->id);
}

int main() {
    try {
        test_csv_loader_reads_rows();
        test_search_by_text_tag_and_pagination();
        std::cout << "OK: search and loader tests\n";
    } catch (const std::exception &e) {
        std::cerr << "FALLO: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
