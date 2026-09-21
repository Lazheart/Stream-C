#pragma once
#include <ostream>
#include <string>
#include <vector>

// Represents a single movie record loaded from the dataset.
struct Movie {
    int id;
    long int year;
    std::string title;
    std::string origin;
    std::string director;
    std::string cast;
    std::string genre;
    std::string wiki_url;
    std::string plot;
    std::vector<std::string> tags;
};

inline std::ostream &operator<<(std::ostream &os, const Movie &m) {
    os << "[" << m.id << "]\n"
       << "  Title: " << m.title << "\n"
       << "  Year: " << m.year << "\n"
       << "  Director: " << m.director << "\n"
       << "  Genre: " << m.genre << "\n"
       << "  Origin: " << m.origin << "\n"
       << "  Plot: " << m.plot << "\n";
    return os;
}