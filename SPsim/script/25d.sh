cd ..
./booksim interposer2 > Edata/2d_in1

sed -i -e 's/injection_rate=0.002/injection_rate=0.004/g' interposer2
./booksim interposer2 > Edata/2d_in2

sed -i -e 's/injection_rate=0.004/injection_rate=0.006/g' interposer2
./booksim interposer2 > Edata/2d_in3

sed -i -e 's/injection_rate=0.006/injection_rate=0.008/g' interposer2
./booksim interposer2 > Edata/2d_in4

sed -i -e 's/injection_rate=0.008/injection_rate=0.010/g' interposer2
./booksim interposer2 > Edata/2d_in5

sed -i -e 's/injection_rate=0.010/injection_rate=0.012/g' interposer2
./booksim interposer2 > Edata/2d_in6

sed -i -e 's/injection_rate=0.012/injection_rate=0.002/g' interposer2

