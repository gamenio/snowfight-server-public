BEGIN TRANSACTION;
DROP TABLE IF EXISTS "realm_info";
CREATE TABLE IF NOT EXISTS "realm_info" (
	"id"	INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,
	"name"	TEXT,
	"group_id"	INTEGER DEFAULT 0,
	"address"	TEXT DEFAULT '0.0.0.0',
	"port"	INTEGER DEFAULT 0,
	"platform"	INTEGER DEFAULT 0,
	"game_version"	TEXT DEFAULT '1.0.0',
	"compatibility"	INTEGER DEFAULT 3
);
DROP TABLE IF EXISTS "region_map";
CREATE TABLE IF NOT EXISTS "region_map" (
	"country"	TEXT UNIQUE,
	"group_id"	INTEGER
);
INSERT INTO "realm_info" ("id","name","group_id","address","port","platform","game_version","compatibility") VALUES
 (1,'WorldServer',1,'0.0.0.0',18402,0,'3.1.0',3);
 
INSERT INTO "region_map" ("country","group_id") VALUES ('US',1);
COMMIT;
