# 定义编译时的路径
PWD := $(shell pwd)

obj-m += myioctl_driver.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules \
		CFLAGS_myioctl_driver.o="-DPERSIST_FILE_PATH='\"$(PWD)/myioctl_count\"'"
	gcc -o myioctl_app myioctl_app.c
	gcc -o myioctl_app_reset myioctl_app_reset.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f myioctl_app
	rm -f myioctl_app_reset
