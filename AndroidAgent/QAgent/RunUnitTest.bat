@echo on
setlocal
@echo update source code
call svn update
@echo clean up emma environment
call ant emma clean
call ant clean
cd tests
call ant emma clean
call ant clean
cd ..\
call android update project --path d:\workspace\Q\src\branches\DaVinci_CPP\AndroidAgent\QAgent
call android update test-project -m d:\workspace\Q\src\branches\DaVinci_CPP\AndroidAgent\QAgent -p  d:\workspace\Q\src\branches\DaVinci_CPP\AndroidAgent\QAgent\tests
call ant emma debug install test
cd tests
call ant emma debug install test
