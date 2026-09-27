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

### 3.1 What we need to understand

- **IP address:** identifies a machine or network interface.
- **Port:** identifies the service to contact on that machine.
- **Socket:** an operating-system interface used for network communication.
- **File descriptor:** the integer handle used to refer to an open socket.
- **Listening socket:** accepts new incoming connections.
- **Client socket:** represents one accepted client connection.
- **TCP byte stream:** incoming reads may contain partial messages or several messages.
- **Non-blocking I/O:** an operation should not freeze the server while waiting for one client.
- **`poll()`:** waits for readiness events on the listening socket and connected client sockets.

**Step 1 — Start and validate arguments**

The program accepts the port and password arguments. 
It should reject missing or invalid startup arguments with a useful error instead of crashing.

**Step 2 — Create the listening socket**

Create and configure a TCP socket, bind it to the chosen port, and start listening. 
Handle failure at every stage and close resources when startup fails.

**Step 3 — Add the event loop**

Use one `poll()`-based event loop to handle the listening socket and connected clients. 
Keep socket operations non-blocking and follow the readiness and I/O restrictions.

**Step 4 — Accept multiple clients**

When the listening socket is ready, accept incoming connections, configure accepted sockets as needed, and add each new client to the server's tracked connections.

**Step 5 — Read into per-client input buffers**

When a client socket is ready for reading, receive available data and append it to **that client's own input buffer**. 
Do not assume one read equals one IRC command.

**Step 6 — Extract complete IRC lines**

Identify complete messages using IRC line endings (`\r\n`). 
Preserve incomplete trailing data until more bytes arrive. 

**Step 7 — Prepare output buffering**

Store outgoing bytes per client and write them only when the socket is ready. 
Account for partial writes: an incomplete send must leave the remaining bytes queued.

**Step 8 — Handle disconnections and errors**

When a client disconnects or an unrecoverable socket error occurs, close the descriptor, remove it from the event loop, and clean up its associated state safely.

### 3.3 Example: why buffering matters

A test client sends one IRC line in two pieces:

```text
First write:   "NICK Ric"
Second write:  "ardo\r\n"
```

The server may receive these pieces separately or together. 
It must produce **one complete line**:

```text
NICK Ricardo
```

It must not process `NICK Ric` prematurely.

---

## 4. Second Milestone — IRC Registration

**Goal:** Turn an accepted TCP connection into a properly registered IRC client.

A TCP connection alone doesn't establish a valid IRC identity. 
The server must track which clients are connected, what registration information they have supplied, and whether they are allowed to use registered-only IRC functionality.

### 4.1 Registration state

A client can be:

```text
TCP connected
     |
     v
Not registered
     |
     | Required PASS / NICK / USER information
     v
Validation
     |
     +---- Invalid ---> Appropriate reply; remain unregistered
     |
     v
Registered
```

Do not assume registration information always arrives in separate reads or in one fixed command order. 
Follow the protocol behavior.

**Step 1 — Extend `Client` state**

Store the registration information and status associated with each client. 
Keep partial registration separate from successful registration.

**Step 2 — Implement `PASS`**

Validate the server password and handle missing, incorrect, repeated, or otherwise invalid use according to the relevant rules.

**Step 3 — Implement `NICK`**

Validate requested nicknames and ensure the same active nickname isn't assigned to two clients. 
Support the required behavior when an already registered client changes their nickname.

**Step 4 — Implement `USER`**

Record the required user information and reject invalid or repeated registration attempts as required.

**Step 5 — Complete registration**

Only mark the client registered when all required conditions are satisfied. 
Send the expected registration replies in the appropriate format.

**Step 6 — Add consistent errors**

Build a small, reusable mechanism for formatting server replies. 
Avoid hand-assembling different versions of the same numeric reply throughout the project.

### 4.3 Example: two clients request the same nickname

```text
Client A ---> NICK Ricardo ---> Accepted

Client B ---> NICK Ricardo ---> Nickname already in use
```

The server must check its shared nickname information. 
A client cannot decide on its own that a nickname is available.

**Integration checkpoint:** we should be able to connect using the same reference IRC client and explain why each registration reply is sent.

---

## 5. Third Milestone — Messages and Channels

**Goal:** Registered clients can exchange private messages, join channels, and communicate with channel members.

We should introduce channels only after basic client identity and command processing are reliable.

### 5.1 Start with private messages

Implement `PRIVMSG` to an individual nickname before adding channel broadcasts.
The server needs to:

1. Parse the target and message text.
2. Validate the request.
3. Find the destination nickname.
4. Construct the appropriate outgoing IRC message.
5. Queue the message for the destination client.

This exercises the nickname lookup and outgoing-message path without requiring channel state.

### 5.2 Introduce the `Channel` component

A channel should manage its own relevant state:

```text
Channel #42
|
|-- Name
|-- Members
|-- Operators
|-- Topic
|-- Invitations
`-- Modes and mode parameters
```

Initially, focus on **channel name and membership**. 
Add administrative state as we approach the final milestone.

Agree on how to represent membership. For example, a channel can keep references or identifiers for clients owned by the server; it shouldn't accidentally become a second owner of the same client objects.

### 5.3 Implement `JOIN`

When a registered client requests `JOIN #42`, the server must locate or create the channel as appropriate, validate entry conditions, update membership, and send the required protocol messages.

Handle repeated joins and channel membership consistently. 
Also decide how empty channels are removed and how membership is cleaned up when clients disconnect.

### 5.4 Implement channel messaging

Once membership works, extend `PRIVMSG` to channel targets.

```text
                    #42
                     |
        +------------+------------+
        |            |            |
     Gonçalo       Pedro        Ricardo
        |
        | PRIVMSG #42 :Hello!
        v
     IRC server
        |
        +-----------> Pedro
        |
        +-----------> Ricardo
```

The server must identify the target channel, validate the request, and forward the message to the appropriate members, normally excluding the sender.

### 5.5 Build the full path, not isolated functions

By the end of this milestone, the same data structures should support:

- Nickname lookup for private messages.
- Channel lookup for channel messages.
- Membership checks.
- Broadcasting to channel members.
- Removal of disconnected clients from every relevant channel.

Don't duplicate the same membership information in unrelated structures unless we have a clear strategy for keeping it synchronized.

**Integration checkpoint:** open three IRC clients, register them, join the same channel, and verify that messages arrive correctly. 
Also test a private message between two of them.

---

## 6. Final Milestone — Modes, Administration, and Testing

**Goal:** Complete the subject's required channel administration, then verify that the server is robust, compatible, and compliant.

This milestone has two parts: **features** and **validation**. 
We shouldn't leave testing until every feature is finished; each new command should come with its own tests.

### 6.1 Channel operators

Introduce or finalize operator membership within each channel.
Operator status is **channel-specific**:

```text
Ricardo
|
|-- #42       -> operator
`-- #general  -> regular member
```

The server must check privileges against the *target channel*, not against a global "operator" flag.

### 6.2 Administrative commands

Implement and test the subject's required administrative commands:

| Command | Purpose | Important validation |
|---|---|---|
| `KICK` | Remove a user from a channel | Sender's channel permissions and target membership |
| `INVITE` | Invite a user to a channel | Channel state, membership, and required privileges |
| `TOPIC` | Read or change a channel's topic | Target channel and topic restrictions |
| `MODE` | Change channel settings or privileges | Correct mode syntax, parameters, and permissions |

A command may have both successful and error outcomes. Document those outcomes in `06-commands.md` as we implement each handler.

### 6.3 Required channel modes

| Mode | Meaning | State to maintain |
|---|---|---|
| `i` | Invite-only | Whether invitation is required |
| `t` | Topic restriction | Whether changing the topic requires operator privileges |
| `k` | Channel key | Current key, if enabled |
| `o` | Operator privilege | Which channel members are operators |
| `l` | User limit | Maximum number of members, if enabled |

Test both enabling and disabling modes where applicable, including commands that need additional parameters.

**Example:** if `#42` has a user limit of two and already contains two members, a third user's `JOIN` request should receive the appropriate error.

### 6.4 Testing strategy

Organize testing into four groups.

**A. Normal behavior**

- Several clients connect and register.
- Users exchange private messages.
- Users join channels and exchange channel messages.
- Operators perform permitted administrative actions.
- Modes affect channel behavior as expected.

**B. Invalid commands and permissions**

- Unknown commands and missing parameters.
- Duplicate or invalid nicknames.
- Unknown message targets.
- Attempts to perform channel actions without membership or permission.
- Incorrect channel keys and attempts to join restricted or full channels.
- Invalid or incomplete mode parameters.

**C. Network edge cases**

- One IRC message split across several sends.
- Several IRC messages combined in one send.
- Multiple clients sending data at the same time.
- Slow or idle clients.
- Partial outgoing writes.
- Abrupt disconnections and repeated connect/disconnect cycles.

**D. Project and build requirements**

- C++98 compilation and strict warning flags.
- Required Makefile targets.
- No prohibited functions or prohibited architecture.
- Required non-blocking behavior and the subject's event-loop restrictions.
- No leaks or invalid memory accesses in relevant test scenarios.
- Compatibility with the selected reference IRC client.

### 6.5 Test the network assumptions explicitly

A test with `nc` or a small test script can send a single command in pieces:

```text
Send 1: "PRI"
Send 2: "VMSG #42 :Hello"
Send 3: "!\r\n"
```

The server should process **one complete command**, not three partial commands. 
Repeat this type of test after adding commands, because new handlers should never bypass the shared input-buffering logic.

---

## 9. Final review before evaluation

Before presenting the project, both teammates should be able to explain the following without relying on one person being "the networking person" and the other being "the channels person":

- What happens between launching `ircserv` and accepting the first client.
- Why we need non-blocking sockets and one event loop.
- Why one `recv()` call doesn't correspond to one IRC message.
- How a client transitions from connected to registered.
- How the server locates the target of a private message.
- How it finds recipients for a channel message.
- How channel membership and operator privileges are represented.
- How the required channel modes change behavior.
- What happens when a client disconnects unexpectedly.
- How we verified the project against the subject.
