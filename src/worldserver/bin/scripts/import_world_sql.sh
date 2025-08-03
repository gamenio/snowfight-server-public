#!/bin/sh

BASEDIR=$(dirname $0)
SQLITE_PROGRAM="sqlite3"
DB_FILES="
	../odb/itembox.db
	../odb/item.db
	../odb/player.db 
	../odb/robot.db 
	../odb/unit.db 
	../map/map.db
	"

import_sql_to_db() {
	db_path=${1}
	sql_path=${db_path}.sql
	if test -f "${sql_path}"; then
		if test -f "${db_path}"; then	
			rm "${db_path}"
		fi
		echo "Import \"${sql_path}\" into \"${db_path}\"."	
		${SQLITE_PROGRAM} "${db_path}" ".read ${sql_path}"
	else
		echo "SQL file \"${sql_path}\" was not found."
	fi
}


for file in $DB_FILES; do
	db_path=${BASEDIR}/${file}
	import_sql_to_db "$db_path"
done

