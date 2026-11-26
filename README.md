*This project has been created as part of the 42 curriculum by pfreire-, rmedeiro.*

# FT_IRC

An Internet Relay Chat (IRC) server with AI-powered agents.

## Description

FT_IRC is an implementation of an IRC server based on the IRC protocol specifications. It supports multiple simultaneous client connections, channels, and AI agents powered by [Ollama](https://ollama.com/).

Users can connect with a standard IRC client or directly through a terminal. In addition to normal IRC communication, the server can host AI agents that participate in conversations and can perform supported IRC actions on behalf of the user who requests them.

The AI agents use individual text-based personality prompts and a large language model (LLM) to generate their responses. When an agent issues an IRC command, that command is executed through the requesting user's connection and is subject to that user's server and channel permissions.

## Instructions

### Requirements

- A C++ compiler and the usual build tools used by the project.
- `make`.
- An IRC client, or `netcat` for a simple terminal connection.
- [Ollama](https://ollama.com/) running and configured if you want to use the AI agents.

### Compile

From the root of the repository, run:

```bash
make
```

### Run the server

Start the server with a port and a connection password:

```bash
./ircserv <port> <password>
```

`<port>` is the TCP port the server will listen on, and `<password>` is the password clients must provide when connecting.

### Connect to the server

Connect using an IRC client configured for the server's address, port, and password. For a quick terminal-based connection, you can use:

```bash
nc -C localhost 8080
```

Replace `8080` with the port used to start the server. The `-C` option makes `netcat` send CRLF line endings, as expected by IRC message framing.

### Register your client

After connecting, send the following IRC commands in order. Replace the placeholders with your own values:

```irc
PASS <password>
NICK <nickname>
USER <username> 0 0 :<real name>
```

- `PASS` sends the server password.
- `NICK` sets your nickname.
- `USER` provides your username and real name. The final parameter begins with `:` so it can contain spaces.

Once registration is complete, you can join channels and communicate with other connected users.

## Resources

### IRC protocol references

- [RFC 1459 — Internet Relay Chat Protocol](https://www.rfc-editor.org/rfc/rfc1459)
- [RFC 2812 — Internet Relay Chat: Client Protocol](https://www.rfc-editor.org/rfc/rfc2812)
- [RFC 2813 — Internet Relay Chat: Server Protocol](https://www.rfc-editor.org/rfc/rfc2813)

### Networking and socket programming

- [Network Fundamentals — Portland State University](https://web.cecs.pdx.edu/~dmcgrath/courses/networking.html)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/split/system-calls-or-bust.html)
- [Socket Programming in C — GeeksforGeeks](https://www.geeksforgeeks.org/c/socket-programming-cc/)

### System calls and API documentation

- [`socket(2)`](https://man7.org/linux/man-pages/man2/socket.2.html)
- [`setsockopt(2)`](https://man7.org/linux/man-pages/man2/setsockopt.2.html)
- [`bind(2)`](https://man7.org/linux/man-pages/man2/bind.2.html)
- [`listen(2)`](https://man7.org/linux/man-pages/man2/listen.2.html)
- [`fcntl(2)`](https://man7.org/linux/man-pages/man2/fcntl.2.html)
- [`poll(2)`](https://man7.org/linux/man-pages/man2/poll.2.html)
- [`accept(2)`](https://man7.org/linux/man-pages/man2/accept.2.html)

### AI usage

AI was used in the following parts of the project:

- **Bot personalities:** AI was used to generate the text-based personality scripts that define how the IRC agents behave and communicate.
- **Bot responses at runtime:** The agents use LLMs through Ollama as their conversational engine. The model generates their replies based on the conversation and each agent's personality prompt.
- **Code completion:** AI-assisted code completion was used while implementing parts of the source code. Some generated or suggested comments may remain in the codebase and should be reviewed.
- **Pretty Formatting this README:** AI was used to put all the pretty things in this readme

The AI agents are an integrated feature of the project; they are not separate IRC users with independent privileges. Supported commands requested through an agent are sent on behalf of the requesting user and remain subject to the permissions of that user's IRC connection.
