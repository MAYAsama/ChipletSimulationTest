cd ..
./booksim test > cretest/spsim_2d_0.01

sed -i -e 's/injection_rate=0.01/injection_rate=0.02/g' test
./booksim test > cretest/spsim_2d_0.02

sed -i -e 's/injection_rate=0.02/injection_rate=0.03/g' test
./booksim test > cretest/spsim_2d_0.03

sed -i -e 's/injection_rate=0.03/injection_rate=0.04/g' test
./booksim test > cretest/spsim_2d_0.04

sed -i -e 's/injection_rate=0.04/injection_rate=0.05/g' test
./booksim test > cretest/spsim_2d_0.05

sed -i -e 's/injection_rate=0.05/injection_rate=0.06/g' test
./booksim test > cretest/spsim_2d_0.06

sed -i -e 's/injection_rate=0.06/injection_rate=0.07/g' test
./booksim test > cretest/spsim_2d_0.07

sed -i -e 's/injection_rate=0.07/injection_rate=0.08/g' test
./booksim test > cretest/spsim_2d_0.08

sed -i -e 's/injection_rate=0.08/injection_rate=0.09/g' test
./booksim test > cretest/spsim_2d_0.09

sed -i -e 's/injection_rate=0.09/injection_rate=0.10/g' test
./booksim test > cretest/spsim_2d_0.10

sed -i -e 's/injection_rate=0.10/injection_rate=0.01/g' test

