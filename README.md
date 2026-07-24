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

Le projet est construit avec CMake et requiert **Qt 6.9.2 ou supérieur** (pour le module HttpServer et le pattern responder).

### Développement local

```bash
rm -rf build
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$HOME/Qt/6.12.0/gcc_64"
cmake --build build
```

### Déploiement sur Ubuntu 26.04

Sur Ubuntu 26.04, Qt 6.9.2 est fourni via apt avec le module HttpServer :

```bash
sudo apt-get install -y qt6-base-dev qt6-concurrent-dev qt6-sql-dev libqt6network6 libqt6httpserver6-dev cmake ninja-build
rm -rf build
cmake -S . -B build -G Ninja
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

Un rate limiter simple est également disponible côté serveur Qt. Il peut être configuré avec :

```bash
export TERRAVOXEL_RATE_LIMIT_REQUESTS_PER_WINDOW=120
export TERRAVOXEL_RATE_LIMIT_WINDOW_SECONDS=60
```

## Exécution

Après la compilation, l’exécutable produit est lancé ainsi :

```bash
export TERRAVOXEL_HTTP_PORT=8080
./build/TerraVoxelHttpServer
```

Un script de démarrage est également fourni :

```bash
./start-terravoxel.sh
```

Il lance le binaire avec les variables d’environnement utiles, crée un répertoire de cache local si nécessaire et enregistre les logs dans le dossier logs/.

## Déploiement avec nginx

Dans un déploiement réel, TerraVoxelHttpServer peut être exécuté en arrière-plan sur un port local, tandis qu’un serveur nginx sert les pages statiques du front office et inverse les requêtes vers l’API du service.

Un schéma simple est le suivant :

- nginx sert les fichiers HTML/CSS/JS depuis un répertoire de pages statiques
- nginx proxy les routes `/api/`, `/cache/`, `/tiles/` et `/health` vers `127.0.0.1:8080`
- nginx gère déjà l’HTTPS et la redirection depuis le port 80 vers le port 443
- TerraVoxelHttpServer continue à gérer la logique métier, les accès base de données, le cache disque et les réponses binaires

Exemple de configuration nginx HTTPS/443 :

```nginx
upstream terravoxel_backend {
    server 127.0.0.1:8080;
}

server {
    listen 80;
    server_name terravoxel.example.com;
    root /var/www/terravoxel/www;
    index index.html;

    location / {
        try_files $uri $uri/ /index.html;
    }

    location /health {
        proxy_pass http://terravoxel_backend;
        proxy_set_header Host $host;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    }

    location /api/ {
        proxy_pass http://terravoxel_backend;
        proxy_set_header Host $host;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    }

    location /cache/ {
        proxy_pass http://terravoxel_backend;
        proxy_set_header Host $host;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    }

    location /tiles/ {
        proxy_pass http://terravoxel_backend;
        proxy_set_header Host $host;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    }
}
```

Un exemple complet est disponible dans [nginx-terravoxel.conf.example](nginx-terravoxel.conf.example).

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
GET /api/t/{niveauEnM}/{nord}/{est}
GET /api/tiles/{type}/{code}/{niveauEnM}
GET /tiles/{niveauEnM}/{nord}/{est}/mesh.bin
GET /tiles/{niveauEnM}/{nord}/{est}/ortho.jpg
GET /tiles/{niveauEnM}/{nord}/{est}/mnt.bin
```

Pour les détails sur les formats et les paramètres attendus, voir les documents suivants :

- [TILE_API.md](TILE_API.md)
- [DATABASE_ENV.md](DATABASE_ENV.md)
- [MESH_ENDPOINT.md](MESH_ENDPOINT.md)
