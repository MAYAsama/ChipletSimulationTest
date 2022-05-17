cd ..
./booksim interposer2 chiplet16/chip1 chiplet16/chip2 chiplet16/chip3 chiplet16/chip4 chiplet16/chip5 chiplet16/chip6 chiplet16/chip7 chiplet16/chip8 chiplet16/chip9 chiplet16/chip10 chiplet16/chip11 chiplet16/chip12 chiplet16/chip13 chiplet16/chip14 chiplet16/chip15 chiplet16/chip16 > Edata/chipsize_in1

sed -i -e 's/injection_rate=0.002/injection_rate=0.004/g' interposer2
./booksim interposer2 chiplet16/chip1 chiplet16/chip2 chiplet16/chip3 chiplet16/chip4 chiplet16/chip5 chiplet16/chip6 chiplet16/chip7 chiplet16/chip8 chiplet16/chip9 chiplet16/chip10 chiplet16/chip11 chiplet16/chip12 chiplet16/chip13 chiplet16/chip14 chiplet16/chip15 chiplet16/chip16 > Edata/chipsize_in2

sed -i -e 's/injection_rate=0.004/injection_rate=0.006/g' interposer2
./booksim interposer2 chiplet16/chip1 chiplet16/chip2 chiplet16/chip3 chiplet16/chip4 chiplet16/chip5 chiplet16/chip6 chiplet16/chip7 chiplet16/chip8 chiplet16/chip9 chiplet16/chip10 chiplet16/chip11 chiplet16/chip12 chiplet16/chip13 chiplet16/chip14 chiplet16/chip15 chiplet16/chip16 > Edata/chipsize_in3

sed -i -e 's/injection_rate=0.006/injection_rate=0.008/g' interposer2
./booksim interposer2 chiplet16/chip1 chiplet16/chip2 chiplet16/chip3 chiplet16/chip4 chiplet16/chip5 chiplet16/chip6 chiplet16/chip7 chiplet16/chip8 chiplet16/chip9 chiplet16/chip10 chiplet16/chip11 chiplet16/chip12 chiplet16/chip13 chiplet16/chip14 chiplet16/chip15 chiplet16/chip16 > Edata/chipsize_in4

sed -i -e 's/injection_rate=0.008/injection_rate=0.010/g' interposer2
./booksim interposer2 chiplet16/chip1 chiplet16/chip2 chiplet16/chip3 chiplet16/chip4 chiplet16/chip5 chiplet16/chip6 chiplet16/chip7 chiplet16/chip8 chiplet16/chip9 chiplet16/chip10 chiplet16/chip11 chiplet16/chip12 chiplet16/chip13 chiplet16/chip14 chiplet16/chip15 chiplet16/chip16 > Edata/chipsize_in5

sed -i -e 's/injection_rate=0.010/injection_rate=0.012/g' interposer2
./booksim interposer2 chiplet16/chip1 chiplet16/chip2 chiplet16/chip3 chiplet16/chip4 chiplet16/chip5 chiplet16/chip6 chiplet16/chip7 chiplet16/chip8 chiplet16/chip9 chiplet16/chip10 chiplet16/chip11 chiplet16/chip12 chiplet16/chip13 chiplet16/chip14 chiplet16/chip15 chiplet16/chip16 > Edata/chipsize_in6

sed -i -e 's/injection_rate=0.012/injection_rate=0.002/g' interposer2

