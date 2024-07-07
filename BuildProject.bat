IF NOT EXIST build\ (
mkdir build
)
cd build
cmake ../
cmake --build .

cd ../