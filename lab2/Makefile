obj-m += myioctl_driver.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc -o myioctl_app myioctl_app.c
	gcc -o myioctl_app_reset myioctl_app_reset.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm myioctl_app
	rm myioctl_app_reset
