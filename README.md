name: Emergency Network Client-Server System

description: >
  A distributed emergency network simulation system with a Java multithreaded server and a C++ event-driven STOMP client.

features:
  - Full STOMP networking protocol implementation
  - Non-blocking Java server (Reactor pattern)
  - Event-driven C++ client with concurrent data structures
  - JSON parsing support

technologies:
  - Java 17+
  - Maven
  - C++11
  - Makefile

structure: |
  .
  ├── server/
  │   ├── pom.xml
  │   └── src/main/java/bgu/spl/net/...
  ├── client/
  │   ├── makefile
  │   ├── src/
  │   ├── include/
  │   ├── bin/
  │   └── data/
  └── README.md
  
  build: 
  
  Server
  
  cd server
  
  mvn clean install



  Client
  
  cd client
  
  make


run: 

  Server
  
  cd server 
  
  mvn exec:java -Dexec.mainClass="bgu.spl.net.impl.stomp.StompServer"


  Run Client
  
  cd client/bin
  
  ./StompClient <host> <port>


notes:
  - Server uses asynchronous Reactor pattern.
  - Client processes real-time events using STOMP protocol.
