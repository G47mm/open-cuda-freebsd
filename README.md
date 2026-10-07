# open-cuda-freebsd
freebsd cuda code made from scratch.

Well i coded the uvm_shim_bsd.c from the linuxulator shim code (uvm_shim.c) i took from https://www.reddit.com/r/freebsd/comments/1sey5l3/cuda_works/

Credits to North_Promise_9835 for making the linuxulator code (uvm_shim.c)
Thanks to North_Promise_9835 for making the linuxulator code (uvm_shim.c)

I made the uvm_shim_bsd.c

Feel free to improve the code in this repository


Proof:

https://github.com/user-attachments/assets/58963d37-8fc1-436f-afea-31cd863f50da


How to compile & run:

1. Clone this repo:
    https://github.com/G47mm/open-cuda-freebsd.git

2. Compile the (uvm_shim.c, i assume you setup linuxulator)
    /compat/linux/usr/bin/gcc -m64 -std=c11 -O2 -fPIC -shared -o uvm_shim.so uvm_shim.c -ldl

    if you must natively run blender (not tested)
        clang -O2 -fPIC -shared -o uvm_shim_bsd.so uvm_shim_bsd.c -ldl

3. Create the directory in /usr/local/lib/ & move the .so file(s) to that directory that is created (by you or anyone/anything else)

    i am going with /usr/local/lib/cuda-shim/
     doas (or sudo) mkdir -p /usr/local/lib/cuda-shim/

   mv path/to/uvm/uvm_shim.so /usr/local/lib/cuda-shim/

       if if you must natively run blender (not tested)
           mv path/to/uvm/uvm_shim_bsd.so /usr/local/lib/cuda-shim/
4. Run a Linux Cuda app or FreeBSD cuda app (idk if it exists)
   env __NV_PRIME_RENDER_OFFLOAD=1 \
     __GLX_VENDOR_LIBRARY_NAME=nvidia \
     LD_PRELOAD=/usr/local/lib/cuda-shims/uvm_linux.so \
     ./path/to/blender (or just ./blender to the directory that is on)

   if you must run FreeBSD cuda app (in which it is not documented and not tested)
     You cant yet.
