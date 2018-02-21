./tools/gen-input $1 test.dat;
echo "==== ORGINAL ====";
./tools/print-data test.dat;

echo "==== FUNCTION ====";
./ssort $2 test.dat;

sleep 5;
echo "==== SORTED ====";
./tools/print-data test.dat;