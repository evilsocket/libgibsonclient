Gibson ( client library )
===

Gibson is a high efficiency, tree based memory cache server. It is not meant to replace a database, since it was written to be a key-value store to be used as a cache server, but it's not the usual cache server.
Normal key-value stores ( memcache, redis, etc ) uses a hash table as their main data structure, so every key is hashed with a specific algorithm and the resulting hash is used to identify the given value in memory. This approach, although very fast, doesn't allow the user to execute globbing expressions/selections with on a given keyset, thus resulting on a pure one-by-one access paradigm.
Gibson is different, it uses a tree based structure allowing the user to perform operations on multiple key sets using a prefix expression achieving the same performance grades

<http://gibson-db.in/>  

Features
---
* Very fast and with the lowest memory footprint possible
* Fast LZF object compression
* Builtin object Time-To-Live
* Cached object locking and unlocking
* Multiple key set operation with M* operators 


Compilation / Installation
---
In order to compile libgibsonclient, you will need cmake and autotools installed, then:

    $ git clone https://github.com/evilsocket/libgibsonclient.git
    $ cd libgibsonclient
    $ git submodule update --init
    $ cmake .
    $ make
    # make install

Client Usage
---
First of all, start with a gibson-cli -h to get the following help menu.

    gibson-cli [-h|--help] [-a|--address ADDRESS] [-p|--port PORT] [-u|--unix UNIX_SOCKET_PATH] [-t|--timeout TIMEOUT]
        -h, --help            	    Print this help and exit.
        -a, --address ADDRESS   	TCP address of Gibson instance.
        -p, --port PORT   		    TCP port of Gibson instance.
        -u, --unix UNIX_SOCKET_PATH Unix socket path of Gibson instance ( overrides TCP arguments ).
        -t, --timeout               Timeout in milliseconds of the socket ( default to 1000 ).

Let's say you want to connect to your local Gibson UNIX domain socket.

    gibson-cli -u /var/run/gibson.sock

Then, use the ':help' ( or just ':h' ) shortcut to get a list of available commands.

    local> :h
        SET <ttl> <key> <value>
        TLL <key> <ttl>
        GET <key>
        DEL <key>
        INC <key>
        DEC <key>
        LOCK <key> <seconds>
        UNLOCK <key>
        MSET <prefix> <value>
        MTTL <prefix> <ttl>
        MGET <prefix>
        MDEL <prefix>
        MINC <prefix>
        MDEC <prefix>
        MLOCK <prefix> <seconds>
        MUNLOCK <prefix>
        COUNT <prefix>
        STATS
        PING
        SIZEOF <key>
        MSIZEOF <prefix>
        ENCOF <key>

You can now start to use the client.

License
---

Released under the BSD license.  
Copyright &copy; 2013, Simone Margaritelli <evilsocket@gmail.com>  
All rights reserved.
