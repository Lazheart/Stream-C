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

clean:
	rm -f programa $(FETCH_BIN) $(DATA_FILE)

.PHONY: run clean

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

build/test_preprocessing: tests/test_preprocessing.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@

test-preprocess: $(PREPROCESS_BIN) build/test_preprocessing
	./build/test_preprocessing ./$(PREPROCESS_BIN)

.PHONY: preprocess test-preprocess
