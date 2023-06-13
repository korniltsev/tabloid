obj-m += tabloid.o

all: module user


module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

user: tabloid-user.c
	gcc -o tabloid-user tabloid-user.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf tabloid-user