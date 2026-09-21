#include "MovieLoader.cpp"
#include <vector>

class CsvMovieLoader : public MovieLoader {
  public:
    explicit CsvMovieLoader(std::string path) : path(std::move(path)) {}

    std::vector<Movie> load() override {
        throw std::runtime_error("CsvMovieLoader not implemented yet");
    }

  private:
    std::string path;
};