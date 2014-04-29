export ALLJOYN_ROOT="/home/ubuntu/alljoyn"
export ALLJOYN_DIST="$ALLJOYN_ROOT/core/alljoyn/build/linux/x86/debug/dist"

export CXXFLAGS="-I$ALLJOYN_DIST/cpp/inc \
		-I$ALLJOYN_DIST/about/inc \
		-I$ALLJOYN_DIST/services_common/inc \
		-I$ALLJOYN_DIST/notification/inc \
		-I$ALLJOYN_DIST/controlpanel/inc \
		-I$ALLJOYN_ROOT/services/services_common/cpp/inc/alljoyn/services_common/ \
		-I$ALLJOYN_ROOT/services/sample_apps/cpp/samples_common/"

export LDFLAGS="-L$ALLJOYN_DIST/cpp/lib \
		-L$ALLJOYN_DIST/about/lib \
		-L$ALLJOYN_DIST/services_common/lib \
		-L$ALLJOYN_DIST/notification/lib \
		-L$ALLJOYN_DIST/controlpanel/lib"

export LD_LIBRARY_PATH="$ALLJOYN_DIST/cpp/lib:$ALLJOYN_DIST/about/lib:$ALLJOYN_DIST/services_common/lib:$ALLJOYN_DIST/notification/lib:$ALLJOYN_DIST/controlpanel/lib"

for f in $ALLJOYN_ROOT/hackfest/aj_samples/*; do export PATH=$PATH:$f/build; done

build() {
    pushd $1
    scons WS=no
    popd
}

m() {
    if [ "$1" == "hackfest" ]; then
        for f in $ALLJOYN_ROOT/hackfest/aj_samples/*; do build $f; done
    else
        build $ALLJOYN_ROOT/hackfest/aj_samples/$1
    fi
}

