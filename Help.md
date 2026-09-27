# FT_IRC

## 1. What is IRC?

**IRC (Internet Relay Chat)** is a text-based communication protocol designed for **real-time communication between multiple users through a server**.

IRC follows a **client-server architecture**, which means that users run an IRC client, and those clients connect to an IRC server.
The server acts as the central point of communication.

```text
                    IRC Server
                        |
          -----------------------------
          |             |             |
          v             v             v
      Client A      Client B      Client C
```

A user does not interact directly with the server's internal structures. 
Instead, the user interacts with an **IRC client**, and that client communicates with the IRC server using the IRC protocol.
The idea is:

```text
User
 |
 v
IRC Client
 │
 │ IRC protocol over TCP
 |
 v
IRC Server
```
---

### 1.1 What is a Communication Protocol?

A **protocol** is a set of rules that defines how two or more systems communicate. Both sides must agree on:

* How messages are formatted.
* How messages are transmitted.
* What commands exist.
* What each command means.
* What responses should be returned.

IRC defines these communication rules. 
For example, when a user wants to join a channel, the IRC client sends a specific IRC command to the server.

Conceptually:

```text
User wants to join #42
          │
          v
     IRC Client
          │
          │ IRC command
          v
     IRC Server
          │
          v
Server interprets the command
          │
          v
User is added to #42
```

Therefore, IRC is not simply a chat application.
IRC is the **protocol that defines how IRC clients and IRC servers communicate**.

---

### 1.1 Text-based protocol

IRC is a **text-based protocol**. This means clients and servers exchange textual IRC messages rather than complex binary structures.

```text
Client
   │
   │ textual IRC message
   v
Server
   │
   │ textual IRC response
   v
Client
```

An IRC message generally represents either a **Command** or **Response**.
For example, a client may request an action:

```text
Client
   │
   │ "I want to join channel #42"
   v
Server
```

The actual IRC protocol represents that intention using a specific IRC command.

The server parses the message, identifies the command, checks whether the action is valid and updates its internal state.

```text
Raw IRC Message
       │
       v
     Parse
       │
       v
Identify Command
       │
       v
Validate Parameters
       │
       v
Check Permissions / State
       │
       v
Execute Action
       │
       v
Send Response
```

---

### 1.2 IRC Uses a Client-Server Architecture

IRC is based on the idea that clients connect to a central server.

```text
                     ----------------
                     │  IRC Server  │
                     ----------------
                            │
             -------------------------------
             │              │              │
             v              v              v
        -----------    -----------    -----------
        │ Client A │  │ Client B  │  | Client C  │
        -----------    -----------    -----------
```

The clients are responsible for interacting with the users.
The server is responsible for coordinating communication between those clients.
This means that the IRC server acts as an intermediary.

For example, if Client A wants to send a private message to Client B:

```text
Client A
   │
   │ message for Client B
   v
IRC Server
   │
   │ finds Client B
   v
Client B
```

The message does not normally travel directly from Client A to Client B. The communication is:

```text
Client A -----> Server -----> Client B
```

---

### 1.3 What is an IRC Client?

An **IRC client** is a program used by a user to connect to and communicate with an IRC server.

The client acts as an **interface between the user and the server**. The user performs actions through the client, and the client 
translates those actions into IRC commands that the server can understand.

```text
                   User
                    │
                    │ performs an action
					|
                    v
              --------------
              │ IRC Client │
              --------------
                    │
                    │ IRC commands
					|
                    v
              --------------
              │ IRC Server │
              --------------
```

The IRC client does not manage the server itself. It simply sends requests to the server and displays the responses it receives.
The user may perform actions such as:

* Connecting to an IRC server.
* Choosing or changing a nickname.
* Joining or leaving a channel.
* Sending a private message or a message to a channel.
* Changing a channel topic.
* Inviting another user.
* Changing channel modes if they have permission.

For example, imagine that a user wants to join the channel `#42`.
From the user's perspective:

```text
User
 │
 │ "I want to join #42"
 v
IRC Client
```

The IRC client translates this action into the corresponding IRC command:

```text
User action
     │
     v
 Join #42
     │
     v
IRC Client
     │
     │ JOIN #42 (command)
     v
IRC Server
```

The server receives the command, checks whether the user is allowed to join the channel, and then updates its internal state.
Another example is sending a message to a channel:

```text
User writes: "Hello everyone!"
        │
        v
    IRC Client
        │
        │ PRIVMSG #42 :Hello everyone! (command)
        v
    IRC Server
        │
        │ finds the members of #42
        v
  Other IRC Clients
        │
        v
Other users see: "Hello everyone!"
```

In `ft_irc`, **we do not create the IRC client**. We create the server that must be able to communicate correctly with an existing IRC client.
So the relationship is:

```text
Existing IRC Client
        │
        │ IRC protocol over TCP
		|
        v
---------------------
│ Our ft_irc Server │
---------------------
```

---

### 1.4 What is an IRC Server?

An **IRC server** is a program that accepts connections from IRC clients and manages the communication between them.

The server acts as the **central point of the IRC system**. Clients send IRC commands to the server, and the server is responsible for interpreting those 
commands, checking whether they are valid, executing the requested actions, and sending the appropriate responses.

Unlike the client, which mainly represents the actions of a user, the server must maintain the **current state of the IRC system**.

This includes information such as:

* Which clients are connected and registered.
* The nickname and username of each client.
* Which channels exist.
* Which users belong to each channel.
* Which users are channel operators.
* The topic of each channel.
* The modes enabled on each channel.

For example, imagine that a client wants to join the channel `#42`.

```
IRC Client
     │
     │ JOIN #42 (command)
     v
IRC Server
     │
     v
Receives the command
     │
     v
Checks the channel
     │
     v
Checks if the client can join
     │
     ├── allowed ──> Add client to #42
     │
     └── denied  ──> Send an error
```

The server does not simply execute everything requested by a client. It must first verify that the request is valid.
For example, if a regular user tries to kick another user from a channel:

```
Regular User
     │
     │ KICK another user
     v
IRC Server
     │
     v
Is this user a channel operator?
     │
   ┌─┴─┐
   │   │
  Yes  No
   │   │
   v   v
 KICK  Error
```

Another important responsibility of the server is **forwarding messages**.

Imagine that Maria sends a message to `#42`:

```
              Maria
                │
                │ message to #42
                v
           --------------
           │ IRC Server │
           --------------
                │
                │ finds #42 members
                v
               #42
              /   \
             v     v
           Pedro   Ricardo
```

Maria does not send the message directly to Pedro and Ricardo.

Instead:

1. Maria's IRC client sends the message to the server.
2. The server receives and interprets the message.
3. The server identifies `#42` as the target.
4. The server finds the clients that belong to `#42`.
5. The server forwards the message to those clients.

The server therefore has several main responsibilities:

```
IRC Server
│
├── Connections
│   ├── Accept clients
│   └── Detect disconnections
│
├── Clients
│   ├── Authentication
│   ├── Registration
│   ├── Nicknames
│   └── Usernames
│
├── Commands
│   ├── Receive
│   ├── Parse
│   ├── Validate
│   └── Execute
│
├── Channels
│   ├── Members
│   ├── Operators
│   ├── Topics
│   └── Modes
│
└── Communication
    ├── Private messages
    └── Channel messages
```

In `ft_irc`, **this is the component that we have to create**.
Our program will receive a port and password when it starts:

```
./ircserv <port> <password>
            │        │
            │        └── Password required by clients
            │
            └── Port where the server listens
```

The server then waits for IRC clients to connect:

```
Start ft_irc
     │
     v
IRC Server
     │
     v
Listen for connections
     │
     ├──── Client A connects
     │
     ├──── Client B connects
     │
     └──── Client C connects
```

Once connected, clients can send IRC commands and the server processes them according to the IRC protocol.
The relationship between the two components can be summarized as:

```
IRC Client
│
├── Interacts with the user
├── Sends IRC commands
├── Receives IRC responses
└── Displays messages/responses to the user
└── Sends requests
        │
        │ IRC Protocol
		|
        v
IRC Server
│
├── Maintains the IRC state
├── Validates requests
├── Receives commands
├── Executes requested actions/commands
├── Maintains users and channels
└── Sends responses/messages back to clients
```

> **The client requests an action; the server validates, executes, and manages the result.**

> **The client asks; the server processes and decides.**

The **client** represents what the **user wants to do**, while the **server** determines whether that **action is valid and updates the IRC state** accordingly.

In `ft_irc`, our main goal is to build this server so that it can manage **multiple IRC clients at the same time** without one client blocking the others.

---

