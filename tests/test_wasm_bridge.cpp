#include "../wasm_bridge.cpp"
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
    const char *csv_text =
        "Release Year,Title,Origin/Ethnicity,Director,Cast,Genre,Wiki Page,Plot\n"
        "2000,Forest Signal,Peru,Elena Ruiz,Actor,Drama,https://example.org/film,"
        "\"A lantern crosses a\n"
        "lavender forest at night.\"\n";

    assert(wasm_load_csv(csv_text) == 1);
    assert(wasm_is_ready());
    assert(wasm_movie_count() == 1);

    const char *json = wasm_search("lavender forest", "", "", 0, 0, 0, 20);
    assert(json && std::strstr(json, "Forest Signal"));

    std::cout << "OK: WASM bridge loads multiline CSV and searches plot text\n";
    return 0;
}
