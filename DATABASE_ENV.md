# Configuration PostgreSQL

Le serveur lit les variables d'environnement suivantes :

- `TERRAVOXEL_DB_HOST` (défaut : `127.0.0.1`)
- `TERRAVOXEL_DB_PORT` (défaut : `5432`)
- `TERRAVOXEL_DB_NAME` (défaut : `geodb`)
- `TERRAVOXEL_DB_USER` (défaut : `geo`)
- `TERRAVOXEL_DB_PASSWORD` (aucune valeur par défaut)

Exemple :

```bash
export TERRAVOXEL_DB_HOST=127.0.0.1
export TERRAVOXEL_DB_PORT=5432
export TERRAVOXEL_DB_NAME=geodb
export TERRAVOXEL_DB_USER=geo
export TERRAVOXEL_DB_PASSWORD='mot-de-passe'
```

Les littéraux `QString` utilisent `QStringLiteral`, ce qui rend le code compatible avec
`QT_NO_CAST_FROM_ASCII` et Qt 6.12.
