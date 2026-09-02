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

# Colaboradores

| Colaborador | github | descripcion |
|----------|----------|----------|
| Lazheart   | [Link](https://github.com/lazheart)   | UI Web y Algoritmos |
| LuvAmoris   | [Link](https://github.com/luv-amori133)   | Integridad de Datos |
| BryanSSS   | [Link](https://github.com/Bryannsss140101)   | Algoritmos de Busqueda   |
