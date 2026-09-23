# Preprocesamiento del CSV

Responsable: luv-amori (sir-alexng). Este módulo convierte el CSV original en
`data/data_clean.csv`, que recibe el módulo de carga e indexación de Brandon.
La limpieza y la lectura productiva están implementadas en C++17.

## Ejecutar

Desde la raíz del repositorio, con un compilador C++17 y Make:

```sh
make preprocess INPUT="/ruta/wiki_movie_plots_deduped.csv"
```

Genera `data/data_clean.csv` y `build/preprocessing_report.json`.
El original no se modifica. El programa rechaza rutas de salida existentes para
evitar sobrescribir datos. Para repetir una ejecución se pueden elegir rutas nuevas:

```sh
make preprocess INPUT="/ruta/wiki_movie_plots_deduped.csv" OUTPUT="data/segunda_limpieza.csv" REPORT="build/segundo_reporte.json"
```

Las carpetas de salida deben existir. Make crea `build`; `data` ya existe en el
repositorio. El programa también puede invocarse directamente:

```sh
./build/cleanData entrada.csv salida.csv reporte.json
make test-preprocess
```

Las pruebas usan Python 3 y su biblioteca estándar como lector independiente;
Python no forma parte de la aplicación ni de su proceso de limpieza.

## Contrato para el módulo de carga

CSV UTF-8 sin BOM, con cabecera y exactamente estas ocho columnas en este orden:

```text
Release Year,Title,Origin/Ethnicity,Director,Cast,Genre,Wiki Page,Plot
```

Se escriben separadores de registro LF. Los saltos internos de las sinopsis se
conservan, incluso CRLF. Un campo con comas, comillas o saltos de línea se encierra
entre comillas; cada comilla interna se representa como `""`. No es correcto leer
una película por línea física ni separar sus atributos solamente con `split(',')`.

Los campos ausentes se representan con `unknown`, incluido un año vacío. Brandon
debe manejar ese marcador antes de convertir el año a un número. No se agregan
IDs ni tags: pertenecen a la carga e indexación. El número de registro del reporte
empieza en 1 después de la cabecera; `linea_inicio` incluye la cabecera.

## Decisiones de limpieza

- Se exige la cabecera acordada para evitar interpretar columnas en otro orden.
- Se acepta BOM UTF-8 al inicio. Se valida UTF-8 y se rechazan filas con bytes
  inválidos o NUL; no se adivina una codificación alternativa.
- Los campos vacíos o formados solo por espacios Unicode se convierten a
  `unknown`. Incluye el espacio no separable U+00A0 encontrado en el dataset.
- Se omiten registros con menos de ocho campos o sin título y sin enlace. Si
  falta únicamente el título, se conserva el registro con su enlace.
- Se omiten columnas adicionales después de la octava, según `ANALYSIS.md`.
- Se elimina una repetición solo si sus ocho campos resultantes son exactamente
  iguales a los de un registro ya conservado. Se conserva la primera aparición.
  Compartir título no basta para considerar dos películas duplicadas.
- Se conservan mayúsculas, tildes, espacios en textos no vacíos y sinopsis completas.
  El PDF de reparto propone normalización para búsqueda; el análisis del repositorio
  exige conservar información válida. Esta implementación conserva el contenido de
  presentación. La normalización de claves de búsqueda queda como punto de
  coordinación con algoritmos, sin cambiar las ocho columnas acordadas.
- No se infieren géneros, directores ni años faltantes, ni se traduce el dataset.

Una fila estructuralmente interpretable puede descartarse con un motivo en el
reporte. Comillas rotas, en cambio, pueden hacer ambiguo dónde termina la fila:
en ese caso se aborta con código 1 y se eliminan los temporales propios, sin
publicar un CSV parcial. El éxito devuelve código 0. El programa escribe primero
temporales; CSV y reporte se publican al terminar. No se garantiza una transacción
conjunta frente a una interrupción del sistema entre los dos renombrados.

## Reporte y verificación

El JSON contiene registros leídos, conservados, descartados, duplicados, extras,
reemplazos por columna y cada descarte con ubicación y motivo. Los duplicados
están incluidos en el total de descartes; los reemplazos cuentan solo las filas
conservadas. Los extras se cuentan en filas estructuralmente válidas, incluso si
después se descartan por duplicadas.

Las pruebas cubren campos multilínea, comillas escapadas, comas internas, Unicode,
BOM, fin de archivo sin salto, vacíos Unicode, columnas extra, filas incompletas,
duplicados, títulos repetidos de años distintos, UTF-8 inválido, comillas rotas,
cabeceras incorrectas, archivos existentes y datasets sin registros.

El parser recorre los bytes una vez y usa un conjunto hash de las filas limpias
para detectar duplicados. El tiempo esperado y la memoria usada por el conjunto
son proporcionales al tamaño del dataset; el archivo no necesita cargarse entero
antes de comenzar. Para datasets mucho mayores habría que revisar el coste de
conservar las claves completas en memoria.

Consulte `RESULTADOS.md` para la ejecución sobre el dataset entregado.
