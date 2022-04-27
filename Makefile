KDIR = /lib/modules/$(shell uname -r)/build
TARGET_MODULE = kecho
CFLAGS_user = -std=gnu99 -Wall -Wextra -Werror

obj-m += $(TARGET_MODULE).o
$(TARGET_MODULE)-objs := \
    kecho_mod.o \
    echo_server.o

obj-m += drop-tcp-socket.o

GIT_HOOKS := .git/hooks/applied
all: $(GIT_HOOKS) bench user-echo-server
	make -C $(KDIR) M=$(PWD) KBUILD_VERBOSE=$(VERBOSE) modules

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

check: all
	@scripts/test.sh

bench: bench.c
	$(CC) -o $@ $(CFLAGS_user) -pthread $<

plot:
	gnuplot scripts/bench.gp

user-echo-server: user-echo-server.c
	$(CC) -o $@ $(CFLAGS_user) $<

clean:
	make -C $(KDIR) M=$(PWD) clean
	$(RM) user-echo-server bench bench.txt

load:
	sudo insmod $(TARGET_MODULE).ko

unload:
	sudo rmmod $(TARGET_MODULE) || true > /dev/null

