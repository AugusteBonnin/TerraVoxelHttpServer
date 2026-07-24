# API tuilée TerraVoxel

L’index public d’une tuile utilise le coin inférieur gauche Lambert-93 :

```text
{niveauEnM}/{nord}/{est}
```

- `niveauEnM` est la longueur du côté du carré en mètres, complétée à gauche par des zéros jusqu’à 7 chiffres ;
- `nord` est l’ordonnée Lambert-93 du bord sud (`minY`) ;
- `est` est l’abscisse Lambert-93 du bord ouest (`minX`).

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
  "niveau": "0001000",
  "niveauM": 1000,
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
GET /api/tiles/{type}/{code}/{niveauEnM}
```

Exemple :

```text
GET /api/tiles/c/34036/0001000
```

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
