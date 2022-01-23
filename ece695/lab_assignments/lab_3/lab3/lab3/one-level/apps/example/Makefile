default:
	cd makeprocs; make
	cd hello_world; make
	cd test_prog_1; make
	cd test_prog_2; make
	cd test_prog_3; make
	cd test_prog_4; make
	cd test_prog_5; make

clean:
	cd makeprocs; make clean
	cd hello_world; make clean
	cd test_prog_1; make clean
	cd test_prog_2; make clean
	cd test_prog_3; make clean
	cd test_prog_4; make clean
	cd test_prog_5; make clean

run:
	cd ../../bin; dlxsim -x os.dlx.obj -a -u makeprocs.dlx.obj 5; ee469_fixterminal
