// Copyright (C) 2014 Internet Systems Consortium, Inc. ("ISC")
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#ifndef UNIX_SOCKET_H
#define UNIX_SOCKET_H

#include <util/buffer.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>

namespace isc {
namespace util {

/// @brief Exception thrown when BaseIPC::open() failed.
class UnixSocketOpenError : public Exception {
public:
    UnixSocketOpenError(const char* file, size_t line, const char* what) :
    isc::Exception(file, line, what) { };
};

/// @brief Exception thrown when UnixSocket::recv() failed.
class UnixSocketRecvError : public Exception {
public:
    UnixSocketRecvError(const char* file, size_t line, const char* what) :
    isc::Exception(file, line, what) { };
};

/// @brief Exception thrown when BaseIPC::send() failed.
class UnixSocketSendError : public Exception {
public:
    UnixSocketSendError(const char* file, size_t line, const char* what) :
    isc::Exception(file, line, what) { };
};

/// @brief An Inter Process Communication(IPC) tool based on UNIX domain socket.
///
/// It is used by 2 processes for data communication. It provides methods for
/// bi-directional binary data transfer.
///
/// There should be 2 instances (a sender and a receiver) using this tool
/// at the same time. The filename for the sockets must match (i.e. 
/// the remote filename of the sender = the local filename of the receiver).
///
/// It should be used as a base class and not directly used for future classes
/// implementing inter process communication.
class UnixSocket {
public:

    /// @brief Packet reception buffer size
    ///
    /// Receive buffer size of UNIX socket
    static const uint32_t RCVBUFSIZE = 4096;

    /// @brief BaseIPC constructor.
    ///
    /// Creates BaseIPC object for UNIX socket communication using the given
    /// filenames. It doesn't create the socket immediately.
    ///
    /// @param local_filename Filename for receiving socket
    /// @param remote_filename Filename for sending socket
    UnixSocket(const std::string& local_filename, const std::string& remote_filename);
    /// @brief BaseIPC destructor.
    ///
    /// It closes the socket explicitly.
    virtual ~UnixSocket();
    
    
    /// @brief Open UNIX socket
    ///
    /// Method will throw if socket creation fails.
    ///
    /// @return A int value of the socket descriptor.
    int open();

    /// @brief Close opened socket.
    void closeIPC();

    /// @brief Send data.
    /// 
    /// @param buf The data to be sent.
    ///
    /// Method will throw if open() has not been called or sendto() failed. 
    /// open() MUST be called before calling this function.
    ///
    /// @return The number of bytes sent.
    int send(const isc::util::OutputBuffer &buf);

    /// @brief Receive data.
    ///
    /// Method will throw if socket recvfrom() failed.
    /// open() MUST be called before calling this function.
    ///
    /// @return The number of bytes received.
    isc::util::InputBuffer recv();

    /// @brief Get socket fd.
    /// 
    /// @return The socket fd of the unix socket.
    int getSocket() { return socketfd_; }

protected:

    /// @brief Set remote filename
    ///
    /// The remote filename is used for sending data. The filename is given
    /// in the constructor.
    void setRemoteFilename();
    
    /// @brief Bind the UNIX socket to the given filename
    ///
    /// The filename is given in the constructor.
    ///
    /// Method will throw if socket binding fails.
    void bindSocket();
    
    /// UNIX socket value.
    int socketfd_;
    
    /// Remote UNIX socket address 
    struct sockaddr_un remote_addr_;
    
    /// Length of remote_addr_
    int remote_addr_len_;

    /// Filename for receiving and sending socket
    std::string local_filename_, remote_filename_;

}; // UnixSocket class

} // namespace util
} // namespace isc

#endif  // UNIX_SOCKET_H
