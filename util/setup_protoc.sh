#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ -d $DIR/protoc ]; then
    echo "$DIR/protoc already exists"
    exit 1
fi

python3 -m pip install protobuf
mkdir $DIR/protoc && pushd $DIR/protoc && wget https://github.com/protocolbuffers/protobuf/releases/download/v3.19.1/protoc-3.19.1-linux-x86_64.zip && unzip protoc-3.19.1-linux-x86_64.zip

popd