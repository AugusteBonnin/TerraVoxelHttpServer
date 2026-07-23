# TerraVoxelHttpServer

TerraVoxelHttpServer est un service HTTP écrit en C++ avec Qt 6 pour exposer des données géographiques sous forme de maillages 3D, métadonnées tuilées et ressources raster. Le projet lit ses données depuis une base PostgreSQL/PostGIS et met en cache les résultats sur disque.

## Fonctionnalités principales

- exposition de maillages binaires pour les entités géographiques : régions, départements, EPCI et communes
- API tuilée pour accéder aux métadonnées et aux ressources associées
- chargement et mise en cache de maillages, orthophotos et données de relief
- configuration via variables d’environnement pour la base de données et le cache disque

## Prérequis

- CMake 3.22 ou plus
- compilateur C++17
- Qt 6 avec les composants suivants : Core, Gui, Network, Sql, HttpServer, Concurrent
- une base PostgreSQL avec les tables et données nécessaires au projet

## Construction

Le projet est construit avec CMake. Une configuration recommandée est la suivante :

```bash
rm -rf build
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$HOME/Qt/6.12.0/gcc_64"
cmake --build build
```

> Il est préférable de ne pas réutiliser un répertoire de build généré avec Unix Makefiles lorsque vous utilisez Ninja.

## Configuration

Le serveur lit les variables d’environnement suivantes :

```bash
export TERRAVOXEL_DB_HOST=127.0.0.1
export TERRAVOXEL_DB_PORT=5432
export TERRAVOXEL_DB_NAME=geodb
export TERRAVOXEL_DB_USER=geo
export TERRAVOXEL_DB_PASSWORD='mot-de-passe'
```

Le répertoire de cache peut être défini ainsi :

```bash
export TERRAVOXEL_CACHE_ROOT="$PWD/cache"
```

## Exécution

Après la compilation, l’exécutable produit est lancé ainsi :

```bash
./build/TerraVoxelHttpServer
```

## Routes principales

### Maillages binaires

```text
GET /cache/regions/{code}/mesh.bin
GET /cache/departements/{code}/mesh.bin
GET /cache/epcis/{code}/mesh.bin
GET /cache/communes/{code}/mesh.bin
```

Si un maillage n’est pas déjà présent dans le cache, le serveur le construit à partir des données PostgreSQL, puis le renvoie avec le type MIME application/octet-stream.

### API tuilée

```text
GET /api/t/{niveauEnM}/{minX}/{minY}
GET /api/tiles/{type}/{code}/{niveauEnM}
GET /tiles/{niveauEnM}/{minX}/{minY}/mesh.bin
GET /tiles/{niveauEnM}/{minX}/{minY}/ortho.jpg
GET /tiles/{niveauEnM}/{minX}/{minY}/mnt.bin
```

Pour les détails sur les formats et les paramètres attendus, voir les documents suivants :

- [TILE_API.md](TILE_API.md)
- [DATABASE_ENV.md](DATABASE_ENV.md)
- [MESH_ENDPOINT.md](MESH_ENDPOINT.md)
