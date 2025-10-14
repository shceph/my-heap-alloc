cd build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_C_COMPILER=gcc\
	-DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" ..
cd ..
