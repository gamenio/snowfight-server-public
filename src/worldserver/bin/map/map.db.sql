BEGIN TRANSACTION;
DROP TABLE IF EXISTS "map_grade";
CREATE TABLE IF NOT EXISTS "map_grade" (
	"combat_grade"	INTEGER,
	"map_id"	INTEGER,
	"weight"	REAL
);
DROP TABLE IF EXISTS "map_config";
CREATE TABLE IF NOT EXISTS "map_config" (
	"id"	INTEGER UNIQUE,
	"population_cap"	INTEGER,
	"battle_duration"	INTEGER,
	PRIMARY KEY("id")
);
INSERT INTO "map_grade" ("combat_grade","map_id","weight") VALUES (1,1,1.0),
 (2,1,1.0),
 (3,1,1.0),
 (4,1,1.0),
 (5,1,1.0),
 (0,0,1.0);
INSERT INTO "map_config" ("id","population_cap","battle_duration") VALUES (0,2,0),
 (1,15,360);
COMMIT;
