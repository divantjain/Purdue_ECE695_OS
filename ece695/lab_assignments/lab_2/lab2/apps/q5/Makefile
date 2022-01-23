default:
	cd krypton; make
	cd oxygen_inject; make
	cd nitrogen_inject; make
	cd oxygen; make
	cd nitrogen; make
	cd ozone; make
	cd no2; make

clean:
	cd krypton; make clean
	cd oxygen_inject; make clean
	cd nitrogen_inject; make clean
	cd oxygen; make clean
	cd nitrogen; make clean
	cd ozone; make clean
	cd no2; make clean

run:
	cd ../../bin; dlxsim -x os.dlx.obj -a -u krypton.dlx.obj 3; ee469_fixterminal
