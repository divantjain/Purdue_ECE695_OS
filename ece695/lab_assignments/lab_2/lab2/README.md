# Lab 2 : Process Synchronization for Dummies

## Link to Project Page
[Project Page](https://engineering.purdue.edu/~ece695x/labs_2021/lab2.html)

## Prerequisite

    Clean ALL

    cd apps
    cd q2/ && make clean && cd ..
    cd q3/ && make clean && cd ..
    cd q4/ && make clean && cd ..
    cd q5/ && make clean && cd ../..

## Q1
    Check Prelab2.txt

## Q2

    First 'cd bin'
    then

    cd .. && mainframer.sh "cd os && make" && mainframer.sh "cd apps/q2 && make" && cd bin && dlxsim -x os.dlx.obj -a -u makeprocs.dlx.obj 2
## Q3

    First 'cd bin'
    then

    cd .. && mainframer.sh "cd os && make" && mainframer.sh "cd apps/q3 && make" && cd bin && dlxsim -x os.dlx.obj -a -u makeprocs.dlx.obj 2

## Q4

    First 'cd bin'
    then

    cd .. && mainframer.sh "cd os && make" && mainframer.sh "cd apps/q4 && make" && cd bin && dlxsim -x os.dlx.obj -a -u makeprocs.dlx.obj 2

## Q5

    First 'cd bin'
    then

    cd .. && mainframer.sh "cd os && make" && mainframer.sh "cd apps/q5 && make" && cd bin && dlxsim -x os.dlx.obj -a -u krypton.dlx.obj 4 10 60