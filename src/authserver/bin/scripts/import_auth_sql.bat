@echo off
setlocal enabledelayedexpansion

set base_dir=%~dp0
set work_dir=%base_dir:~0,-1%
set sqlite_program="sqlite3"
set db_files=^
	..\realmlist.db ^
	..\bannedlist.db

for %%f in (%db_files%) do (
	set db_path=%work_dir%\%%f
	call :import_sql_to_db !db_path!
)

pause

EXIT /B %ERRORLEVEL%
	
:import_sql_to_db

set db_path=%~1
set sql_path=%db_path%.sql

echo %sql_path%

if exist "%sql_path%" (
	set sql_path_to_unix=%sql_path:\=/%
	if exist "%db_path%" del "%db_path%"
	echo Import "%sql_path%" into "%db_path%".
	%sqlite_program% "%db_path%" ".read !sql_path_to_unix!"
) else (
	echo SQL file "%sql_path%" was not found.
)

EXIT /B 0
