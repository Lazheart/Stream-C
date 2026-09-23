#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

// fs abrevia filesystem, que permite comprobar y renombrar archivos.
namespace fs = filesystem;
// Una fila es un vector con los campos de una pelicula.
using Row = vector<string>;
const array<string, 8> columns = {
    "Release Year", "Title", "Origin/Ethnicity", "Director",
    "Cast", "Genre", "Wiki Page", "Plot"};

// Una fila logica puede contener varios saltos de linea entre comillas.
class CsvReader {
    istream& input;
public:
    size_t line = 1;
    explicit CsvReader(istream& stream) : input(stream) {}

    bool read(Row& row) {
        enum class State { Start, Plain, Quoted, Closed };
        State state = State::Start;
        row.clear();
        string field;
        bool hasData = false;
        char c;
        while (input.get(c)) {
            hasData = true;
            if (c == '\n') ++line;
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
                if (c == '\r') {
                    if (input.peek() == '\n') input.get();
                    ++line;
                }
                row.push_back(field);
                return true;
            } else if (c == '"' && state == State::Start) {
                state = State::Quoted;
            } else {
                if (c == '"' || state == State::Closed)
                    throw runtime_error("CSV: comillas invalidas cerca de linea " + to_string(line));
                field += c;
                state = State::Plain;
            }
        }
        if (input.bad()) throw runtime_error("Error de lectura del CSV");
        if (state == State::Quoted)
            throw runtime_error("CSV: campo entre comillas sin cerrar al final del archivo");
        if (!hasData) return false;
        row.push_back(field);
        return true;
    }
};

bool blank(const string& s) {
    // Incluye espacios Unicode, por ejemplo NBSP (U+00A0) presente en el dataset.
    for (size_t i = 0; i < s.size();) {
        auto c = static_cast<unsigned char>(s[i++]);
        uint32_t codePoint = c;
        int remainingBytes = 0;
        if (c >= 0xC2 && c <= 0xDF) { codePoint = c & 0x1F; remainingBytes = 1; }
        else if (c >= 0xE0 && c <= 0xEF) { codePoint = c & 0x0F; remainingBytes = 2; }
        else if (c >= 0xF0 && c <= 0xF4) { codePoint = c & 7; remainingBytes = 3; }
        else if (c >= 0x80) return false;
        while (remainingBytes--) {
            if (i == s.size()) return false;
            auto next = static_cast<unsigned char>(s[i++]);
            if ((next & 0xC0) != 0x80) return false;
            codePoint = (codePoint << 6) | (next & 0x3F);
        }
        if (!(codePoint == 0x20 || (codePoint >= 9 && codePoint <= 13) || codePoint == 0x85 || codePoint == 0xA0 ||
              codePoint == 0x1680 || (codePoint >= 0x2000 && codePoint <= 0x200A) || codePoint == 0x2028 ||
              codePoint == 0x2029 || codePoint == 0x202F || codePoint == 0x205F || codePoint == 0x3000)) return false;
    }
    return true;
}

// Validacion UTF-8 sin alterar tildes, mayusculas ni caracteres de otros idiomas.
bool validUtf8(const string& s) {
    for (size_t i = 0; i < s.size();) {
        const auto c = static_cast<unsigned char>(s[i++]);
        if (c < 0x80) { if (c == 0) return false; continue; }
        int remainingBytes;
        uint32_t codePoint, minimum;
        if (c >= 0xC2 && c <= 0xDF) { remainingBytes = 1; codePoint = c & 0x1F; minimum = 0x80; }
        else if (c >= 0xE0 && c <= 0xEF) { remainingBytes = 2; codePoint = c & 0x0F; minimum = 0x800; }
        else if (c >= 0xF0 && c <= 0xF4) { remainingBytes = 3; codePoint = c & 7; minimum = 0x10000; }
        else return false;
        while (remainingBytes--) {
            if (i == s.size()) return false;
            const auto next = static_cast<unsigned char>(s[i++]);
            if ((next & 0xC0) != 0x80) return false;
            codePoint = (codePoint << 6) | (next & 0x3F);
        }
        if (codePoint < minimum || codePoint > 0x10FFFF || (codePoint >= 0xD800 && codePoint <= 0xDFFF)) return false;
    }
    return true;
}

string serialize(const Row& row) {
    string result;
    for (size_t i = 0; i < row.size(); ++i) {
        if (i) result += ',';
        const auto& field = row[i];
        const bool quote = field.find_first_of(",\"\r\n") != string::npos;
        if (quote) result += '"';
        for (char c : field) {
            result += c;
            // En CSV, una comilla dentro del texto se escribe dos veces.
            if (c == '"') {
                result += '"';
            }
        }
        if (quote) result += '"';
    }
    return result + '\n';
}

struct Rejection { size_t record, line; string reason; };

int main(int argc, char** argv) {
    if (argc != 4) {
        cerr << "Uso: " << argv[0] << " entrada.csv salida.csv reporte.json\n";
        return 1;
    }
    fs::path output = argv[2], report = argv[3];
    fs::path outputTmp = output.string() + ".tmp", reportTmp = report.string() + ".tmp";
    bool ownOutputTmp = false, ownReportTmp = false;
    try {
        // Nunca sobreescribir originales, resultados previos ni temporales ajenos.
        unordered_set<string> paths;
        for (const auto& p : {fs::path(argv[1]), output, report, outputTmp, reportTmp})
            if (!paths.insert(fs::weakly_canonical(p).string()).second)
                throw runtime_error("Las rutas de entrada, salida, reporte y temporales deben ser distintas");
        for (const auto& p : {output, report, outputTmp, reportTmp})
            if (fs::exists(p)) throw runtime_error("La ruta ya existe: " + p.string());
        ifstream input(argv[1], ios::binary);
        if (!input) throw runtime_error("No se pudo abrir el CSV original");
        // BOM UTF-8 opcional; el resto de la codificacion se valida por registro.
        char bom[3];
        input.read(bom, 3);
        if (input.gcount() != 3 || string(bom, 3) != "\xEF\xBB\xBF") {
            input.clear(); input.seekg(0);
        }
        CsvReader reader(input);
        Row row;
        if (!reader.read(row) || row != Row(columns.begin(), columns.end()))
            throw runtime_error("Cabecera inesperada: se requieren las ocho columnas acordadas, en orden");
        ofstream out(outputTmp, ios::binary);
        if (!out) throw runtime_error("No se pudo crear el CSV de salida; compruebe su carpeta");
        ownOutputTmp = true;
        out << serialize(row);
        size_t total = 0, accepted = 0, duplicates = 0, extraRows = 0, extraFields = 0;
        array<size_t, 8> replacements{};
        unordered_set<string> uniqueRows;
        vector<Rejection> rejected;
        while (true) {
            const auto firstLine = reader.line;
            if (!reader.read(row)) break;
            ++total;
            string reason;
            if (row.size() < 8) reason = "menos_de_ocho_columnas";
            else {
                for (const auto& value : row)
                    if (!validUtf8(value)) { reason = "utf8_invalido_o_nul"; break; }
                if (reason.empty() && blank(row[1]) && blank(row[6]))
                    reason = "sin_titulo_ni_enlace";
            }
            if (!reason.empty()) { rejected.push_back({total, firstLine, reason}); continue; }
            if (row.size() > 8) { ++extraRows; extraFields += row.size() - 8; row.resize(8); }
            array<bool, 8> replaced{};
            for (size_t i = 0; i < 8; ++i)
                if (blank(row[i])) {
                    row[i] = "unknown";
                    replaced[i] = true;
                }
            const auto csvLine = serialize(row);
            if (uniqueRows.count(csvLine) > 0) {
                ++duplicates;
                rejected.push_back({total, firstLine, "duplicado_exacto_tras_limpieza"});
                continue;
            }
            uniqueRows.insert(csvLine);
            for (size_t i = 0; i < 8; ++i) {
                if (replaced[i]) {
                    ++replacements[i];
                }
            }
            out << csvLine;
            ++accepted;
        }
        out.close();
        if (!out) throw runtime_error("Error escribiendo el CSV limpio");
        ofstream reportFile(reportTmp);
        if (!reportFile) throw runtime_error("No se pudo crear el reporte; compruebe su carpeta");
        ownReportTmp = true;
        reportFile << "{\n  \"registros_leidos\": " << total
              << ",\n  \"registros_conservados\": " << accepted
              << ",\n  \"registros_descartados\": " << rejected.size()
              << ",\n  \"duplicados\": " << duplicates
              << ",\n  \"filas_con_extras\": " << extraRows
              << ",\n  \"columnas_extra_omitidas\": " << extraFields
              << ",\n  \"vacios_reemplazados_en_salida\": {";
        for (size_t i = 0; i < 8; ++i)
            reportFile << (i ? "," : "") << "\n    \"" << columns[i] << "\": " << replacements[i];
        reportFile << "\n  },\n  \"descartes\": [";
        for (size_t i = 0; i < rejected.size(); ++i) {
            const auto& r = rejected[i];
            reportFile << (i ? "," : "") << "\n    {\"registro\": " << r.record
                  << ", \"linea_inicio\": " << r.line << ", \"motivo\": \"" << r.reason << "\"}";
        }
        reportFile << "\n  ]\n}\n";
        reportFile.close();
        if (!reportFile) throw runtime_error("Error escribiendo el reporte");
        fs::rename(reportTmp, report);
        ownReportTmp = false;
        try { fs::rename(outputTmp, output); }
        catch (...) { fs::remove(report); throw; }
        ownOutputTmp = false;
        cout << "Leidos: " << total << " | Conservados: " << accepted
                  << " | Descartados: " << rejected.size() << '\n';
    } catch (const exception& e) {
        error_code ignored;
        if (ownOutputTmp) fs::remove(outputTmp, ignored);
        if (ownReportTmp) fs::remove(reportTmp, ignored);
        cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
