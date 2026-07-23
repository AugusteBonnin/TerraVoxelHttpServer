BEGIN;

CREATE EXTENSION IF NOT EXISTS postgis;
CREATE EXTENSION IF NOT EXISTS pgcrypto;

DO $$
BEGIN
    CREATE TYPE type_entite AS ENUM ('france','region','departement','epci','commune');
EXCEPTION WHEN duplicate_object THEN NULL;
END $$;

DO $$
BEGIN
    CREATE TYPE couverture_tuile AS ENUM ('carre','rectangle','contour','user_rectangle','user_polygon');
EXCEPTION WHEN duplicate_object THEN NULL;
END $$;

CREATE TABLE IF NOT EXISTS tuile
(
    niveau smallint NOT NULL CHECK (niveau BETWEEN -3 AND 10),
    x bigint NOT NULL,
    y bigint NOT NULL,
    cle text GENERATED ALWAYS AS (niveau::text || '/' || x::text || '/' || y::text) STORED,
    geometrie geometry(Polygon,2154) GENERATED ALWAYS AS
    (
        ST_MakeEnvelope(
            x::double precision,
            y::double precision,
            x::double precision + 1000.0 * power(2.0,niveau::double precision),
            y::double precision + 1000.0 * power(2.0,niveau::double precision),
            2154
        )
    ) STORED,
    created_at timestamptz NOT NULL DEFAULT now(),
    PRIMARY KEY(niveau,x,y)
);

CREATE UNIQUE INDEX IF NOT EXISTS tuile_cle_idx ON tuile(cle);
CREATE INDEX IF NOT EXISTS tuile_geometrie_idx ON tuile USING gist(geometrie);

CREATE TABLE IF NOT EXISTS jeu_tuiles
(
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    type_entite type_entite NOT NULL,
    code_entite varchar(32) NOT NULL,
    nom text,
    couverture couverture_tuile NOT NULL,
    niveau smallint NOT NULL CHECK (niveau BETWEEN -3 AND 10),
    geometrie geometry(Geometry,2154),
    created_at timestamptz NOT NULL DEFAULT now(),
    updated_at timestamptz NOT NULL DEFAULT now(),
    CHECK (
        (couverture IN ('user_rectangle','user_polygon') AND geometrie IS NOT NULL)
        OR couverture IN ('carre','rectangle','contour')
    )
);

CREATE INDEX IF NOT EXISTS jeu_tuiles_entite_idx ON jeu_tuiles(type_entite,code_entite);
CREATE INDEX IF NOT EXISTS jeu_tuiles_geometrie_idx ON jeu_tuiles USING gist(geometrie);

CREATE TABLE IF NOT EXISTS jeu_tuiles_element
(
    jeu_tuiles_id uuid NOT NULL REFERENCES jeu_tuiles(id) ON DELETE CASCADE,
    niveau smallint NOT NULL CHECK (niveau BETWEEN -3 AND 10),
    x bigint NOT NULL,
    y bigint NOT NULL,
    taux_couverture double precision NOT NULL DEFAULT 1.0 CHECK (taux_couverture BETWEEN 0.0 AND 1.0),
    PRIMARY KEY(jeu_tuiles_id,niveau,x,y),
    FOREIGN KEY(niveau,x,y) REFERENCES tuile(niveau,x,y) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS jeu_tuiles_element_tuile_idx ON jeu_tuiles_element(niveau,x,y);

CREATE OR REPLACE FUNCTION check_jeu_tuiles_element_level()
RETURNS trigger LANGUAGE plpgsql AS $$
DECLARE expected_level smallint;
BEGIN
    SELECT niveau INTO expected_level FROM jeu_tuiles WHERE id = NEW.jeu_tuiles_id;
    IF expected_level IS NULL THEN
        RAISE EXCEPTION 'Jeu de tuiles inexistant : %', NEW.jeu_tuiles_id;
    END IF;
    IF NEW.niveau <> expected_level THEN
        RAISE EXCEPTION 'Niveau incorrect : attendu %, reçu %', expected_level, NEW.niveau;
    END IF;
    RETURN NEW;
END;
$$;

DROP TRIGGER IF EXISTS jeu_tuiles_element_level_trigger ON jeu_tuiles_element;
CREATE TRIGGER jeu_tuiles_element_level_trigger
BEFORE INSERT OR UPDATE ON jeu_tuiles_element
FOR EACH ROW EXECUTE FUNCTION check_jeu_tuiles_element_level();

COMMIT;
