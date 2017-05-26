@echo off

set dataFolder=\\shwdeopenlab024\9ZaoSharedFile\WriteableFolder\benchmark\QuakeIII\

adb shell mkdir /sdcard/quake3
adb shell mkdir /sdcard/quake3/baseq3
adb shell mkdir /data/data/org.kwaak3
adb push %dataFolder%pak0.pk3 /sdcard/quake3/baseq3
adb push %dataFolder%pak1.pk3 /sdcard/quake3/baseq3
adb push %dataFolder%pak2.pk3 /sdcard/quake3/baseq3
adb push %dataFolder%pak3.pk3 /sdcard/quake3/baseq3
adb push %dataFolder%pak4.pk3 /sdcard/quake3/baseq3
adb push %dataFolder%pak5.pk3 /sdcard/quake3/baseq3
adb push %dataFolder%pak6.pk3 /sdcard/quake3/baseq3
adb push %dataFolder%pak7.pk3 /sdcard/quake3/baseq3
adb push %dataFolder%pak8.pk3 /sdcard/quake3/baseq3
adb logcat -c
