#pragma once

#include <istream>
#include <stdexcept>
#include <string>
#include <vector>

// Reads one RFC 4180 style record, including quoted fields spanning lines.
// Returns false only when EOF is reached before a new record starts.
namespace csv {
inline bool read_record(std::istream &input, std::vector<std::string> &row) {
    enum class State { Start, Plain, Quoted, Closed };
    State state = State::Start;
    row.clear();
    std::string field;
    bool has_data = false;
    char c;

    while (input.get(c)) {
        has_data = true;
        if (state == State::Quoted) {
            if (c == '"') state = State::Closed;
            else field += c;
            continue;
        }
        if (state == State::Closed && c == '"') {
            field += '"';
            state = State::Quoted;
            continue;
        }
        if (c == ',') {
            row.push_back(field);
            field.clear();
            state = State::Start;
        } else if (c == '\r' || c == '\n') {
            if (c == '\r' && input.peek() == '\n') input.get();
            row.push_back(field);
            return true;
        } else if (c == '"' && state == State::Start) {
            state = State::Quoted;
        } else {
            if (c == '"' || state == State::Closed)
                throw std::runtime_error("CSV: comillas invalidas");
            field += c;
            state = State::Plain;
        }
    }

    if (input.bad()) throw std::runtime_error("CSV: error de lectura");
    if (state == State::Quoted)
        throw std::runtime_error("CSV: campo entre comillas sin cerrar");
    if (!has_data) return false;
    row.push_back(field);
    return true;
}
} // namespace csv
