#!/bin/bash
#
# Starts beagle CI: init project, be-srv, daemon
#

set -e

BE=/src/build/be/be
BE_SRV=/src/build/be/be-srv
PROJECT=${1:-/tmp/testproject}

# create demo project

mkdir -p "$PROJECT"
cd "$PROJECT"

if [ ! -f main.c ]; then
    cat > main.c << 'SRC'
#include <stdio.h>
int main() {
    printf("hello beagle\n");
    return 0;
}
SRC
fi

if [ ! -f ci.json ]; then
    cat > ci.json << 'SRC'
{"name": "testproject", "build": "gcc-14 -o main main.c", "test": "./main && echo ok"}
SRC
fi

# copy dashboard into the project so it gets stored in beagle
cp /src/ci-app/index.html "$PROJECT/index.html"

# import everything into beagle

$BE post //testrepo/testproject
echo "project imported into beagle"

# start be-srv

$BE_SRV 8800 &
sleep 1
echo "be-srv on :8800"

# start CI daemon

echo "starting CI daemon on :8801"
exec node /src/ci-app/daemon.js http://127.0.0.1:8800 "$PROJECT"
