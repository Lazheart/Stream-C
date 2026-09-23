#include "../models/Movie.h"
#include <vector>

class MovieLoader {
  public:
    virtual ~MovieLoader() = default;
    virtual std::vector<Movie> load() = 0;
};