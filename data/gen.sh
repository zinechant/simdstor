for i in 001 002 003 004 006 008 012 016 024 032 048 064 096 128 192 256; do
    dd if=/dev/urandom of=./${i}MiB bs=1MiB count=$i
done
