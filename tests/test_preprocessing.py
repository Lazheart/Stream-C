"""Pruebas de caja negra. El procesamiento productivo es exclusivamente C++."""
import csv
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

BIN = str(Path(sys.argv.pop(1)).resolve())
HEADER = ['Release Year', 'Title', 'Origin/Ethnicity', 'Director',
          'Cast', 'Genre', 'Wiki Page', 'Plot']
BASE = ['2000', 'Película', 'Perú', 'Ana', 'Actor', 'Drama', 'https://example.org/a', 'Trama']


def encode(rows):
    s = io.StringIO(newline='')
    csv.writer(s).writerows([HEADER] + rows)
    return s.getvalue().encode('utf-8')


class PreprocessingTests(unittest.TestCase):
    def run_clean(self, content):
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        root = Path(folder.name)
        source, output, report = (root / n for n in ['source.csv', 'clean.csv', 'report.json'])
        source.write_bytes(content)
        result = subprocess.run([BIN, str(source), str(output), str(report)], capture_output=True)
        self.assertEqual(source.read_bytes(), content, 'El original debe conservarse')
        return result, source, output, report

    def test_round_trip_multiline_quotes_unicode_and_bom(self):
        row = BASE.copy()
        row[1] = 'El "barco", fantasma'
        row[7] = 'Primera línea\r\nSegunda línea\n日本語, "sí"'
        result, _, out, report = self.run_clean(b'\xef\xbb\xbf' + encode([row]).rstrip(b'\r\n'))
        self.assertEqual(result.returncode, 0, result.stderr)
        with out.open(encoding='utf-8', newline='') as f:
            self.assertEqual(list(csv.reader(f)), [HEADER, row])
        self.assertEqual(json.loads(report.read_text())['registros_conservados'], 1)

    def test_rules_and_auditable_counts(self):
        empty = BASE.copy(); empty[3:6] = ['\u00a0', ' \t\u2003', '']
        missing = BASE.copy(); missing[1] = missing[6] = ''
        fallback = BASE.copy(); fallback[1] = ''
        extra = BASE.copy(); extra[1] = 'Otra'; extra += ['extra', 'extra2']
        remake = BASE.copy(); remake[0] = '2020'
        rows = [BASE, BASE, empty, missing, ['2000', 'incompleta'], fallback, extra, remake]
        result, _, out, report = self.run_clean(encode(rows))
        self.assertEqual(result.returncode, 0, result.stderr)
        stats = json.loads(report.read_text())
        self.assertEqual((stats['registros_leidos'], stats['registros_conservados'],
                          stats['registros_descartados'], stats['duplicados']), (8, 5, 3, 1))
        self.assertEqual(stats['columnas_extra_omitidas'], 2)
        self.assertEqual(stats['vacios_reemplazados_en_salida']['Cast'], 1)
        self.assertEqual([d['registro'] for d in stats['descartes']], [2, 4, 5])
        with out.open(newline='') as f:
            cleaned = list(csv.reader(f))[1:]
        self.assertTrue(all(len(r) == 8 for r in cleaned))
        self.assertEqual(cleaned[1][3:6], ['unknown'] * 3)
        self.assertEqual(cleaned[2][1], 'unknown')
        self.assertEqual(cleaned[-1][0], '2020')

    def test_invalid_utf8_is_reported(self):
        result, _, _, report = self.run_clean(encode([BASE]).replace(b'Actor', b'\xff'))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(report.read_text())['descartes'][0]['motivo'], 'utf8_invalido_o_nul')

    def test_broken_quotes_abort_without_partial_output(self):
        for tail in [b'2000,"unclosed', b'2000,ba"d,x', b'2000,"closed"oops,x']:
            with self.subTest(tail=tail):
                result, _, out, report = self.run_clean(encode([BASE]) + tail)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse(out.exists())
                self.assertFalse(report.exists())
                self.assertFalse(Path(str(out) + '.tmp').exists())

    def test_wrong_header_aborts(self):
        result, _, out, _ = self.run_clean(b'Title,Plot\na,b\n')
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(out.exists())

    def test_existing_output_and_original_are_protected(self):
        result, source, out, report = self.run_clean(encode([BASE]))
        self.assertEqual(result.returncode, 0)
        before = out.read_bytes()
        for dest in [out, source]:
            again = subprocess.run([BIN, str(source), str(dest), str(report)], capture_output=True)
            self.assertNotEqual(again.returncode, 0)
        self.assertEqual(out.read_bytes(), before)
        self.assertEqual(source.read_bytes(), encode([BASE]))

    def test_header_only(self):
        result, _, _, report = self.run_clean(encode([]))
        self.assertEqual(result.returncode, 0)
        self.assertEqual(json.loads(report.read_text())['registros_leidos'], 0)


if __name__ == '__main__':
    unittest.main()
