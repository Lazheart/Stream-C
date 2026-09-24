#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <cstdlib>

#include "algorithm/loaders/CsvMovieLoader.cpp"
#include "../tests/MockMovieLoader.cpp"
#include "search/searchEngine.cpp"
#include "helpers/userDataManager.cpp"

void clear_screen() {
#ifdef _WIN32
    std::system("cls");
#else
    std::system("clear");
#endif
}

void print_movie_list(const std::vector<const Movie*>& results, int offset) {
    if (results.empty()) {
        std::cout << "No results found.\n";
        return;
    }
    for (size_t i = 0; i < results.size(); ++i) {
        const Movie &m = *results[i];
        std::cout << "[" << (offset + i + 1) << "] " << m.title << " (" << m.year << ")\n";
    }
}

int main(int argc, char* argv[]) {
    std::vector<Movie> movies;
    std::string csv_path = "data/data.csv";
    if (argc > 1) {
        csv_path = argv[1];
    }
    
    std::cout << "Loading movies from " << csv_path << "...\n";
    try {
        CsvMovieLoader loader(csv_path);
        movies = loader.load();
    } catch (const std::exception &e) {
        std::cerr << "Warning: " << e.what() << " - falling back to mock data.\n";
        MockMovieLoader mock;
        movies = mock.load();
    }

    std::cout << "Building search index for " << movies.size() << " movies...\n";
    SearchEngine engine;
    engine.build(movies);
    std::cout << "Index built.\n\n";

    UserDataManager user_data("userData.json", true);

    while (true) {
        clear_screen();
        std::cout << "===========================\n";
        std::cout << "      Stream-C CLI\n";
        std::cout << "===========================\n";
        std::cout << "1. Search Movies\n";
        std::cout << "2. View Watch Later List\n";
        std::cout << "3. Exit\n";
        std::cout << "Select an option: ";
        
        std::string choice;
        if (!std::getline(std::cin, choice)) break;

        if (choice == "1") {
            std::cout << "Enter search query: ";
            std::string query_text;
            std::getline(std::cin, query_text);
            
            int offset = 0;
            const int limit = 10;
            
            while (true) {
                SearchQuery q;
                q.text = query_text;
                q.offset = offset;
                q.limit = limit;
                
                auto results = engine.search(q);
                
                clear_screen();
                std::cout << "Search Results for '" << query_text << "' (Page " << (offset / limit + 1) << ")\n";
                std::cout << "------------------------------------------\n";
                print_movie_list(results, offset);
                std::cout << "------------------------------------------\n";
                std::cout << "[N]ext page | [P]revious page | [S]elect movie | [B]ack\n";
                std::cout << "Choice: ";
                
                std::string action;
                std::getline(std::cin, action);
                
                if (action == "N" || action == "n") {
                    if (results.size() == limit) {
                        offset += limit;
                    } else {
                        std::cout << "Already on the last page. Press Enter to continue...";
                        std::string dummy; std::getline(std::cin, dummy);
                    }
                } else if (action == "P" || action == "p") {
                    if (offset >= limit) {
                        offset -= limit;
                    } else {
                        std::cout << "Already on the first page. Press Enter to continue...";
                        std::string dummy; std::getline(std::cin, dummy);
                    }
                } else if (action == "S" || action == "s") {
                    std::cout << "Enter the number of the movie to select: ";
                    std::string num_str;
                    std::getline(std::cin, num_str);
                    try {
                        int num = std::stoi(num_str);
                        int idx = num - offset - 1;
                        if (idx >= 0 && idx < (int)results.size()) {
                            const Movie* selected = results[idx];
                            clear_screen();
                            std::cout << *selected;
                            std::cout << "\n[A]dd to Watch Later | [B]ack\nChoice: ";
                            std::string sub_action;
                            std::getline(std::cin, sub_action);
                            if (sub_action == "A" || sub_action == "a") {
                                MovieData md(selected->title, selected->year, selected->genre, selected->director, selected->plot);
                                md.id = std::to_string(selected->id);
                                if (user_data.addWatchLater(md)) {
                                    user_data.saveToFile();
                                    std::cout << "Added to Watch Later. Press Enter to continue...";
                                } else {
                                    std::cout << "Already in Watch Later. Press Enter to continue...";
                                }
                                std::string dummy; std::getline(std::cin, dummy);
                            }
                        } else {
                            std::cout << "Invalid number. Press Enter to continue...";
                            std::string dummy; std::getline(std::cin, dummy);
                        }
                    } catch (...) {
                        std::cout << "Invalid input. Press Enter to continue...";
                        std::string dummy; std::getline(std::cin, dummy);
                    }
                } else if (action == "B" || action == "b") {
                    break;
                }
            }
        } else if (choice == "2") {
            clear_screen();
            user_data.displayStartupSummary(std::cout);
            std::cout << "\nPress Enter to return...";
            std::string dummy; std::getline(std::cin, dummy);
        } else if (choice == "3") {
            std::cout << "Goodbye!\n";
            break;
        }
    }

    return 0;
}
