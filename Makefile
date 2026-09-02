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
