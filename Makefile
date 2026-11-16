CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17
LDFLAGS = -lcurl

FETCH_BIN = data/fetchData
DATA_FILE = data/data.csv

# Programa principal
programa: main.cpp $(DATA_FILE)
	$(CXX) $(CXXFLAGS) main.cpp -o programa

# Descargar los datos si no existen
$(DATA_FILE): $(FETCH_BIN)
	./$(FETCH_BIN)

# Compilar la herramienta de descarga
$(FETCH_BIN): data/fetchData.cpp
	$(CXX) $(CXXFLAGS) data/fetchData.cpp -o $(FETCH_BIN) $(LDFLAGS)

run: programa
	./programa

cli: src/cli.cpp $(DATA_FILE)
	$(CXX) $(CXXFLAGS) src/cli.cpp -o cli

run-cli: cli
	./cli

clean:
	rm -f programa cli $(FETCH_BIN) $(DATA_FILE)

# Preprocesamiento independiente del programa principal y de la descarga.
PREPROCESS_BIN = build/cleanData
INPUT ?= data/data.csv
OUTPUT ?= data/data_clean.csv
REPORT ?= build/preprocessing_report.json

$(PREPROCESS_BIN): src/preprocessing/cleanData.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@

preprocess: $(PREPROCESS_BIN)
	./$(PREPROCESS_BIN) "$(INPUT)" "$(OUTPUT)" "$(REPORT)"

# Prepares the CSV in the public directory served by Vite and packaged by Pages.
# The original and cleaned datasets remain in data/ as the canonical pipeline outputs.
web-data:
	@if [ ! -s "data/data_clean.csv" ]; then \
		$(MAKE) "$(DATA_FILE)" && \
		$(MAKE) preprocess INPUT="$(DATA_FILE)" OUTPUT="data/data_clean.csv"; \
	fi
	mkdir -p web/public
	cp data/data_clean.csv web/public/data_clean.csv

web-dev:
	$(MAKE) web-data
	$(MAKE) wasm
	cd web && pnpm exec vite --host 0.0.0.0

build/test_preprocessing: tests/test_preprocessing.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@

test-preprocess: $(PREPROCESS_BIN) build/test_preprocessing
	./build/test_preprocessing ./$(PREPROCESS_BIN)

build/test_search: tests/test_search.cpp tests/MockMovieLoader.cpp \
		src/algorithm/loaders/CsvParser.h \
		src/search/searchEngine.cpp src/algorithm/hashing/MovieHashIndex.cpp \
		src/algorithm/trees/MovieSuffixArray.cpp src/algorithm/trees/MovieTree.cpp \
		src/algorithm/tagger.cpp src/search/ranking.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@

test-search: build/test_search
	./build/test_search

build/test_wasm_bridge: tests/test_wasm_bridge.cpp wasm_bridge.cpp \
		src/algorithm/loaders/CsvParser.h src/search/searchEngine.cpp \
		src/algorithm/hashing/MovieHashIndex.cpp \
		src/algorithm/trees/MovieSuffixArray.cpp src/algorithm/trees/MovieTree.cpp \
		src/algorithm/tagger.cpp src/search/ranking.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@

test-wasm-bridge: build/test_wasm_bridge
	./build/test_wasm_bridge

# WebAssembly
EMCC ?= emcc
EM_CACHE_DIR ?= $(CURDIR)/build/emscripten-cache
EMFLAGS ?= -O3 -std=c++17 -fexceptions \
	-s WASM=1 \
	-s ASSERTIONS=1 \
	-s INITIAL_MEMORY=134217728 \
	-s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","UTF8ToString","stringToUTF8","lengthBytesUTF8"]' \
	-s EXPORTED_FUNCTIONS='["_wasm_load_csv","_wasm_is_ready","_wasm_movie_count","_wasm_search","_wasm_get_movie","_wasm_get_genres","_wasm_get_featured","_wasm_browse_genre","_wasm_get_user_data_json","_wasm_load_user_data_json","_wasm_add_watch_later","_wasm_remove_watch_later","_wasm_has_watch_later","_wasm_toggle_watch_later","_wasm_add_liked","_wasm_remove_liked","_wasm_has_liked","_wasm_toggle_liked","_wasm_clear_user_data","_malloc","_free"]' \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s MODULARIZE=1 \
	-s EXPORT_NAME='createStreamingModule' \
	-s ENVIRONMENT='web'

wasm: wasm_bridge.cpp src/algorithm/loaders/CsvParser.h src/search/searchEngine.cpp \
		src/algorithm/hashing/MovieHashIndex.cpp \
		src/algorithm/trees/MovieSuffixArray.cpp src/algorithm/trees/MovieTree.cpp \
		src/algorithm/tagger.cpp src/search/ranking.cpp
	mkdir -p web/public "$(EM_CACHE_DIR)"
	EM_CACHE="$(EM_CACHE_DIR)" $(EMCC) $(EMFLAGS) wasm_bridge.cpp -o web/public/streaming.js

.PHONY: run clean preprocess web-data web-dev test-preprocess test-search test-wasm-bridge wasm
