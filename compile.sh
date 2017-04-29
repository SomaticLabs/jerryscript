python tools/build.py --jerry-libc=off --jerry-libm=off --jerry-cmdline=off --snapshot-exec=ON --snapshot-save=off --mem-heap=18 --clean --toolchain=./cmake/toolchain_mcu_nrf52832.cmake

rm dist/libjerry-core.a
cp build/lib/libjerry-core.a dist/
