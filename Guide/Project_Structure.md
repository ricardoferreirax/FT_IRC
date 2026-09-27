# Project Structure Roadmap

> **Purpose:** Our practical plan for building `ft_irc`. The idea is to get a small, working server first, then add registration, messaging, channels, and finally administrative commands and thorough testing :D
>
> **Approach:** Each milestone should leave us with a version that compiles, runs, and can be tested. We should avoid implementing everything at once.

---

## 1. Development strategy

The most useful approach is to implement a small feature, connect it to the running server, and test it before moving on.

Our proposed order:

```text
Project structure and shared design
                |
                v
       M1: Working TCP server
                |
                v
     M2: IRC client registration
                |
                v
    M3: Messages and channels
                |
                v
     M4: Modes and full testing
                |
                v
       Final review and cleanup
```

### 1.1 Our working rules

- **Keep the server runnable.** Integrate small changes frequently.
- **Agree on interfaces before splitting work.** Two independently written classes are not useful if they disagree on how clients and channels are represented.
- **Separate network I/O from IRC behavior.** Reading bytes, extracting complete messages, parsing commands, and executing IRC actions are different responsibilities.
- **Test invalid input as well as valid input.** A server must reject incorrect requests without crashing or corrupting its state.

---

## 2. Initial Project Structure

We should begin with a structure:

```text
ft_irc/
|
|-- Makefile
|-- README.md
|
|-- include/
|   |-- Server.hpp
|   |-- Client.hpp
|   |-- Channel.hpp
|   |-- Command.hpp
|   `-- Replies.hpp             (maybe...)
|
|-- src/
|   |-- main.cpp
|   |
|   |-- server/
|   |   `-- Server.cpp
|   |
|   |-- client/
|   |   `-- Client.cpp
|   |
|   |-- channel/
|   |   `-- Channel.cpp
|   |
|   |-- command/
|   |   |-- Command.cpp
|   |   |-- Registration.cpp    (maybe)
|   |   |-- Messaging.cpp       (maybe?)
|   |   `-- ChannelCommands.cpp (maybe)
|   |
|   `-- utils/
|
```

### 2.1 Responsibilities of the main components

| Component | Main responsibility | Should not be responsible for |
|---|---|---|
| `main` | Validate startup arguments, create and start the server | Processing IRC commands |
| `Server` | Listening socket, event loop, connected-client ownership, routing requests | Storing every detail of channel behavior |
| `Client` | One connection's descriptor, registration state, identity, input/output buffers | Owning the server or all channels |
| `Channel` | Membership, operators, topic, invitations, and modes | Reading from network sockets |
| `Command` / handlers | Interpret command parameters and apply the appropriate rules | Running a separate event loop |
| Reply helpers | Build consistent protocol replies and errors | Deciding whether an action is authorized |

### 2.2 Decisions to make

We should agree on these points first:

1. **Who owns `Client` objects?** A simple option is for `Server` to own them and remove them when connections close.
2. **How do we identify clients?** The file descriptor is useful for network events; the nickname is useful for IRC routing. They are not the same identifier.
3. **Who owns `Channel` objects?** A central server-managed channel collection is a simple starting point.
4. **How do commands access state?** A handler needs controlled access to the requesting client and the relevant server/channel information.
5. **Where do outgoing messages go?** Prefer one per-client output queue so network writes can be handled by the event loop.
6. **How will we report errors?** Agree on a consistent way to generate IRC numeric replies and notifications.

---

## 3. First Milestone — Working TCP Server

**Goal:** Start a server, accept multiple TCP connections, receive arbitrary bytes without blocking other clients, and disconnect clients safely.

At this stage, we are building the network foundation, **not yet a complete IRC server**.

### 3.1 What we need to understand first

Before implementing this milestone, we should be able to explain:

- **IP address:** identifies a machine or network interface.
- **Port:** identifies the service to contact on that machine.
- **Socket:** an operating-system interface used for network communication.
- **File descriptor:** the integer handle used to refer to an open socket.
- **Listening socket:** accepts new incoming connections.
- **Client socket:** represents one accepted client connection.
- **TCP byte stream:** incoming reads may contain partial messages or several messages.
- **Non-blocking I/O:** an operation should not freeze the server while waiting for one client.
- **`poll()`:** waits for readiness events on the listening socket and connected client sockets.


### 3.2 Suggested implementation order

**Step 1 — Start and validate arguments**

The program accepts the port and password arguments. It should reject missing or invalid startup arguments with a useful error instead of crashing.

**Step 2 — Create the listening socket**

Create and configure a TCP socket, bind it to the chosen port, and start listening. Handle failure at every stage and close resources when startup fails.

**Step 3 — Add the event loop**

Use one `poll()`-based event loop (or the subject's permitted equivalent) to handle the listening socket and connected clients. Keep socket operations non-blocking and follow the subject's exact readiness and I/O restrictions.

**Step 4 — Accept multiple clients**

When the listening socket is ready, accept incoming connections, configure accepted sockets as needed, and add each new client to the server's tracked connections.

**Step 5 — Read into per-client input buffers**

When a client socket is ready for reading, receive available data and append it to **that client's own input buffer**. Do not assume one read equals one IRC command.

**Step 6 — Extract complete IRC lines**

Identify complete messages using IRC line endings (`\r\n`). Preserve incomplete trailing data until more bytes arrive. At this milestone, logging extracted lines is enough; full command execution comes later.

**Step 7 — Prepare output buffering**

Store outgoing bytes per client and write them only when the socket is ready. Account for partial writes: an incomplete send must leave the remaining bytes queued.

**Step 8 — Handle disconnections and errors**

When a client disconnects or an unrecoverable socket error occurs, close the descriptor, remove it from the event loop, and clean up its associated state safely.

### 3.3 Example: why buffering matters

A test client sends one IRC line in two pieces:

```text
First write:   "NICK Ric"
Second write:  "ardo\r\n"
```

The server may receive these pieces separately or together. It must produce **one complete line**:

```text
NICK Ricardo
```

It must not process `NICK Ric` prematurely.

### 3.4 Definition of done

- [ ] The server starts with valid arguments and handles invalid ones.
- [ ] It accepts several simultaneous client connections.
- [ ] One idle client does not block another.
- [ ] Each client has an independent input buffer.
- [ ] Fragmented lines and multiple lines in one read are handled correctly.
- [ ] Outgoing data can be queued and partially sent without being lost.
- [ ] Client disconnections do not crash the server.
- [ ] All relevant socket operations follow the subject's non-blocking and `poll()` rules.
- [ ] Both teammates can explain the complete connection and event-loop flow.

**Don't move on until this works reliably.** Registration and channels depend on this foundation.

---
