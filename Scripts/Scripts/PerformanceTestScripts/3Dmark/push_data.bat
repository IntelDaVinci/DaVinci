@echo off

set dataFolder=\\shwdeopenlab024\9ZaoSharedFile\WriteableFolder\benchmark\3Dmark\
set destFolder=/sdcard/Android/obb/com.futuremark.dmandroid.application

adb shell mkdir %destFolder%
adb push %dataFolder%main.1232.com.futuremark.dmandroid.application.obb %destFolder%
adb push %dataFolder%patch.1232.com.futuremark.dmandroid.application.obb %destFolder%
