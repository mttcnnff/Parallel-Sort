./tools/gen-input $1 test.dat;
/usr/bin/time -p ./tssort $2 test.dat output.dat;
./tools/check-sorted output.dat;
