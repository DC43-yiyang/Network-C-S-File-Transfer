## Network C/S File Transfer Tool

### Functional Design

1. **LAN Detection**:  
   The server broadcasts its IP and port every 3 seconds on port 4001, while the client listens for 5 seconds and prints available servers to connect.
   
2. **Upload/Download**:  
   - Uses TCP for communication, with file reading/writing handled via `mmap` to reduce system call overhead.
   - Upload resumes from the last offset if interrupted, and the same applies to downloads. Progress bars are displayed.

3. **UDP/TCP Options**:  
   By default, TCP is used unless specified. For UDP, a custom protocol ensures reliable data transfer with 1KB packets and acknowledgment IDs.

4. **File Verification**:  
   MD5 checksums verify file integrity after upload/download.

### Client Commands:
Commands include options for upload (put) or download (get) with either TCP or UDP, along with session history and auto-completion features.


### Code Structure:

```C
│─── bin                         // Binary output
│     ├─ client  
│     └─ server  
│─── include                     // Header files and MD5/common implementations
│     ├─ md5.h
│     ├─ common.h
│     ├─ client_work.h
│     ├─ server_work.h
│     ├─ common.c
│     └─ md5.c
│─── client                      // Client-side code
│     ├─ client.c
│     └─ client_work.c
│─── server                      // Server-side code
│     ├─ server.c
│     └─ server_work.c
│─── pic                         // Pictures and gifs for documentation
└─── makefile                    // Build script
'''

### Limitations:
- Single-client-server interaction with no concurrency support.
- Future improvements could include deduplication using MD5 checksums for identical files with different names and the use of databases for file management.
