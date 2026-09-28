# Milestone 01 — Building a Working TCP Server

> **Goal:** Build the networking base of `ft_irc`: start a TCP server, accept multiple clients, receive and send data without allowing one client to block the others, reconstruct complete IRC lines, and handle disconnections safely.

---

## What is a IRC?

**IRC (Internet Relay Chat)** is an application-layer, text-based communication protocol for real-time communication between users through IRC servers.

IRC is **not a particular chat application**. 

It defines rules, such as, message formats, commands, replies, and behavior, that different programs can implement so that they understand each other.

IRC follows an Client–Server architeture.
In the client–server model, each client establishes a connection to a central server. 
Clients do not normally send IRC chat messages directly to one another.

The **client** handles user interaction and sends requests. 

The **server** receives those requests, maintains shared information, and sends results to the relevant clients.

An IRC client and an IRC server are two different programs that follow those rules.

An **IRC server** is the program clients connect to. 
In `ft_irc`, we implement the server, not the client.

```text
User Ricardo                         User Pedro
     |                                    |
     v                                    v
IRC client A                         IRC client B
     |                                    |
     +----------- TCP connections --------+
                         |
                         v
                    Our IRC server
```

Clients do not normally exchange IRC chat messages directly. 
Each clients maintains a connection to the server, which will eventually interpret and route messages.

**Example:** Ricardo types a private message. His client sends the IRC line `PRIVMSG Pedro :Hello Pedro!\r\n` to the server. Later will identify `PRIVMSG`, locate Pedro, and forward a correctly formatted IRC message. **This milestone's job** is to accept the connections, receive the bytes, and reconstruct the complete line.

| Concept | Definition | Our project |
|---|---|---|
| **IRC** | The protocol: rules for commands, message structure, responses, and behavior. | We implement the required server rules. |
| **IRC client** | The program through which a user connects, sends commands, and sees messages. | We use an existing client for testing. |
| **IRC server** | The program that accepts connections, interprets commands, manages shared state, and routes messages. | **This is what we build.** |

Examples of IRC clients include HexChat and WeeChat. 
Their interfaces may differ, but they can communicate with compatible IRC servers because they follow the same protocol.

**Important distinction:** the *user* interacts with the *client*, not directly with our *server's* internal data structures. 
The *client* translates user actions into IRC messages that the *server* can understand.

## What are server responsabilities?

A complete IRC server has several responsibilities:

- **Connections:** accept clients and detect disconnections.
- **Registration:** authenticate clients and track their identities.
- **Commands:** receive, parse, validate, and execute requests.
- **Channels:** manage membership, topics, operators, and modes.
- **Communication:** deliver private and channel messages.

---

## What Is a Communication Protocol?

A **communication protocol** is a set of agreed rules for exchanging and interpreting information.

Think of two people speaking on cellphone: the connection lets them hear each other, but they still need a common language to understand what is being said. 
Likewise, a successful TCP connection does not automatically mean two programs understand each other's messages.

IRC provides that common language.

## IRC Uses Text-Based Messages

IRC uses textual messages with a defined structure. 
This makes it possible to inspect the data exchanged between clients and servers.
For example:

```irc
PRIVMSG Pedro :Hello Pedro!
```

This is not an arbitrary sentence: it is a structured IRC message. 
`PRIVMSG` is the command, `Pedro` is the target, and `:Hello Pedro!` is the message text.

IRC messages are terminated with `\r\n` (carriage return followed by line feed). 
This matters because our networking code must detect **complete lines** before passing them to the future IRC parser.

### Command-based: messages request actions

A command tells the server what the client wants to do.

| Command | Intended action |
|---|---|
| `NICK` | Set or change a nickname |
| `JOIN` | Join a channel |
| `PRIVMSG` | Send a private or channel message |
| `TOPIC` | View or change a channel topic |

The server doesn't simply execute every request. 
It identifies the command, validates its parameters and the current state, then performs the action or sends an error.

```text
Complete IRC line
        |
        v
  Identify command
        |
        v
  Extract parameters
        |
        v
 Validate rules/state
        |
        v
 Execute or send error
```

Our first milestone stops **before** this command-processing stage: it delivers complete lines that later code will parse.

### What does IRC define?

| Rule | What it means | Example |
|---|---|---|
| **Message format** | How a message is structured and terminated. | IRC lines end with `\r\n`. |
| **Commands** | Which actions a client can request. | `JOIN`, `NICK`, `PRIVMSG`. |
| **Parameters** | Information a command needs. | `JOIN #42` supplies a channel name. |
| **Responses** | How the server reports results or errors. | A successful action or an error reply. |
| **Behavior** | When actions are valid and how server state changes. | Joining a channel depends on its rules. |

**Compatibility** means independently developed programs can communicate because they follow the same rules. 
We must implement the required protocol behavior rather than invent our own message format.

**For now, our TCP server only needs to receive and reconstruct the complete command line correctly.**


## Application-layer: IRC and TCP have different jobs

The **application layer** defines the meaning of messages exchanged by applications. 
IRC belongs here because it defines chat-related commands and behavior.

**TCP (Transmission Control Protocol)** is a transport-layer protocol. 
It provides an ordered, reliable byte stream between connected endpoints, but it does not understand IRC commands.

```text
          IRC CLIENT                     IRC SERVER
      +----------------+             +----------------+
      | IRC protocol   |             | IRC protocol   |
      | Meaning/rules  |             | Meaning/rules  |
      +----------------+             +----------------+
              |                              ^
              v                              |
      +----------------+             +----------------+
      | TCP transport  | ----------> | TCP transport  |
      | Ordered bytes  |             | Ordered bytes  |
      +----------------+             +----------------+
```

**Remember:** TCP delivers bytes; **our application** identifies IRC line boundaries and, later, interprets those lines as commands.

| Layer / component | Responsibility | Example |
|---|---|---|
| **IRC (application layer)** | Defines the meaning and structure of chat messages | `JOIN #42\r\n` |
| **TCP (transport layer)** | Carries a reliable, ordered stream of bytes | Transports the bytes of `JOIN #42\r\n` |
| **IP (network layer)** | Addresses and routes packets between network interfaces | `127.0.0.1` for local testing |
| **Socket API** | Lets our C++ program use TCP through the operating system | `socket()`, `bind()`, `accept()`, `recv()` |

**Key distinction:** TCP does not know what `JOIN` or `NICK` means. Our application does not manually perform TCP retransmission or packet ordering: the operating system's TCP implementation handles that.

## How can one program communicate with many clients without getting stuck waiting for one of them?

Our server uses **one event loop** to coordinate a listening socket and several connected client sockets. It must not get stuck waiting for an idle or slow client.

An **event loop** repeatedly waits for events and handles them. 
Our server will use **one `poll()` - based event loop** to monitor the listening socket and all accepted client sockets.

`poll()` reports **readiness**: whether an operation may be possible without waiting. We then perform the appropriate operation and check its result.

| Event | Where | Possible action |
|---|---|---|
| `POLLIN` | Listening socket | `accept()` a pending connection |
| `POLLIN` | Client socket | `recv()` available bytes |
| `POLLOUT` | Client socket with queued output | `send()` pending bytes |
| Error/hangup events | Relevant socket | Inspect the condition and clean up when appropriate |

**Why non-blocking sockets?** Readiness doesn't guarantee that an operation can transfer everything we want. 
Non-blocking I/O allows an operation to return when it cannot proceed, so the event loop can continue serving other clients.

We do not need one thread or one blocking loop per client.

```text
                         ircserv process
                                |
                        Listening socket
                                |
                   poll() reports new connection
                                |
                             accept()
                                |
                  +-------------+-------------+
                  |             |             |
                Client A      Client B      Client C
                socket        socket        socket
                  |             |             |
                  +-------------+-------------+
                                |
                        One poll() loop
                                |
                    recv() / send() as ready
                                |
                    Separate buffers per client
```

Imagine a restaurant with one receptionist and a lot of customers. The reception desk is what we are calling `listening socket`. Each new costumer the receptionist welcomes is what we call an `accepted client socket`. `poll()` tells the receptionist which customer currently need something or attention. If one customer takes several minutes to decide, the receptionist should be able to help someone else rather than wait indefinitely. So, `Non-blocking I/O` prevents the receptionist from waiting indefinitely for one person.

Our server follows a similar principle.

| Networking concept | Meaning | Restaurant analogy |
|---|---|---|
| **Server process** | Our running `ircserv` program. | The restaurant. |
| **Listening socket** | A socket waiting for incoming connections. | Reception desk. |
| **Accepted client socket** | The server's endpoint of one TCP connection. | A separate communication path for each customer. |
| **File descriptor (fd)** | An integer handle for an open socket in our process. | A reference number for that communication path. |
| **`poll()`** | Reports which monitored sockets have relevant events. | Identifying who needs attention. |
| **Non-blocking I/O** | Socket operations don't wait indefinitely when they cannot proceed. | Not waiting on one inactive customer. |
| **Per-client buffers** | Store incoming and unsent bytes separately for each client. | Keeping each customer's requests separate. |

## The concepts

| Step | Concepts | Practical outcome |
|---|---|---|
| 1 | Command-line arguments, port ranges, passwords | Reject invalid startup configuration |
| 2 | IP addresses, ports, sockets, file descriptors, `socket()`, `setsockopt()`, `fcntl()`, `bind()`, `listen()` | Create a TCP listening socket |
| 3 | Blocking vs. non-blocking I/O, event loops, `poll()` and readiness | Monitor sockets in one loop |
| 4 | TCP connection establishment, listening vs. accepted sockets, `accept()` | Track multiple connections |
| 5 | TCP byte streams, `recv()`, input buffers | Store bytes independently per client |
| 6 | IRC message boundaries, `\r\n`, fragmented/combined reads | Extract complete IRC lines |
| 7 | `send()`, output queues, partial writes, `POLLOUT` | Transmit without losing queued bytes |
| 8 | EOF, socket errors, descriptor ownership, `close()` | Disconnect safely and release resources |

---

## Start and validate arguments

We run the server as `./ircserv <port> <password>` and reject missing or invalid arguments before attempting any network operations.

A **TCP port** is a 16-bit number used alongside an IP address and the TCP protocol to identify a local communication endpoint. 
Port numbers range from 0 to 65535, but **our project's validation deliberately accepts 1–65535**. 
Port 0 has a special operating-system meaning and is not a user-selected listening port in this design.

**Numeric validity is not availability.** Port `6667` can be syntactically valid but already occupied by another process. 
That is detected later by `bind()`.

---

## Create the listening socket

We create the server's network entry point, associate it with the chosen IP address and port, and tell the operating system to listen for incoming TCP connections. 
**We do not implement client handling yet.**

### IP address, port, socket, and file descriptor

An **IP address** identifies a network interface/address for communication and helps route network traffic.

A **port** identifies a TCP communication endpoint for a transport protocol on that interface/address. Together, `127.0.0.1:6667` describes where a local TCP client could contact our server.

A **socket** is an operating-system communication resource exposed through an API. It lets our C++ program use TCP without constructing IP packets or managing TCP acknowledgments itself.

A **file descriptor (fd)** is a non-negative integer used by our process to refer to an open resource, including a socket. An fd is **not** the port number. 
`6667` might be the port while `3` happens to be the fd.

| Concept | Example | Meaning |
|---|---|---|
| IP address | `127.0.0.1` | Local destination/interface |
| TCP port | `6667` | Service endpoint |
| Socket | OS-managed TCP socket | Resource used for communication |
| File descriptor | `3` | Process-local handle for that socket |
| TCP connection | Client endpoint ↔ server endpoint | Established bidirectional byte stream |

```text
IRC client                                  ircserv
    |                                          |
Client socket ---- TCP connection ---- Accepted client socket
                                               ^
                                               |
                                    Created later by accept()
                                               |
                                         Listening socket
                                         127.0.0.1:6667
```

### Why do we need a listening socket?

A **listening socket** is the server's entry point for *new connections*. 
It is not used to exchange every client's chat messages. 
Later, each successful `accept()` returns a **different, connected socket** representing one client's connection. 
The listening socket stays available for more clients.

| Listening socket | Accepted client socket |
|---|---|
| Created using `socket()`, then `bind()` and `listen()` | Returned by `accept()` |
| Waits for incoming connections | Exchanges bytes with one client |
| Usually one for our chosen address/port | One per accepted client |
| Later watched for `POLLIN` to call `accept()` | Later watched for reading and queued writing |


```text
./ircserv 6667 secret
         |
         v
 Validate arguments
 	     |
 	     v
   	 socket()                   Create an IPv4 TCP socket
 	     |
 	     v
 	setsockopt()                Configure optional address reuse
 	     |
 	     v
 	  fcntl()                   Enable non-blocking mode
 	     |
 	     v
 	   bind()                   Assign local IP address and port
 	     |
 	     v
 	  listen()                  Allow incoming connection requests
 	     |
 	     v
 Listening socket ready 
 	     |
 	     v
  poll() / accept()             Wait for relevant socket events
```

When a new connection is ready, the server calls `accept()`. 
That operation returns **a new socket** for the individual client. 
The listening socket remains available for future connections.

```text
                       SERVER PROCESS
                             |
                      Listening socket
                          (fd 3)
                             |
                   New connections arrive
                             |
             +---------------+---------------+
             |               |               |
          accept()        accept()        accept()
             |               |               |
             v               v               v
         Client A        Client B        Client C
          fd 4            fd 5            fd 6
```

The fd numbers are examples; the operating system assigns them.

The precise placement of `fcntl()` can vary, but the socket must be non-blocking before it is used by our event loop.

### `socket()`

```cpp
int serverFd = socket(AF_INET, SOCK_STREAM, 0);
```

| Argument | Meaning |
|---|---|
| `AF_INET` | IPv4 address family |
| `SOCK_STREAM` | Stream socket |
| `0` | Default protocol for this combination: TCP |

On success, `socket()` returns an fd (`0` or greater). 
On failure, `-1`. 
**The socket is not listening yet.** Check its return value before we using it.

### 2.5 `setsockopt()` — configure address reuse

During development, we will frequently stop and restart the server. 
`SO_REUSEADDR` allows certain forms of local-address reuse that make restarting easier.
`SOL_SOCKET` selects the socket-level option. `SO_REUSEADDR` names the option; `option = 1` enables it. Check for `-1` and clean up on failure.

**Important:** This does not generally let two ordinary TCP listeners bind the exact same address and port simultaneously. The next `bind()` can still fail.

### 2.6 `fcntl()` — make the socket non-blocking

A blocking operation can wait for an event. 
That is dangerous in our later single-threaded event loop because one client could prevent others from being served.

```cpp
int flags = fcntl(serverFd, F_GETFL, 0);
```

`F_GETFL` reads the existing file-status flags. 
`F_SETFL` updates them, and `O_NONBLOCK` requests non-blocking operation. We preserve the previous flags by using `flags | O_NONBLOCK`.

**Non-blocking does not mean an operation always succeeds.** 
If no operation is currently possible, it may return `-1` with `errno` set to `EAGAIN` or `EWOULDBLOCK`. Later we combine non-blocking sockets with `poll()`.

### 2.7 `bind()` — choose the local address and port

After creating a socket, we must associate it with the local endpoint clients will contact. For IPv4 we use `sockaddr_in`.

| Field or function | Purpose |
|---|---|
| `sin_family` | Select IPv4 |
| `sin_port` | Store the TCP port in network byte order |
| `htons(port)` | Convert a 16-bit host-order value to network order |
| `sin_addr.s_addr` | Select the local IPv4 address |
| `htonl(INADDR_ANY)` | Listen on all suitable local IPv4 interfaces |

For local-only testing, use the loopback address instead of `INADDR_ANY`. `bind()` can fail when the address/port is already occupied or for other OS-level reasons.

### 2.8 `listen()` — allow incoming connections

`listen()` changes the bound socket into a listening socket. The **backlog** parameter controls the pending-connection queue, subject to OS limits. It is **not** the maximum number of clients your IRC server can eventually manage.

After `listen()` succeeds, the OS can handle incoming TCP connection establishment and queue completed connections. Your program still needs `accept()` later to retrieve them.


---

## Step 3 — Add the event loop

### Goal

Build **one `poll()`-based event loop** that monitors the listening socket and, later, all connected client sockets. The server should remain responsive without one thread per client.

### 3.1 Blocking vs. non-blocking I/O

With **blocking I/O**, `recv()` can wait for a client that has not sent anything. While waiting, a single-threaded server cannot serve its other clients.

With **non-blocking I/O**, an operation that cannot proceed immediately returns instead of waiting. This lets our program handle another ready socket.

```text
Blocking design:                   Non-blocking + poll():

recv(Client A)                     poll(all sockets)
    |                                  |
    | waits for A                      +--> A ready? recv(A)
    |                                  |
Client B must wait                    +--> B ready? recv(B)
                                       |
                                       +--> New connection? accept()
```

Non-blocking calls may return `EAGAIN`/`EWOULDBLOCK`; that means *try again when ready*, not necessarily *disconnect the client*.

### 3.2 What is an event loop?

An **event loop** repeatedly waits for events and handles whichever sockets need attention. In our design, the loop uses `poll()`.

A `pollfd` stores a descriptor, the events we want to monitor, and the events that occurred:

| Member | Meaning |
|---|---|
| `fd` | Socket descriptor to monitor |
| `events` | Requested events, such as `POLLIN` |
| `revents` | Reported events after `poll()` returns |

| Event | Interpretation | Typical action |
|---|---|---|
| `POLLIN` on listening socket | Connection may be ready | `accept()` |
| `POLLIN` on client socket | Bytes/EOF may be available | `recv()` |
| `POLLOUT` on client socket | Sending may be possible | Flush queued output |
| `POLLERR` / `POLLHUP` / `POLLNVAL` | Error, hangup, or invalid fd | Inspect and clean up as appropriate |

`poll()` reports **readiness, not guaranteed success**. Always check the actual `accept()`, `recv()`, and `send()` results. Keep those operations non-blocking.

### 3.3 Why one `poll()` loop?

The same loop can coordinate many connections, including clients that are idle, sending, or slow to receive. There is no need to create one blocking thread for each client.

```text
             +-----------------------+
             |       poll()          |
             +-----------+-----------+
                         |
            +------------+------------+
            |            |            |
       Listener      Client A      Client B
        POLLIN        POLLIN        POLLOUT
            |            |            |
         accept()      recv()       send()
            |            |            |
            +------------+------------+
                         |
                    Repeat loop
```

---

## Step 4 — Accept multiple clients

### Goal

When `poll()` reports that the listening socket is ready, call `accept()` and track the newly connected client.

### 4.1 TCP connection vs. IRC registration

A TCP connection is a **transport-layer connection** between client and server. IRC registration is a **later application-layer process** involving commands such as `PASS`, `NICK`, and `USER`.

```text
Client opens TCP connection
            |
            v
Server accepts client socket          [Milestone 01]
            |
            v
Client sends IRC registration         [Later milestone]
            |
            v
Server validates and registers user
```

A connected client is **not automatically a registered IRC user**.

### 4.2 What does `accept()` return?

`accept()` returns a **new fd** for one accepted connection. The listening socket remains open and can accept further connections.

```text
                  Listening socket (example fd 3)
                              |
                    poll() reports POLLIN
                              |
                           accept()
                              |
                     New client socket
                        example fd 4

Another connection -> accept() -> example fd 5
Another connection -> accept() -> example fd 6
```

The operating system chooses the fd values; they are not fixed and are not IRC nicknames.

### 4.3 Track each connection

A simple C++98 design uses:

```cpp
std::vector<struct pollfd> _pollFds;
std::map<int, Client>      _clients;
```

The vector holds descriptors/events for `poll()`. The map associates each accepted fd with that client's state. A client object can later hold separate input/output buffers and IRC identity information.

Configure accepted sockets appropriately for non-blocking operation and add them to the event loop. Decide which object owns and closes each fd.

---

## Step 5 — Read into per-client input buffers

### Goal

Receive available bytes from ready client sockets and append them to **the correct client's input buffer**.

### 5.1 What is a TCP byte stream?

TCP provides an **ordered stream of bytes**, not a sequence of IRC messages. It preserves byte order, but it does **not** preserve the boundaries between your application's writes or commands.

For example, a client sends:

```text
NICK Ricardo\r\n
```

The server might observe:

| `recv()` call | Bytes received |
|---|---|
| 1 | `"NICK Ric"` |
| 2 | `"ardo\r\n"` |

Or a single `recv()` might return **multiple complete lines**. Both behaviors are valid. Never assume that one `send()` by the client corresponds to one `recv()` by the server.

### 5.2 Why separate input buffers?

Each client has an independent stream. If Client A sends half a command, those bytes must remain with Client A and must not delay or mix with Client B's data.

```text
Client A socket --> A input: "NICK Ric"
Client B socket --> B input: "JOIN #42\r\n"
Client C socket --> C input: ""
```

Read available bytes when `poll()` reports appropriate readiness, append them to that client's buffer, and preserve incomplete content for later.

---

## Step 6 — Extract complete IRC lines

### Goal

Turn buffered TCP bytes into complete IRC lines. **Do not implement the full IRC command parser yet.**

### 6.1 Why `\r\n` matters

IRC lines are terminated with **CRLF**, written as `\r\n` in C++ string notation. `\r` is carriage return; `\n` is line feed. This delimiter tells us where a complete IRC message ends.

```text
Input buffer:
"NICK Ricardo\r\nJOIN #42\r\nPRIV"

Extract:
1. "NICK Ricardo"
2. "JOIN #42"

Keep:
"PRIV"
```

The remaining `"PRIV"` must stay in that client's buffer until more bytes arrive.

### 6.2 Fragmented and combined reads

| Situation | What to do |
|---|---|
| `"NICK Ric"` | Store; no complete line yet |
| Later `"ardo\r\n"` | Combine and extract `NICK Ricardo` |
| `"NICK A\r\nUSER a 0 * :A\r\n"` | Extract two separate lines |
| `"JOIN #42\r"` | Store; wait for `\n` |
| `"\n"` after previous `"\r"` | Complete the line |

```text
recv() bytes
     |
     v
Append to this client's input buffer
     |
     v
Contains \r\n ?
   /       \
 no         yes
 |           |
Keep        Extract one line
waiting      |
             v
       More complete lines?
          /       \
         yes       no
          |         |
       Extract     Keep trailing bytes
```

---

## Step 7 — Prepare output buffering

### Goal

Queue outgoing bytes for each client and send them when their socket is ready, without losing data during partial writes.

### 7.1 Why `send()` may not send everything

A non-blocking `send()` can transmit fewer bytes than requested. It can also report that sending is temporarily unavailable. This does **not** mean the rest of the message should be discarded.

**Example:** We want to send 20 bytes, but `send()` reports that only 8 were sent.

```text
Output buffer: [--------- 20 bytes ---------]
Sent:          [ 8 bytes ]
Remaining:               [--- 12 bytes ----]
```

Remove only the 8 successfully sent bytes. Keep the remaining 12 queued.

### 7.2 When to monitor `POLLOUT`

Monitor writable readiness **when a client actually has pending output**. Otherwise, writable sockets are often reported repeatedly, which can cause unnecessary wake-ups.

```text
Generate outgoing bytes
          |
          v
Append to client's output buffer
          |
          v
Enable/monitor POLLOUT
          |
          v
poll() reports writable
          |
          v
send() some queued bytes
          |
          +--> bytes remain: keep POLLOUT
          |
          +--> queue empty: stop monitoring POLLOUT
```

---

## Step 8 — Handle disconnections and errors

### Goal

Remove clients safely when they disconnect or suffer an unrecoverable socket error. Never let one client's departure crash the server or disrupt other clients.

### 8.1 Normal disconnection vs. temporary unavailability

| Condition | Meaning | General response |
|---|---|---|
| `recv()` returns `> 0` | Bytes received | Append/process |
| `recv()` returns `0` | Peer has performed an orderly shutdown of its sending side | Handle EOF and connection lifecycle |
| `recv()` returns `-1` with `EAGAIN`/`EWOULDBLOCK` | No data currently available | Wait for later readiness |
| `recv()` returns `-1` with `EINTR` | Interrupted by signal | Handle/retry appropriately |
| Unrecoverable error | Connection can no longer be used safely | Clean up the client |

EOF on receiving does not, by itself, always mean the peer can no longer receive: TCP supports half-close. Choose and document your server's lifecycle policy.

### 8.2 Safe cleanup order

Conceptually:

```text
Client disconnects / unrecoverable error
                 |
                 v
Identify the correct client fd
                 |
                 v
Stop monitoring the fd in poll()
                 |
                 v
Remove associated Client state
                 |
                 v
Close the owned socket exactly once
                 |
                 v
Continue serving remaining clients
```

The precise implementation order can vary with your containers and ownership model. Avoid invalidating iterators while traversing a `vector` or `map`, and never keep monitoring a closed fd.

### 8.3 What about startup errors?

Startup failures from Step 2 follow the same ownership principle: close any socket you successfully created, report the failure, and stop startup. Shutdown should release the listening socket and every accepted client socket still owned by the server.


---


**Milestone 01 is complete when the networking foundation is reliable, not when full IRC commands work.** Once complete IRC lines can be received independently from multiple clients and output can be sent safely, we are ready to build registration (`PASS`, `NICK`, `USER`) in the next milestone.
