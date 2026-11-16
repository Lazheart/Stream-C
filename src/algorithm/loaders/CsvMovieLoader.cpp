#include "MovieLoader.cpp"
#include "CsvParser.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// Trims leading and trailing whitespace from a string.
static std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// Parses the year from a string. Returns 0 on failure.
static long parse_year(const std::string &s) {
    std::string t = trim(s);
    if (t.empty())
        return 0;
    char* end;
    long val = std::strtol(t.c_str(), &end, 10);
    if (end == t.c_str() + t.size()) {
        return val;
    }
    return 0;
}

// CSV loader: reads a file whose columns are (in order):
//   Release Year, Title, Origin/Ethnicity, Director, Cast, Genre, Wiki Page, Plot
// The first row is treated as a header and skipped.
class CsvMovieLoader : public MovieLoader {
  public:
    explicit CsvMovieLoader(std::string path) : path(std::move(path)) {}

    std::vector<Movie> load() override {
        std::ifstream file(path);
        if (!file.is_open())
            throw std::runtime_error("CsvMovieLoader: cannot open file: " + path);

        std::vector<Movie> movies;
        bool first_line = true;
        int next_id = 1;
        std::vector<std::string> fields;

        while (csv::read_record(file, fields)) {
            // Skip header row.
            if (first_line) {
                first_line = false;
                continue;
            }

            // Skip blank lines.
            if (fields.size() == 1 && trim(fields[0]).empty())
                continue;

            // Expect at least 8 columns; pad missing ones with empty strings.
            while (fields.size() < 8)
                fields.push_back("");

            Movie m;
            m.id       = next_id++;
            m.year     = parse_year(fields[0]);
            m.title    = trim(fields[1]);
            m.origin   = trim(fields[2]);
            m.director = trim(fields[3]);
            m.cast     = trim(fields[4]);
            m.genre    = trim(fields[5]);
            m.wiki_url = trim(fields[6]);
            m.plot     = trim(fields[7]);

            // Normalize empty sentinel values.
            if (m.title.empty())
                continue; // Skip rows without a title.

            movies.push_back(std::move(m));
        }

        return movies;
    }

  private:
    std::string path;
};
