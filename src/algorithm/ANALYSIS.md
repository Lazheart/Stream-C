# Análisis de Algoritmos

En esta carpeta se encuentra el código fuente de los algoritmos utilizados por el programa para procesar y realizar búsquedas sobre los datos de películas provenientes de `data_clean.csv`, generado durante la etapa de **preprocessing**.

El procesamiento se centra principalmente en los campos de **título, sinopsis y descripción** de cada película. Adicionalmente, se incluye el módulo `tags`, encargado de clasificar y organizar la información mediante diferentes atributos, como:

* Director
* Género
* Año
* País

El objetivo es proporcionar diferentes mecanismos de indexación y búsqueda que permitan al usuario filtrar el conjunto de películas y obtener información más específica de acuerdo con sus criterios.

## Estructuras y algoritmos

### Hash Map
### Suffix Tree
### Tree

## Tags


## Consideraciones

La selección de estas estructuras de datos responde a la naturaleza del dataset y a los tipos de consultas que se desean realizar.

Cada estructura cumple una función diferente:

| Estructura    | Propósito                                    |
| ------------- | -------------------------------------------- |
| `Hash Map`    | Acceso rápido mediante claves y atributos    |
| `Suffix Tree` | Búsqueda eficiente sobre texto               |
| `Tree`        | Organización y clasificación de información  |
| `Tags`        | Filtrado mediante atributos de las películas |

La implementación de estos algoritmos constituye la base del sistema de procesamiento de datos y complementa el módulo `search`, donde estas estructuras serán utilizadas para construir un mecanismo de búsqueda más completo.

## Relación con el módulo `search`

El módulo `search` utilizará las estructuras desarrolladas en esta carpeta para procesar las consultas realizadas por el usuario.

De esta manera, el sistema podrá combinar:

1. Búsqueda por texto.
2. Búsqueda por atributos.
3. Filtrado mediante tags.
4. Combinación de diferentes criterios de búsqueda.

El objetivo final es obtener un sistema de búsqueda eficiente y adaptable a las características del dataset, evitando depender únicamente de una búsqueda secuencial sobre todo el archivo CSV.
