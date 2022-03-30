# CS 462 - Networks Project
Implementation of 3 sliding window protocols: Stop and Wait, Go-Back-N, and Selective Repeat

## Usage

### Compilation
```
make /* compile to use tcp (sock_stream) */
make udp /* compile to use udp (sock_dgram) */
make clean
```

### Running
```
./server <port>
./client <host> <port>

./serverudp <port>
./clientudp <host> <port> <path-to-file>
```

## TODO
- [ ] Menu
- [ ] Send file/acks
- [ ] Timeouts
- [ ] Situational Errors
- [ ] Error detection (CRC/Internet Checksum)
- [ ] Stop and Wait
- [ ] Go-Back-N
- [ ] Selective Repeat
