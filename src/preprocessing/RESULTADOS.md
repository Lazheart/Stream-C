# Resultado de la validación del dataset

Entrada: `wiki_movie_plots_deduped.csv`, proporcionado por el usuario.
No se verificó que sea idéntico al archivo de los enlaces externos del enunciado.

| Comprobación | Resultado |
| --- | ---: |
| Registros leídos | 34.886 |
| Registros conservados | 34.886 |
| Registros descartados | 0 |
| Duplicados de las ocho columnas limpias | 0 |
| Filas con columnas adicionales | 0 |
| Vacíos corregidos en Cast | 1.454 |
| Vacíos corregidos en Genre | 28 |
| Vacíos corregidos en Director | 8 |
| Total de campos corregidos | 1.490 |
| Sinopsis con saltos de línea internos conservadas | 23.425 |

De los vacíos, 32 de Cast y 8 de Director contenían U+00A0, un espacio Unicode
visualmente vacío. Se sustituyeron por `unknown` igual que los vacíos ordinarios.

Validación realizada:

1. Compilación C++17 con `-Wall -Wextra`, sin advertencias.
2. Siete pruebas automatizadas de caja negra en C++ aprobadas (`make test-preprocess`).
3. Verificación adicional realizada durante el desarrollo (no requerida para ejecutar
   el proyecto): lectura independiente del original y del resultado con el módulo CSV de Python:
   comparación de las 34.886 filas y sus ocho campos; cada campo no vacío coincide
   exactamente con el original y los vacíos se convierten a `unknown`.
4. Segunda ejecución del limpiador sobre el CSV limpio: salida idéntica byte a byte,
   sin nuevos reemplazos ni descartes (idempotencia).

SHA-256 del original:

```text
e5450142ff22420ec2229ab1ee5a567e771f001fc04ac9d7960b533c88e5993c
```

SHA-256 de `data/data_clean.csv`:

```text
c89fec79f497bf4418e1d6d82bbe95b2619faef1f237fe81ee58da8ccd0d9463
```

Los CSV y `build/` están excluidos de Git por las reglas existentes. El equipo
puede reproducir el resultado con la misma entrada y los comandos del README de
este módulo, o recibir el CSV generado como archivo por separado.
