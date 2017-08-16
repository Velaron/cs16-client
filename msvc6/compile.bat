..\vsmini\bin\vc
cl /DXASH_DISABLE_FWGS_EXTENSIONS /DMY_COMPILER_SUCKS user32.lib ..\*.cpp ..\controls\*.cpp ..\menus\*.cpp ..\menus\dynamic\*.cpp -I..\ -I..\controls -I..\menus -I..\..\xash3d -I..\..\xash3d\engine -I..\..\xash3d\pm_shared -I..\..\xash3d\common -o menu.dll /link /dll /out:menu.dll
del *.obj
