@echo off
rem OWTF2: generates game_tf.sln with only the TF client, server and the
rem libraries they need (VPC's "game" group). Unlike createallprojects.bat it
rem skips tools such as qc_eyes that require MFC, so it builds faster.
pushd "%~dp0"
devtools\bin\vpc.exe /tf /define:SOURCESDK +game /mksln game_tf.sln
set RESULT=%ERRORLEVEL%
popd
exit /b %RESULT%
