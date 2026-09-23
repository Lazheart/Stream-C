# Análisis y Preprocesamiento de Datos

La implementación y sus decisiones están descritas en [README.md](README.md).
La validación del dataset entregado está registrada en [RESULTADOS.md](RESULTADOS.md).

Los datos utilizados en este proyecto corresponden a una base de datos pública de películas, proporcionada como archivo **CSV**. La base de datos original puede ser descargada desde el siguiente enlace:

[Base de datos de películas](https://drive.google.com/file/d/1Gae6uXFvvu5FbVNw3H2l69OpByFALqBd/)

El preprocesamiento de los datos es responsabilidad del grupo, de acuerdo con los requisitos establecidos para el proyecto.

## Estructura de los datos

Cada registro de la base de datos representa una película y está compuesto por los siguientes atributos:

| Atributo           | Descripción                                                                  |
| ------------------ | ---------------------------------------------------------------------------- |
| `Release Year`     | Año de lanzamiento de la película. Se representa mediante un valor numérico. |
| `Title`            | Título de la película.                                                       |
| `Origin/Ethnicity` | País, región o grupo de origen asociado a la película.                       |
| `Director`         | Director o grupo responsable de la dirección de la película.                 |
| `Cast`             | Actores y participantes del reparto.                                         |
| `Genre`            | Género o géneros correspondientes a la película.                             |
| `Wiki Page`        | Enlace a la página de Wikipedia asociada a la película.                      |
| `Plot`             | Sinopsis o descripción de la trama de la película.                           |

Estos campos constituyen la información principal utilizada posteriormente por el sistema para realizar búsquedas y mostrar información de las películas.

## Reglas de preprocesamiento

Para garantizar que los datos puedan ser utilizados correctamente por el programa, se aplicarán las siguientes reglas durante el proceso de limpieza.

### 1. Valores desconocidos y en blanco

Los valores desconocidos, ausentes o campos en blanco (por ejemplo, valores vacíos entre comas `,,`) dentro de la base de datos serán reemplazados por:

```text
unknown
```

De esta manera, se mantiene una representación uniforme para los atributos cuyo contenido original no se encuentre disponible o esté en blanco.

### 2. Separación de atributos

Los registros utilizan **comas como separadores de atributos**.

Un registro válido debe contener los campos correspondientes a:

```text
Release Year,
Title,
Origin/Ethnicity,
Director,
Cast,
Genre,
Wiki Page,
Plot
```

Los datos que contengan comas dentro del contenido de un atributo deberán ser interpretados de acuerdo con la estructura del formato CSV, evitando que dichas comas sean consideradas como separadores adicionales.

### 3. Registros inválidos y omisión de datos

Cualquier registro que no pueda ser interpretado correctamente de acuerdo con la estructura definida será considerado inválido y será **descartado (omitido) del conjunto de datos procesado**.

Esto incluye:
- Registros que presenten una estructura incompatible, campos imposibles de identificar correctamente o una cantidad de atributos no resoluble.
- **Validación de campos indispensables**:
  - El **título** (`Title`) es el campo principal que no debe faltar.
  - En caso de que **falte el título**, como requisito mínimo indispensable el registro **debe contar con el enlace a Wikipedia (`Wiki Page`)**.
  - Si un registro carece de título y tampoco tiene enlace a Wikipedia, el dato será **omitido / descartado**.

### 4. Atributos adicionales y valores en blanco

Si un registro contiene información adicional que no pertenece a los ocho atributos definidos anteriormente, esta información será omitida. Asimismo, los valores vacíos dentro de los ocho atributos serán reemplazados por `unknown`.

Por ejemplo, si un registro presenta una estructura similar a:

```text
2000,1984,,George Orwell,"Mary Pickford, Mack Sennett",Terror,https://es.wikipedia.org/...,La novela 1984...,5
```

En este caso:
1. El valor en blanco entre el título (`1984`) y el director (`George Orwell`) corresponde al origen/etnicidad (`Origin/Ethnicity`). Al estar vacío (`,,`), será reemplazado por `unknown`.
2. El último valor (`5`) corresponde a un atributo adicional que no forma parte de la estructura utilizada por este proyecto. Por lo tanto, dicho valor será **ignorado durante el preprocesamiento**.

El registro procesado conservará únicamente los atributos correspondientes al modelo definido:

```text
2000,1984,unknown,George Orwell,"Mary Pickford, Mack Sennett",Terror,https://es.wikipedia.org/...,La novela 1984...
```

### 5. Conservación de información

El proceso de limpieza tiene como objetivo corregir únicamente los problemas necesarios para que los registros puedan ser utilizados por el sistema.

No se modificarán deliberadamente los valores válidos de las películas ni se eliminará información perteneciente a los ocho atributos definidos, salvo que el registro no pueda ser interpretado correctamente.

## Resultado del preprocesamiento

Como resultado del proceso se obtendrá un archivo CSV limpio y estructurado, cuyos registros contendrán únicamente los atributos utilizados por el sistema.

La estructura final será:

```text
Release Year,Title,Origin/Ethnicity,Director,Cast,Genre,Wiki Page,Plot
```

Estos datos serán utilizados posteriormente para cargar las películas en la estructura de datos seleccionada para el sistema de búsqueda. El proyecto requiere que los datos corregidos sean cargados en un **árbol que permita realizar búsquedas rápidas**, utilizando caracteres alfanuméricos como valores almacenados en sus nodos.
