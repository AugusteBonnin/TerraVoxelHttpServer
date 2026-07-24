# API tuilée TerraVoxel

L’index public d’une tuile utilise le coin inférieur gauche Lambert-93 :

```text
{niveauEnM}/{nord}/{est}
```

- `niveauEnM` est la longueur du côté du carré en mètres, complétée à gauche par des zéros jusqu’à 7 chiffres ;
- `nord` est l’ordonnée Lambert-93 du bord sud (`minY`) ;
- `est` est l’abscisse Lambert-93 du bord ouest (`minX`).

Les niveaux acceptés sont des longueurs entières en mètres :
`125`, `250`, `500`, `1000`, puis chaque niveau doublé jusqu’à `1024000`.
Il n’existe plus de niveau interne exprimé sous forme d’exposant.

Exemple pour une tuile de 1 000 m dont le coin inférieur gauche est
`minX = 725000`, `minY = 6300000` :

```text
0001000/6300000/725000
```

Le modèle C++ utilise directement ces coordonnées :

```text
x = minX
y = minY
xmax = minX + niveauEnM
ymax = minY + niveauEnM
```

## Métadonnées

```text
GET /api/t/{niveauEnM}/{nord}/{est}
```

Exemple :

```text
GET /api/t/0001000/6300000/725000
```

La réponse contient notamment :

```json
{
  "id": "0001000/6300000/725000",
  "niveau": 1000,
  "minX": 725000,
  "minY": 6300000,
  "est": 725000,
  "nord": 6300000
}
```

## Ressources

```text
GET /tiles/{niveauEnM}/{nord}/{est}/mesh.bin
GET /tiles/{niveauEnM}/{nord}/{est}/ortho.jpg
GET /tiles/{niveauEnM}/{nord}/{est}/mnt.bin
```

Exemples :

```text
GET /tiles/0001000/6300000/725000/mesh.bin
GET /tiles/0001000/6300000/725000/ortho.jpg
GET /tiles/0001000/6300000/725000/mnt.bin
```

- `mesh.bin` utilise le format `TVM1` ;
- `ortho.jpg` est téléchargé à la demande depuis le WMS Géoplateforme ;
- `mnt.bin` est un BIL float32 de 101 × 101 échantillons.

L’index spatial ne change pas l’ordre mémoire des rasters. La BBOX WMS reste
`minX,minY,maxX,maxY`. Le JPEG et le BIL sont enregistrés tels que renvoyés par
le serveur WMS : première ligne au nord, puis progression vers le sud. Aucun
retournement vertical n’est effectué lors de la mise en cache.

## Couverture d’une entité

```text
GET /api/tiles/{type}/{code}/{couverture}/{niveauEnM}
```

Couvertures acceptées :

```text
rectangle
carre
contour
```

Exemples :

```text
GET /api/tiles/c/34036/rectangle/0001000
GET /api/tiles/c/34036/carre/0001000
GET /api/tiles/c/34036/contour/0001000
```

La couverture `rectangle` utilise le rectangle englobant de l’entité.
La couverture `carre` utilise son carré englobant. La couverture `contour`
part du TileSet carré, puis ne conserve que les tuiles qui intersectent la
géométrie réelle de l’entité dans PostGIS.

Types acceptés : `r`, `regions`, `d`, `departements`, `e`, `epci`, `epcis`, `c`, `communes`.

## Cache disque

```text
cache/
  tiles/
    0001000/
      6300000/
        725000/
          mesh.bin
          ortho.jpg
          mnt.bin
```

Le répertoire racine est défini par `TERRAVOXEL_CACHE_ROOT`.
