# Endpoint mesh binaire

Route générique :

```text
/cache/<type>/<code>/mesh.bin
```

Types admis : `regions`, `departements`, `epcis`, `communes`.

## Découverte de l’URL

Une requête vers une entité (`/api/r/...`, `/api/d/...`, `/api/e/...` ou
`/api/c/...`) ne lit et ne construit aucun fichier de maillage. Sa réponse JSON
contient uniquement l’URL de la ressource :

```json
{
  "mesh": "/cache/<type>/<code>/mesh.bin"
}
```

Le client effectue ensuite une requête GET vers cette URL lorsqu’il a besoin du
maillage.

## Traitement de la requête du maillage

À la réception de la requête `GET /cache/<type>/<code>/mesh.bin` :

1. recherche et lecture du fichier dans le cache ;
2. s’il existe, réponse immédiate `application/octet-stream` sans lecture du
   maillage en base ;
3. sinon, requête SQL ciblée `ST_AsBinary(triangles)` ;
4. décodage WKB par `WkbReader` ;
5. construction par `Mesh` ;
6. sérialisation TVM1 par `MeshSerializer` ;
7. écriture atomique dans le cache ;
8. réponse `application/octet-stream`.
