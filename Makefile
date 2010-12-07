#
# TCP Proxy Server
# ~~~~~~~~~~~~~~~~~~~
#
# Copyright (c) 2007 Arash Partow (http://www.partow.net)
# URL: http://www.partow.net/programming/tcpproxy/index.html
#
# Distributed under the Boost Software License, Version 1.0.
#


COMPILER         = -c++
OPTIMIZATION_OPT = -O3
OPTIONS          = -pedantic -ansi -Wall -Werror $(OPTIMIZATION_OPT) -o
PTHREAD          = -lpthread
LINKER_OPT       = -L/usr/lib -lstdc++ $(PTHREAD) -lboost_thread -lboost_system

BUILD_LIST+=tcpproxy_server

all: $(BUILD_LIST)

tcpproxy_server: tcpproxy_server.cpp
	$(COMPILER) $(OPTIONS) tcpproxy_server tcpproxy_server.cpp $(LINKER_OPT)

strip_bin :
	strip -s tcpproxy

clean:
	rm -f core *.o *.bak *~ *stackdump *#
