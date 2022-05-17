cd ..

./booksim interposer2 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/asynchronus_sy_in1

sed -i -e 's/injection_rate=0.001/injection_rate=0.002/g' interposer2
./booksim interposer2 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/asynchronus_sy_in2

sed -i -e 's/injection_rate=0.002/injection_rate=0.003/g' interposer2
./booksim interposer2 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/asynchronus_sy_in3

sed -i -e 's/injection_rate=0.003/injection_rate=0.004/g' interposer2
./booksim interposer2 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/asynchronus_sy_in4

sed -i -e 's/injection_rate=0.004/injection_rate=0.005/g' interposer2
./booksim interposer2 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/asynchronus_sy_in5

sed -i -e 's/injection_rate=0.005/injection_rate=0.006/g' interposer2
./booksim interposer2 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/asynchronus_sy_in6

sed -i -e 's/injection_rate=0.006/injection_rate=0.001/g' interposer2
