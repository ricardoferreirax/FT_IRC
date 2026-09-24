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

IRC is a **text-based protocol**.
This means clients and servers exchange textual IRC messages rather than complex binary structures.

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