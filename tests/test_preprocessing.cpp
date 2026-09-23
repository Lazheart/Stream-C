#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
using namespace std;
namespace fs = filesystem;

const string header = "Release Year,Title,Origin/Ethnicity,Director,Cast,Genre,Wiki Page,Plot\n";
const string movie = "2000,Pelicula,Peru,Ana,Actor,Drama,https://example.org/a,Trama\n";
fs::path folder;
string program;
int passed = 0;

void check(bool condition, const string& message) {
    if (!condition) throw runtime_error(message);
}
string readFile(const fs::path& path) {
    ifstream file(path, ios::binary);
    check(bool(file), "No se pudo leer " + path.string());
    return string(istreambuf_iterator<char>(file), istreambuf_iterator<char>());
}
// Protege rutas con espacios o apostrofes al ejecutar en macOS/Linux.
string shellQuote(const string& value) {
    string result = "'";
    for (char c : value) result += c == '\'' ? "'\"'\"'" : string(1, c);
    return result + "'";
}
int execute(const string& destination = "clean.csv", const string& report = "report.json") {
    string command = shellQuote(program) + " " + shellQuote((folder / "input.csv").string())
        + " " + shellQuote((folder / destination).string())
        + " " + shellQuote((folder / report).string())
        + " > " + shellQuote((folder / "log.txt").string()) + " 2>&1";
    return system(command.c_str());
}
int run(const string& content) {
    for (const string name : {"clean.csv", "report.json"}) fs::remove(folder / name);
    ofstream input(folder / "input.csv", ios::binary);
    input << content;
    input.close();
    check(bool(input), "Error creando entrada de prueba");
    int result = execute();
    check(readFile(folder / "input.csv") == content, "El original fue modificado");
    return result;
}
void reportContains(const string& expected) {
    check(readFile(folder / "report.json").find(expected) != string::npos,
          "Falta en el reporte: " + expected);
}
void pass(const string& name) { cout << "OK: " << name << '\n'; ++passed; }

int main(int argc, char** argv) {
    if (argc != 2) { cerr << "Uso: test_preprocessing ruta_al_limpiador\n"; return 1; }
    program = fs::absolute(argv[1]).string();
    folder = fs::temp_directory_path() / ("stream-c-tests-" +
        to_string(chrono::steady_clock::now().time_since_epoch().count()));
    try {
        check(fs::create_directory(folder), "No se pudo crear carpeta temporal");
        // 1. Texto complicado: comas, comillas, Unicode y saltos internos.
        string complex = "2000,\"El \"\"barco\"\", fantasma\",Perú,Ana,Actor,Drama,url,\"Primera línea\r\nSegunda línea\n日本語, \"\"sí\"\"\"";
        check(run(string("\xEF\xBB\xBF") + header + complex) == 0, "Fallo con texto complejo");
        check(readFile(folder / "clean.csv") == header + complex + "\n", "Se altero el texto");
        pass("comillas, multilinea, Unicode, BOM y final sin salto");

        // 2. Compara toda la salida con el resultado esperado, no solo contadores.
        string empty = "2000,Pelicula,Peru,\xC2\xA0, \t\xE2\x80\x83,,url,Trama\n";
        string fallback = "2000,,Peru,Ana,Actor,Drama,url,Trama\n";
        string extra = "2000,Otra,Peru,Ana,Actor,Drama,url,Trama,extra,extra2\n";
        string remake = "2020,Pelicula,Peru,Ana,Actor,Drama,https://example.org/a,Trama\n";
        check(run(header + movie + movie + empty + "2000,,Peru,Ana,Actor,Drama,,Trama\n"
                  + "2000,incompleta\n" + fallback + extra + remake) == 0, "Fallo en reglas");
        string expected = header + movie + "2000,Pelicula,Peru,unknown,unknown,unknown,url,Trama\n"
            + "2000,unknown,Peru,Ana,Actor,Drama,url,Trama\n"
            + "2000,Otra,Peru,Ana,Actor,Drama,url,Trama\n" + remake;
        check(readFile(folder / "clean.csv") == expected, "Limpieza incorrecta");
        for (const string field : {"\"registros_leidos\": 8", "\"registros_conservados\": 5",
             "\"registros_descartados\": 3", "\"duplicados\": 1", "\"columnas_extra_omitidas\": 2",
             "\"Cast\": 1", "\"registro\": 2,", "\"registro\": 4,", "\"registro\": 5,"}) reportContains(field);
        pass("vacios, duplicados, descartes, extras y titulos repetidos");

        // 3. Codificacion invalida.
        check(run(header + "2000,Pelicula,Peru,Ana," + string(1, char(0xFF)) + ",Drama,url,Trama\n") == 0,
              "Fallo al descartar UTF-8 invalido");
        reportContains("utf8_invalido_o_nul");
        check(readFile(folder / "clean.csv") == header, "Se conservo UTF-8 invalido");
        pass("UTF-8 invalido registrado como descarte");

        // 4. Nunca publicar una salida parcial si las comillas estan rotas.
        for (const string tail : {"2000,\"unclosed", "2000,ba\"d,x", "2000,\"closed\"oops,x"}) {
            check(run(header + movie + tail) != 0, "Se aceptaron comillas rotas");
            for (const string name : {"clean.csv", "report.json", "clean.csv.tmp", "report.json.tmp"})
                check(!fs::exists(folder / name), "Quedo un archivo parcial");
        }
        pass("comillas rotas sin archivos parciales");

        // 5. Cabecera incompatible.
        check(run("Title,Plot\na,b\n") != 0, "Se acepto una cabecera incorrecta");
        check(!fs::exists(folder / "clean.csv"), "Se genero salida con cabecera incorrecta");
        pass("cabecera incorrecta rechazada");

        // 6. Proteccion del original y de los resultados existentes.
        check(run(header + movie) == 0, "Fallo creando resultado");
        string previous = readFile(folder / "clean.csv");
        check(execute("clean.csv", "new-report.json") != 0, "Se sobrescribio el resultado");
        check(execute("input.csv", "new-report.json") != 0, "Se sobrescribio el original");
        check(readFile(folder / "clean.csv") == previous, "Cambio el resultado anterior");
        check(readFile(folder / "input.csv") == header + movie, "Cambio el original");
        pass("proteccion de originales y resultados existentes");

        // 7. Un archivo con cabecera y sin peliculas es valido.
        check(run(header) == 0, "Fallo con dataset sin peliculas");
        reportContains("\"registros_leidos\": 0");
        check(readFile(folder / "clean.csv") == header, "Salida vacia incorrecta");
        pass("dataset sin registros");
        fs::remove_all(folder);
        cout << passed << " pruebas aprobadas.\n";
    } catch (const exception& error) {
        cerr << "FALLO: " << error.what() << "\nArchivos para revisar: " << folder << '\n';
        return 1;
    }
}
