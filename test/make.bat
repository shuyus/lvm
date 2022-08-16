del .\*exe
cl .\main.c ..\compiler\*.c ..\clib\*.c ..\common\*.c ..\vm\*.c ..\debugg\*.c
del .\*obj

@REM cl /c  ..\clib\*.c ..\common\*.c ..\vm\*.c ..\debug\*.c