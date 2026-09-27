# Milestone 01 — Building a Working TCP Server

> **Goal:** Build the networking base of `ft_irc`: start a TCP server, accept multiple clients, receive and send data without allowing one client to block the others, reconstruct complete IRC lines, and handle disconnections safely.
>
> **Scope:** We are **not yet implementing** `PASS`, `NICK`, `USER`, channels, or permissions. Those features will use the networking foundation built here.

Before writing code, we need two perspectives:

1. **IRC perspective:** What are we building, and how should clients and servers communicate?
2. **Networking perspective:** How can our program maintain several TCP connections and reliably receive and send data?

Understanding the first helps us make the right architectural decisions for the second.

---

# Part I — Understanding What We Are Building

## 1. What Is IRC?

**IRC (Internet Relay Chat)** is a text-based communication protocol for real-time communication between users through IRC servers.

IRC is **not a particular chat application**. 
It defines rules that different programs can implement so that they understand each other. 
An IRC client and an IRC server are two different programs that follow those rules.

### 1.1 IRC vs. IRC client vs. IRC server

| Concept | Definition | Our project |
|---|---|---|
| **IRC** | The protocol: rules for commands, message structure, responses, and behavior. | We implement the required server rules. |
| **IRC client** | The program through which a user connects, sends commands, and sees messages. | We use an existing client for testing. |
| **IRC server** | The program that accepts connections, interprets commands, manages shared state, and routes messages. | **This is what we build.** |

Examples of IRC clients include HexChat and WeeChat. 
Their interfaces may differ, but they can communicate with compatible IRC servers because they follow the same protocol.

```text
         User A                              User B
           |                                   |
           v                                   v
      IRC Client A                        IRC Client B
           |                                   |
           |          IRC over TCP             |
           +--------------+--------------------+
                          |
                          v
                      IRC Server
```

**Important distinction:** the *user* interacts with the *client*, not directly with our *server's* internal data structures. 
The *client* translates user actions into IRC messages that the *server* can understand.

### 1.2 What will our server eventually do?

A complete IRC server has several responsibilities:

- **Connections:** accept clients and detect disconnections.
- **Registration:** authenticate clients and track their identities.
- **Commands:** receive, parse, validate, and execute requests.
- **Channels:** manage membership, topics, operators, and modes.
- **Communication:** deliver private and channel messages.

Only the **connection and data-transfer foundation** belongs to this milestone.

---

## 2. What Is a Communication Protocol?

A **communication protocol** is a set of agreed rules for exchanging and interpreting information.

Think of two people speaking on cellphone: the connection lets them hear each other, but they still need a common language to understand what is being said. 
Likewise, a successful TCP connection does not automatically mean two programs understand each other's messages.

IRC provides that common language.

### 2.1 What does IRC define?

| Rule | What it means | Example |
|---|---|---|
| **Message format** | How a message is structured and terminated. | IRC lines end with `\r\n`. |
| **Commands** | Which actions a client can request. | `JOIN`, `NICK`, `PRIVMSG`. |
| **Parameters** | Information a command needs. | `JOIN #42` supplies a channel name. |
| **Responses** | How the server reports results or errors. | A successful action or an error reply. |
| **Behavior** | When actions are valid and how server state changes. | Joining a channel depends on its rules. |

**Compatibility** means independently developed programs can communicate because they follow the same rules. 
We must implement the required protocol behavior rather than invent our own message format.

Imagine a user selecting channel `#42` in an IRC client. The client sends:

```irc
JOIN #42
```

The server must understand the command, extract its parameter, and check whether the action is permitted.

```text
User chooses #42
       |
       v
   IRC Client
       |
       |  JOIN #42
       v
   IRC Server
       |
       v
 Parse command and parameters
       |
       v
 Validate channel and permissions
       |
       +---- Allowed ---> Update membership and notify clients
       |
       +---- Denied ----> Send an appropriate error
```

**For now, our TCP server only needs to receive and reconstruct the complete command line correctly.**

---

## 3. Why Is IRC Text-Based, Command-Based, and Application-Layer?

These three descriptions explain **how IRC messages are represented**, **what they mean**, and **where IRC fits into network communication**.

### 3.1 Text-based: readable message representation

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

### 3.2 Command-based: messages request actions

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

### 3.3 Application-layer: IRC and TCP have different jobs

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

---

## 4. How Does IRC's Client–Server Architecture Work?

In the client–server model, each client establishes a connection to a central server. 
Clients do not normally send IRC chat messages directly to one another.

The **client** handles user interaction and sends requests. 
The **server** receives those requests, maintains shared information, and sends results to the relevant clients.

### 4.1 What information does the server maintain?

The server's **internal state** is the information it currently stores about the system. 
Eventually, it will include connected clients, nicknames, channel memberships, operators, topics, and modes.

During Milestone 01, we only need **connection state**: which sockets are connected and what incoming or outgoing bytes belong to each connection.

### 4.2 Example: a private message

Suppose Ricardo wants to send Pedro a message:

```irc
PRIVMSG Pedro :Hello Pedro!
```

```text
Ricardo's client           IRC Server             Pedro's client
       |                       |                         |
       |--- PRIVMSG Pedro ---->|                         |
       |                       |                         |
       |                 Locate Pedro                    |
       |                       |                         |
       |                       |------ Message --------->|
       |                       |                         |
```

Ricardo's client sends the message **to the server**. The server finds Pedro's connection and forwards the appropriate IRC message.

This shows why our server needs to manage multiple connections at once: receiving from one client and sending to another are separate network operations.

---

# Part II — Understanding Our First Milestone

## 5. The Complete Picture

Now we know what the final IRC server will do. 
The next question is: **how can one program communicate with many clients without getting stuck waiting for one of them?**

Imagine a restaurant with one receptionist. 
The receptionist welcomes new customers and attends to customers who need something. 
If one customer takes several minutes to decide, the receptionist should be able to help someone else rather than wait indefinitely.

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

### 5.1 From starting the program to accepting clients

We start the server with a port and password:

```bash
./ircserv 6667 password
```

The password is for later IRC registration. 
For now, the server creates a **listening socket** and associates it with a local IP address and the requested port.

- An **IP address** identifies a network interface and helps route network traffic.
- A **port** identifies a TCP communication endpoint on that interface.
- A **socket** is an operating-system resource through which a program performs network communication.
- A **file descriptor** is the integer our program uses to refer to that socket.

The basic startup sequence is:

```text
Start ircserv
      |
      v
socket()  -> Create a TCP socket
      |
      v
fcntl()   -> Configure non-blocking mode
      |
      v
bind()    -> Associate local address and port
      |
      v
listen()  -> Allow incoming connection requests
      |
      v
poll()    -> Wait for relevant socket events
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

### 5.2 One event loop for all connections

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

### 5.3 Why each client needs its own buffers

TCP is a **byte stream**. 
It delivers bytes in order, but it does not preserve the boundaries of application messages.
Suppose a client sends:

```text
NICK Ricardo\r\n
```

Our server might receive it in two parts:

```text
recv() #1: "NICK Ric"
recv() #2: "ardo\r\n"
```

We must not process the first part as a complete command. 
Instead, we append received bytes to that client's **input buffer** and extract a line only when its `\r\n` delimiter arrives.

An **output buffer** is also necessary because `send()` may transmit only part of the requested data. 
Any unsent bytes must remain queued until the socket can send them.

```text
                    SERVER
                       |
           +-----------+-----------+
           |                       |
       Client A                Client B
           |                       |
   Input: "NICK Ric"      Input: "JOIN #42\r\n"
   Output: ""             Output: "Pending bytes"
```

Client A's incomplete message must not delay Client B or become mixed with Client B's data.

---

## 6. How We Will Represent This in C++98

We can begin with two standard-library containers:

```cpp
std::vector<struct pollfd> _pollFds;
std::map<int, Client>      _clients;
```

| Container | Purpose | Why it fits |
|---|---|---|
| `std::vector<pollfd>` | Stores descriptors and events monitored by `poll()`. | A vector stores elements contiguously, matching the array expected by `poll()`. |
| `std::map<int, Client>` | Associates an accepted socket's fd with its `Client` state. | We can look up the correct client when `poll()` reports an event for its fd. |
| `std::string` | Stores each client's input and output bytes. | We can append bytes and retain incomplete data. |

A simple initial design is:

```text
                        Server
                           |
              +------------+------------+
              |                         |
       vector<pollfd>             map<int, Client>
              |                         |
      Monitored sockets          Connected clients
                                        |
                                   Each Client
                                        |
                               +--------+--------+
                               |                 |
                          Input buffer     Output buffer
```

`Server` coordinates sockets and the event loop. Each `Client` stores the data associated with one connection. We should also agree on **one clear owner for closing each fd**, so that sockets aren't accidentally closed twice.

We will add nickname and channel containers in later milestones, when we actually need them.

---

## 7. What We Must Demonstrate Before Milestone 02

At the end of this milestone, we should be able to start our server:

```bash
./ircserv 6667 password
```

Then open multiple terminals and connect basic TCP test clients:

```bash
nc 127.0.0.1 6667
```

We should demonstrate that:

- The server accepts multiple simultaneous connections.
- Each connection has independent input and output buffers.
- A client sending only half an IRC line does not block other clients.
- Several complete lines received together are extracted individually.
- Pending output is retained when a send is incomplete.
- Disconnecting one client does not terminate the server or affect the others.