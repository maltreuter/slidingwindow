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

Situational errors include lost packets, corrupted packets, and lost acks. List which packets to lose or corrupt, or which acks to lose based on number of packets sent or number of acks received.

Client default values:
- file path: "./testrandom" (30kb)
- host: "127.0.0.1"
- port: "9001"
- packet size: "128"
- timeout interval (ms): "5000"
- window size: "8"
- situational errors: "0"

Server default values:
- output file path: "/tmp/menterzj3144out"
  - output file will increment based on how many files are sent (e.g. out0, out1, etc.)


## TODO
- [x] Menu
- [x] Send file/acks
- [x] Timeouts
- [x] Situational Errors
- [x] Error detection (CRC/Internet Checksum)
- [x] Stop and Wait
- [x] Go-Back-N
- [x] Selective Repeat
