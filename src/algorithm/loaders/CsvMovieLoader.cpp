#include "MovieLoader.cpp"
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

class CsvMovieLoader : public MovieLoader {
  public:
    explicit CsvMovieLoader(std::string path) : path(std::move(path)) {}

    std::vector<Movie> load() override {
        std::ifstream file(path);
        if (!file)
            throw std::runtime_error("No se pudo abrir el archivo CSV: " + path);

        std::string line;
        if (!std::getline(file, line))
            return {};

        std::vector<Movie> movies;
        int next_id = 1;

        while (std::getline(file, line)) {
            auto fields = parse_csv_line(line);
            if (fields.size() < 8)
                continue;

            Movie movie{};
            movie.id = next_id++;
            movie.year = parse_year(fields[0]);
            movie.title = clean_field(fields[1]);
            movie.origin = clean_field(fields[2]);
            movie.director = clean_field(fields[3]);
            movie.cast = clean_field(fields[4]);
            movie.genre = clean_field(fields[5]);
            movie.wiki_url = clean_field(fields[6]);
            movie.plot = clean_field(fields[7]);
            movies.push_back(std::move(movie));
        }

        return movies;
    }

  private:
    std::string path;

    static std::string clean_field(std::string value) {
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
            value.pop_back();

        size_t i = 0;
        while (i < value.size() && std::isspace(static_cast<unsigned char>(value[i])))
            ++i;

        if (i > 0)
            value.erase(0, i);

        return value.empty() ? "unknown" : value;
    }

    static long parse_year(const std::string &value) {
        try {
            return std::stol(clean_field(value));
        } catch (...) {
            return 0;
        }
    }

    static std::vector<std::string> parse_csv_line(const std::string &line) {
        std::vector<std::string> fields;
        std::string current;
        bool in_quotes = false;

        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];

            if (c == '"') {
                if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') {
                    current += '"';
                    ++i;
                } else {
                    in_quotes = !in_quotes;
                }
            } else if (c == ',' && !in_quotes) {
                fields.push_back(current);
                current.clear();
            } else {
                current += c;
            }
        }

        fields.push_back(current);
        return fields;
    }
};