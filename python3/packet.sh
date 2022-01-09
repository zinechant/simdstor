bash ../util/setup_protoc.sh

echo "protoc installed, generalting pb interface ..."
../util/protoc/bin/protoc --python_out=. --proto_path=$HOME/gem5/src/proto $HOME/gem5/src/proto/packet.proto
