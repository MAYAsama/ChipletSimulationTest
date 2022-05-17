cd ..
./booksim interposer5 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/syn_blackscholes_64c_simsmall

sed -i -e 's/blackscholes_64c_simsmall.tra.bz2/blackscholes_64c_simmedium.tra.bz2/g' interposer5
./booksim interposer5 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/syn_blackscholes_64c_simmedium

sed -i -e 's/blackscholes_64c_simmedium.tra.bz2/blackscholes_64c_simlarge.tra.bz2/g' interposer5
./booksim interposer5 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/syn_blackscholes_64c_simlarge

sed -i -e 's/blackscholes_64c_simlarge.tra.bz2/fluidanimate_64c_simsmall.tra.bz2/g' interposer5
./booksim interposer5 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/syn_fluidanimate_64c_simsmall

sed -i -e 's/fluidanimate_64c_simsmall.tra.bz2/fluidanimate_64c_simmedium.tra.bz2/g' interposer5
./booksim interposer5 chip1_1 chip2_1 chip3_1 chip4_1 > Edata/syn_fluidanimate_64c_simmedium

sed -i -e 's/fluidanimate_64c_simmedium.tra.bz2/blackscholes_64c_simsmall.tra.bz2/g' interposer5


