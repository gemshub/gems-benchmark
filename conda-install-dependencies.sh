#!/bin/bash

if [ "$(uname)" == "Darwin" ]; then
    EXTN=dylib
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
    EXTN=so
fi

# Uncomment what is necessary to reinstall by force 
#rm -f  ${CONDA_PREFIX}/lib/libGEMS3K.$EXTN

# GEMS3K library
# if no GEMS3K installed in /usr/local/lib/libGEMS3K.so (/usr/local/include/GEMS3K)
test -f ${CONDA_PREFIX}/lib/libGEMS3K.$EXTN || {

        # Building GEMS3k library
        mkdir -p ~/code && \
        cd ~/code && \
        git clone https://github.com/gemshub/GEMS3K.git  && \
        cd GEMS3K && \
        mkdir -p build && \
        cd build && \
        cmake .. -DCMAKE_CXX_FLAGS=-fPIC -DBUILD_SOLMOD=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${CONDA_PREFIX} && \
        make -j4 && \
        sudo make install

        # Removing generated build files
        cd ~ && \
        rm -rf ~/code
}

