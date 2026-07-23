# Endpoint mesh binaire

Route générique :

```text
/cache/<type>/<code>/mesh.bin
```

Types admis : `regions`, `departements`, `epcis`, `communes`.

Étapes :

1. requête SQL ciblée `ST_AsBinary(triangles)` ;
2. lecture du fichier déjà en cache lorsqu'il existe ;
3. sinon décodage WKB par `WkbReader` ;
4. construction par `Mesh` ;
5. sérialisation TVM1 par `MeshSerializer` ;
6. écriture atomique dans le cache ;
7. réponse `application/octet-stream`.
