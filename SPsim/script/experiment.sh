cd ..
./booksim interposer1 chip1 chip2 chip3 chip4 > Edata/asynchronus_asy_in1

sed -i -e 's/injection_rate=0.002/injection_rate=0.004/g' interposer1
./booksim interposer1 chip1 chip2 chip3 chip4 > Edata/asynchronus_asy_in2

sed -i -e 's/injection_rate=0.004/injection_rate=0.006/g' interposer1
./booksim interposer1 chip1 chip2 chip3 chip4 > Edata/asynchronus_asy_in3

sed -i -e 's/injection_rate=0.006/injection_rate=0.008/g' interposer1
./booksim interposer1 chip1 chip2 chip3 chip4 > Edata/asynchronus_asy_in4

sed -i -e 's/injection_rate=0.008/injection_rate=0.010/g' interposer1
./booksim interposer1 chip1 chip2 chip3 chip4 > Edata/asynchronus_asy_in5

sed -i -e 's/injection_rate=0.010/injection_rate=0.012/g' interposer1
./booksim interposer1 chip1 chip2 chip3 chip4 > Edata/asynchronus_asy_in6

sed -i -e 's/injection_rate=0.012/injection_rate=0.002/g' interposer1


./booksim interposer2 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/asynchronus_sy_in1

sed -i -e 's/injection_rate=0.002/injection_rate=0.004/g' interposer2
./booksim interposer2 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/asynchronus_sy_in2

sed -i -e 's/injection_rate=0.004/injection_rate=0.006/g' interposer2
./booksim interposer2 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/asynchronus_sy_in3

sed -i -e 's/injection_rate=0.006/injection_rate=0.008/g' interposer2
./booksim interposer2 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/asynchronus_sy_in4

sed -i -e 's/injection_rate=0.008/injection_rate=0.010/g' interposer2
./booksim interposer2 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/asynchronus_sy_in5

sed -i -e 's/injection_rate=0.010/injection_rate=0.012/g' interposer2
./booksim interposer2 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/asynchronus_sy_in6

sed -i -e 's/injection_rate=0.012/injection_rate=0.002/g' interposer2
