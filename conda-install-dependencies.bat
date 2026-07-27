
mkdir tmp_velo
cd tmp_velo

echo
echo ******                    ******
echo ****** Compiling GEMS3K  ******
echo ******                    ******
echo

echo git clone GEMS3K...
git clone https://github.com/sdmytrievs/GEMS3K.git
cd GEMS3K

echo "Configuring..."
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH="%CONDA_PREFIX%\Library" -DBUILD_SOLMOD=OFF -A x64 -S . -B build
echo "Building..."
cmake --build build --target install  --config Release

cd ..\..

rd /s /q tmp_velo
