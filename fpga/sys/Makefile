

ifneq ($(KERNELRELEASE),)

EXTRA_CFLAGS += -Wno-declaration-after-statement

obj-m += slf_fpga.o

else

# When called directory from the command line, we figure out the
# location of the kernel source, and recurse to the kernel build
# context do to the actual building.
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif
