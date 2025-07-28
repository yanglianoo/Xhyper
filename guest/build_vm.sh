if [ -d "./build" ]; then
    rm -rf ./build
fi

mkdir -p build
cd build

cmake ..
make

aarch64-linux-gnu-ld -r -b binary Guest_VM -o Guest_VM.o
cp Guest_VM.o ../
