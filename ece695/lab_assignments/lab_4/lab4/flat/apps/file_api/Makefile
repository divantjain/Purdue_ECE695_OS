default:
	cd file_api; make

clean:
	cd file_api; make clean

run:
	cd ../../os; make; cd ../bin; dlxsim -x os.dlx.obj -a -D F -u file_api.dlx.obj; ee469_fixterminal
