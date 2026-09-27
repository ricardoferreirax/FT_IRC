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