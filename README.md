# CS 462 - Networks Project
Implementation of 3 sliding window protocols: Stop and Wait, Go-Back-N, and Selective Repeat

## Usage

### Compilation
Compiled with gcc 8.4.0 (g++) and std=c++17
```
make
make clean
```

### Running
```
./server <port>
./client
```

Run ./server <port> in one terminal and ./client in another
You will be prompted for necessary values in ./client or choose default values

Situational errors include lost packets, corrupted packets, or lost acks. List which packets or acks based on sequence number.

Default values:
	- file path: "./testrandom" (30kb)
	- host: "127.0.0.1"
	- port: "9001"
	- packet size: "128"
	- timeout interval (ms): "5000"
	- window size: "8"
	- situational errors: "0"


## TODO
- [x] Menu
- [x] Send file/acks
- [x] Timeouts
- [ ] Situational Errors
- [x] Error detection (CRC/Internet Checksum)
- [x] Stop and Wait
- [x] Go-Back-N
- [x] Selective Repeat
- [ ] send ack method
- [ ] split header method
- [ ] receive frame method
- [ ] handshake
- [ ] print window utils method
