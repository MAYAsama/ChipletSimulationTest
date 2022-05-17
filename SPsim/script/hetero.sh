cd ..
./booksim interposer3 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/hetero_in1

sed -i -e 's/injection_rate=0.005/injection_rate=0.010/g' interposer3
./booksim interposer3 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/hetero_in2

sed -i -e 's/injection_rate=0.010/injection_rate=0.015/g' interposer3
./booksim interposer3 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/hetero_in3

sed -i -e 's/injection_rate=0.015/injection_rate=0.020/g' interposer3
./booksim interposer3 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/hetero_in4

sed -i -e 's/injection_rate=0.020/injection_rate=0.025/g' interposer3
./booksim interposer3 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/hetero_in5

sed -i -e 's/injection_rate=0.025/injection_rate=0.03/g' interposer3
./booksim interposer3 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/hetero_in6

sed -i -e 's/injection_rate=0.03/injection_rate=0.005/g' interposer3

