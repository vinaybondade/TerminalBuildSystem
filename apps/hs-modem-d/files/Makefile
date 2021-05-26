NUM_OF_CORE = 16

obj-m := hsmodem.o \
         lowrate.o \
		 hsfifo.o

hsmodem-y := hs-modem.o \
             air_interface.o \
			 hs_modem_rate.o \
			 hs_modem_dbg.o

lowrate-y := low_rate.o

hsfifo-y := hs-fifo.o

SRC := $(shell pwd)

all:
	$(MAKE) -j$(NUM_OF_CORE) -C $(KERNEL_SRC) M=$(SRC) modules

modules_install:
	$(MAKE) -j$(NUM_OF_CORE) -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers
