# Streaming-C

Este repositorio contiene una implementacion de una plataforma web de streaming gratuito con una base de datos de alrededor de mas de 20000 películas , esta informacion es de acceso publica a traves del siguiente [enlace](https://drive.google.com/file/d/1Gae6uXFvvu5FbVNw3H2l69OpByFALqBd)


Adicionalmente contamos con una demo disponible en el siguiente [enlace](https://lazheart.github.io/streaming-c/)

Esta demo indexa y realiza busquedas utilizando algoritmos escritos en C++ , con el objetivo de optimizar la busqueda y el rendimiento.

Como parte de las tecnologias utilizadas para este proyecto se encuentra WebAssembly que permite ejecutar codigo c++ en el navegador web , adicionalmente se utiliza react para la UI Web de la aplicacion


## Estructura del Proyecto

```text
Streaming-C/
├── data/             # Base de datos (data.csv) y scripts C++ de carga de datos
│   ├── data.csv      # Dataset con más de 20,000 películas
│   └── fetchData.cpp # Script C++ para descarga de datos con libcurl
├── src/              # Algoritmos de búsqueda e indexación en C++
├── web/              # Aplicación Frontend (React + TypeScript + Vite)
│   ├── public/       # Archivos estáticos y módulos WebAssembly
│   └── src/          # Componentes e interfaz de usuario de React
├── main.cpp          # Código principal C++ compilable a WebAssembly
├── Makefile          # Scripts de automatización de compilación
├── ISSUE.md          # Control de tareas e imprevistos
└── README.md         # Documentación general del proyecto
```

## Búsqueda e indexación

El flujo de búsqueda está organizado con cuatro patrones de diseño:

- **Facade:** `SearchEngine::search` coordina la consulta, los índices, los filtros, el ranking y la paginación.
- **Strategy:** las consultas con texto y las consultas solo con filtros usan recorridos distintos.
- **Factory:** `SearchStrategyFactory` elige y crea la estrategia según la consulta.
- **Builder:** `SearchQueryBuilder` permite construir consultas con texto y paginación de forma encadenada.

Las búsquedas de títulos usan el suffix array; las de sinopsis usan un índice de trigramas y verifican el término sobre la trama original. Las consultas con varias palabras combinan resultados de ambas fuentes. En compilaciones nativas, los términos del suffix array se consultan en paralelo con un máximo de cuatro tareas; WebAssembly conserva la ruta secuencial porque se compila sin pthreads.

### Ejecutar la aplicación web localmente

Se requiere Node.js, pnpm, Emscripten (`emcc`), un compilador C++ y libcurl. `pnpm dev` prepara el catálogo en `data/` si falta, lo limpia, lo copia a `web/public/data_clean.csv`, compila `wasm_bridge.cpp` y finalmente inicia Vite:

```sh
cd web
pnpm install
pnpm dev
```

El CSV público es un artefacto generado por `make web-data`; el original y el limpio canónico viven en `data/`. Para preparar solo la copia pública desde la raíz, ejecuta `make web-data`; para compilar solo el módulo C++ ejecuta `make wasm`.

### CI/CD de GitHub Pages

`.github/workflows/ci.yml` valida cada push y pull request: compila el limpiador y el programa nativo, ejecuta las pruebas de búsqueda, del puente C++/WASM y del preprocesador, compila el módulo WASM y valida el frontend.

`.github/workflows/deploy.yml` ejecuta el flujo completo al actualizar `main`/`master` o manualmente: descarga el CSV original, lo limpia, copia el catálogo al sitio, compila el motor C++ a WASM, construye el frontend y publica el artefacto en GitHub Pages. El deploy se construye desde el mismo commit; no descarga artefactos de una ejecución anterior.

# Colaboradores

| Colaborador | github | descripcion |
|----------|----------|----------|
| Lazheart   | [Link](https://github.com/lazheart)   | UI Web y Algoritmos |
| sir-alexng   | [Link](https://github.com/sir-alexng)   | Integridad de Datos |
| BryanSSS   | [Link](https://github.com/Bryannsss140101)   | Algoritmos de Busqueda   |
| brandonRKO   | [Link](https://github.com/brandonRKO)   | Integridad de Datos   |
