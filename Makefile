#based on SilkJS Makefile

ARCH := $(shell getconf LONG_BIT)

COREMQ= mqdb.o
OBJ=
HMQ = include/lru_cache/lru_cache.h include/lru_cache/scoped_mutex.h fstools.cc

CORECL= client.o
OBJ=

V8DIR=	./v8-read-only

V8LIB_64 := $(V8DIR)/out/x64.release/obj.target/tools/gyp
V8LIB_32 := $(V8DIR)/out/ia32.release/obj.target/tools/gyp
V8LIB_DIR := $(V8LIB_$(ARCH))

V8VERSION_64 := x64.release
V8VERSION_32 := ia32.release
V8VERSION := $(V8VERSION_$(ARCH))

V8=	$(V8LIB_DIR)/libv8_base.a $(V8LIB_DIR)/libv8_snapshot.a

CFLAGS = -O6 -fomit-frame-pointer -fdata-sections -ffunction-sections -fno-strict-aliasing -fno-rtti -fno-exceptions -fvisibility=hidden -Wall -W -Wno-unused-parameter -Wnon-virtual-dtor -m$(ARCH) -O3 -fomit-frame-pointer -fdata-sections -ffunction-sections -ansi -fno-strict-aliasing -fexceptions

all: mqdb client mqdb2 tasksink taskvent taskvs asclient

%.o: %.cpp Makefile
	g++ $(CFLAGS) -c -I./include -I$(V8DIR)/include -g -o $*.o $*.cpp

mqdb:	$(V8) $(COREMQ) $(OBJ) $(HMQ) Makefile
	g++ $(CFLAGS) -I./include -I$(V8DIR)/include -o mqdb mqdb.cpp -L$(V8LIB_DIR)/ -L./leveldb/ -lv8_base -lv8_snapshot -lmm -lpthread -lzmq -lleveldb

#-lgd

client:	$(V8) $(CORECL) $(OBJ) Makefile
	g++ $(CFLAGS) -o client $(CORECL) $(OBJ) -L$(V8LIB_DIR)/ -lmm -lpthread -lzmq
	
$(V8):
	cd $(V8DIR) && make dependencies && GYP_GENERATORS=make make $(V8VERSION)

update:
	cd $(V8DIR) && svn update && make dependencies && GYP_GENERATORS=make make $(V8VERSION)
	git pull
	
