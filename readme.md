#### Description
The C++ TCP Proxy server is a simple utility using the ASIO networking library,
for proxying (tunneling or redirecting) connections from external clients to a
specific server. The TCP Proxy server can be used to easily and efficiently:

+ Limit the number of client connections to the server
+ Load balance client connections between multiple server instances
+ Provide IP or connection time based filtering and access control mechanisms
+ Inspect (log), filter or otherwise modify data flowing between the clients and the server


![ScreenShot](http://www.partow.net/images/tcpproxy_server_diagram.png?raw=true "TCP Proxy Server Diagram - Copyright Arash Partow")


#### Download
http://www.partow.net/programming/tcpproxy/index.html


#### Compatibility
The C++ TCP Proxy server implementation is compatible with the following C++
compilers:

* GNU Compiler Collection (4.1+)
* Intel® C++ Compiler (9.x+)
* Clang/LLVM (1.1+)
* PGI C++ (10.x+)
* Microsoft Visual Studio C++ Compiler (8.1+)
* IBM XL C/C++ (10.x+)


----


#### Internals Of The Proxy
The proxy from an implementation aspect is primarily composed of three
components named the **Acceptor**, **Session** and the ASIO **I/O Service**
proactor. The acceptor and session components register with the I/O service
requests and associated completion handlers (callbacks) for reading and writing
from socket(s). The state diagram below depicts the the various completion
handlers and their relationship to the I/O service component. For exposition
purposes let's assume that the completion handlers and the I/O service component
are each a unique state in a state machine that represents the TCP proxy.


![ScreenShot](http://www.partow.net/images/tcpproxy_state_diagram.png?raw=true "TCP Proxy State Diagram - Copyright Arash Partow")


The TCP proxy server is broken down into three functional *'groupings'*
denoted in the diagram by the colours blue, green and red attached to
the transitions between the states (completion handlers) and the I/O service.
The groupings are summarised as follows:

|  Phase  |  Transitions                         |  Definition                                              |
| :-------| :----------------------------------: | :------------------------------------------------------- |
| Blue    | 1 - 8                                | Start-up and client connection instantiation phase.      |
| Green   | A<sub>1</sub> - A<sub>4</sub>        | Process data flow from remote server to proxy to client. |
| Red     | B<sub>1</sub> - B<sub>4</sub>        | Process data flow from client to proxy to remote server. |


#### The Blue Phase - Startup and Initialisation
In this phase the proxy itself is setup, which includes instantiating the
acceptor, binding-to and listening in on the given IP and port number, and
invoking the **accept_connections** method, which in turn will register a
completion handler with the I/O service, that will later on be invoked when
new connections are made to the proxy server.

When a client makes a connection to the proxy server, the **handle_accept**
completion handler will be invoked by the I/O service. This handler will then
proceed to instantiate and start a client session (bridge) instance. Once that
is complete, it will then invoke **accept_connections** which will complete
the cycle by re-registering the **handle_accept** method with the I/O service
as the completion handler for any new connections.

Meanwhile when the start method on the client session was invoked during the
**handle_accept** call, it immediately attempted to asynchronously establish
a connection with the remote server. When the remote server accepts the
connection, the I/O service will then invoke the **handle_upstream_connect**
completion handler. This handler will in turn proceed to register two
asynchronous read requests coupled with the completion handlers
**handle_downstream_read** and **handle_upstream_read** with the I/O service,
one for data coming from the client, the other being for data coming from
the remote server respectively.


![ScreenShot](http://www.partow.net/images/tcpproxy_state_bluephase_diagram.png?raw=true "TCP Proxy Blue Phase Diagram - Copyright Arash Partow")


Based on which-end point data arrives at the proxy, one of the following
phases will be engaged:

1. Green Phase
2. Red Phase


#### The Green Phase - Remote Server To Proxy To Client Data Flow
This phase is engaged when data from the Remote Server *(aka up-stream end point)*
arrives at the proxy. Once some amount of data is ready, the I/O service will
invoke the **handle_upstream_read** completion handler. This handler will in turn
take the data and register an asynchronous write request with the I/O service in
order to send the data to the Client end point. Once the write request has
completed, the I/O service will invoke the **handle_downstream_write** completion
handler. This handler will complete the cycle for the green phase by re-registering
with the I/O service an asynchronous read request from the upstream end-point coupled
with the **handle_upstream_read** method as the associated completion handler.


![ScreenShot](http://www.partow.net/images/tcpproxy_state_greenphase_diagram.png?raw=true "TCP Proxy Green Phase Diagram - Copyright Arash Partow")


#### The Red Phase - Client To Proxy To Remote Server Data Flow
This phase is engaged when data from the Client (aka down-stream end point)
arrives at the proxy. Once some amount of data is ready, the I/O service will
invoke the **handle_downstream_read** completion handler. This handler will
in turn take the data and register an asynchronous write request with the I/O
service in order to send the data to the Remote Server end point. Once the
write request has been completed, the I/O service will invoke the **handle_upstream_write**
completion handler. This handler will complete the cycle for the red phase by
re-registering with the I/O service an asynchronous read request from the
downstream end-point coupled with the **handle_downstream_read** method as the
associated completion handler.


![ScreenShot](http://www.partow.net/images/tcpproxy_state_redphase_diagram.png?raw=true "TCP Proxy Red Phase Diagram - Copyright Arash Partow")


#### Bridge Shutdown Process
When either of the end points terminate their respective connection to the
proxy, the proxy will proceed to close (or shutdown) the other corresponding
connection. This includes releasing any outstanding asynchronous requests,
culminating in the reference count of the bridge (client session) reaching zero
at which point the bridge instance itself will subsequently have its destructor
called.
