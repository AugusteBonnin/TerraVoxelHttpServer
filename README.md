# TerraVoxelHttpServer — sources complètes

Cette version conserve toutes les sources historiques et ajoute le Repository et les routes API.

## Construction

```bash
rm -rf build
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$HOME/Qt/6.12.0/gcc_64"
cmake --build build
```

Ne réutilise pas un répertoire de build généré avec Unix Makefiles pour Ninja.

## Route de mesh binaire

```text
GET /cache/regions/{code}/mesh.bin
GET /cache/departements/{code}/mesh.bin
GET /cache/epcis/{code}/mesh.bin
GET /cache/communes/{code}/mesh.bin
```

Le serveur recherche d'abord `mesh.bin` dans le cache. S'il est absent, il lit
`triangles` dans PostgreSQL, décode le WKB, construit le mesh, l'enregistre puis
le renvoie avec le type MIME `application/octet-stream`.

Le répertoire de cache est configurable :

```bash
export TERRAVOXEL_CACHE_ROOT=/var/www/terravoxel/cache
```

Pour un test local sans droits sous `/var/www` :

```bash
export TERRAVOXEL_CACHE_ROOT="$PWD/cache"
```

## API tuilée

Voir `TILE_API.md` pour les routes `/api/t`, `/api/tiles` et `/tiles`.
