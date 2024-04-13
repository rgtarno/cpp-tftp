# CPP-TFTP

This is a C++ implementation of the TFTP protocol, specifically, this repo implements the following RFCs:

1) RFC 1350: The TFTP Protocol (Revision 2) 
2) RFC 2347: TFTP Option Extension 
3) RFC 2348: TFTP Blocksize Option 
4) RFC 2349:  TFTP Timeout Interval and Transfer Size Options 

***

## Dependencies

1. spdlog
2. fmtlib
3. googletest (for tests only)

## Build

To build the server
```
  make server
```

To build the client
```
  make client
```

## Run

```
./build/apps/tftp_server
```