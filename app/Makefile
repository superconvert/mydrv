all: write read mmap unicast multicast namespace

write:
	gcc -o write write.c

read:
	gcc -o read read.c

mmap:
	gcc -o mmap mmap.c

unicast:
	gcc -o unicast netlink_unicast.c

multicast:
	gcc -o multicast netlink_multicast.c

namespace:
	gcc -o namespace netlink_namespace.c

clean:
	rm write read mmap unicast multicast namespace -rf
