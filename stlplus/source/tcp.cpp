/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  ------------------------------------------------------------------------------*/
#include "os_fixes.hpp"
#include "tcp.hpp"
#ifdef _WIN32
// Windoze-specific includes
#include <winsock2.h>
#define ERRNO WSAGetLastError()
#define HERRNO WSAGetLastError()
#define IOCTL ioctlsocket
#define CLOSE closesocket
#define SHUT_RDWR SD_BOTH
#define EINPROGRESS WSAEINPROGRESS
#define EWOULDBLOCK WSAEWOULDBLOCK
#define SOCKLEN_T int
#else
// Generic Unix includes
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#define INVALID_SOCKET -1
#define ERRNO errno
#define HERRNO h_errno
#define SOCKET int
#define SOCKET_ERROR -1
#define IOCTL ::ioctl
#define CLOSE ::close
#define SOCKLEN_T socklen_t
#ifdef SOLARIS
// Sun put some definitions in a different place
#include <sys/filio.h>
#endif
#ifdef MACOS
#undef SOCKLEN_T
#define SOCKLEN_T int
#endif
#endif
#include "debug.hpp"
#include "fileio.hpp"
#include "string_utilities.hpp"

////////////////////////////////////////////////////////////////////////////////
// Utilities

static otext& operator << (otext& str, const sockaddr& address)
{
  if (address.sa_family == AF_INET)
    str << "INET";
  else
    str << address.sa_family;
  str << ":";
  unsigned short* ptr = (unsigned short*)(&address.sa_data);
  // TODO - work out how to print this as a proper internet address
  str << (*(unsigned long*)(ptr+1));
  str << ":";
  str << *ptr;
  return str;
}

static std::string error_string(int error)
{
  std::string result = "error " + to_string(error);
#ifdef _WIN32
  char* message = 0;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                0,
                error,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // "User default language"
                (LPTSTR)&message,
                0,0);
  if (message) {                  // can happen in NT/9x
    result = message;
    LocalFree(message);
  } else {
    result = strerror(error);     // does this give same as FormatMessage ?
  }
#else
  result = strerror(error);
#endif
  // strip any trailing whitespace
  result = trim(result);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
// Initialisation
// Windows requires that Winsock is initialised before use and closed after
// These routines initialise once on first use and close on the destruction of the last object using it

static int sockets_count = 0;

static int sockets_init(void)
{
  DEBUG_TRACE;
  int error = 0;
  if (sockets_count++ == 0)
  {
#ifdef _WIN32
    WSAData winsock_info;
    // request Winsock 2.0 or higher
    error = WSAStartup(MAKEWORD(2,0),&winsock_info);
    IF_DEBUG(ferr << "initialised sockets with result " << error << " = " << error_string(error) << endl);
#endif
  }
  return error;
}

static int sockets_close(void)
{
  DEBUG_TRACE;
  int error = 0;
  if (--sockets_count == 0)
  {
#ifdef _WIN32
    if (WSACleanup() == SOCKET_ERROR)
      error = ERRNO;
    IF_DEBUG(ferr << "Closed sockets with result " << error << " = " << error_string(error) << endl);
#endif
  }
  return error;
}

////////////////////////////////////////////////////////////////////////////////
// Socket - factored out common code to manipulate a TCP socket

class TCP_socket
{
private:
  SOCKET m_socket;
  int m_error;
  std::string m_message;

public:

  TCP_socket(SOCKET socket = INVALID_SOCKET) : m_socket(INVALID_SOCKET), m_error(0)
    {
      DEBUG_TRACE;
      set_error(sockets_init());
      m_socket = socket;
      IF_DEBUG(ferr << "constructed with " << socket << endl);
    }

  ~TCP_socket(void)
    {
      DEBUG_TRACE;
      close();
      set_error(sockets_close());
    }

  TCP_socket(const TCP_socket& socket) : m_socket(INVALID_SOCKET), m_error(0)
    {
      DEBUG_TRACE;
      set_error(sockets_init());
      *this = socket;
    }

  TCP_socket& operator= (const TCP_socket& socket)
    {
      DEBUG_TRACE;
      DEBUG_ASSERT(m_socket == INVALID_SOCKET);
      DEBUG_ASSERT(socket.m_socket == INVALID_SOCKET);
      m_socket = socket.m_socket;
      m_error = socket.m_error;
      m_message = socket.m_message;
      return *this;
    }

  bool set(SOCKET socket = INVALID_SOCKET)
    {
      DEBUG_TRACE;
      close();
      m_socket = socket;
      return true;
    }

  bool initialise(void)
    {
      DEBUG_TRACE;
      close();
      // create an anonymous socket
      m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
      IF_DEBUG(ferr << "created socket " << m_socket << endl);
      if (m_socket == INVALID_SOCKET)
      {
        set_error(ERRNO);
        close();
        return false;
      }
      // set the socket into non-blocking mode
      unsigned long nonblocking = 1;
      if (IOCTL(m_socket, FIONBIO, &nonblocking) == SOCKET_ERROR)
      {
        set_error(ERRNO);
        return false;
      }
      return true;
    }

  SOCKET socket(void) const
    {
      DEBUG_TRACE;
      return m_socket;
    }

  void set_error (int error)
    {
      DEBUG_TRACE;
      if (m_error == 0 && error != 0)
      {
        m_error = error;
        m_message = error_string(error);
        IF_DEBUG(ferr << "error set to " << m_error << " = " << m_message << endl);
      }
    }

  int error(void) const
    {
      DEBUG_TRACE;
      return m_error;
    }

  std::string message(void) const
    {
      DEBUG_TRACE;
      return m_message;
    }

  bool initialised(void) const
    {
      DEBUG_TRACE;
      IF_DEBUG(ferr << "socket = " << m_socket << endl);
      return m_socket != INVALID_SOCKET;
    }

  bool send_ready(unsigned wait)
    {
      DEBUG_TRACE;
      if (!initialised()) return false;
      // determines whether the socket is ready to send
      fd_set write_set; FD_ZERO(&write_set);
      FD_SET(m_socket,&write_set);
      timeval timeout;
      timeout.tv_sec = wait/1000000;
      timeout.tv_usec = wait%1000000;
      int select_result = select(m_socket+1, 0, &write_set, 0, &timeout);
      IF_DEBUG(ferr << "select for socket " << m_socket << " returned " << select_result << endl);
      switch(select_result)
      {
      case SOCKET_ERROR:
        // select failed with an error - trap the error
        set_error(ERRNO);
        return false;
      case 0:
        // timeout exceeded without a connection appearing
        return false;
      default:
        // at least one connection is pending
        return true;
      }
    }

  bool send (std::string& data)
    {
      DEBUG_TRACE;
      if (!initialised()) return false;
      if (!send_ready(0)) return true;
      // send the data - this will never block but may not send all the data
      int bytes = ::send(m_socket, data.c_str(), data.size(), 0);
      if (bytes == SOCKET_ERROR)
      {
        set_error(ERRNO);
        return false;
      }
      // remove the sent bytes from the data buffer so that the buffer represents the data still to be sent
      data.erase(0,bytes);
      return true;
    }

  bool receive_ready(unsigned wait)
    {
      DEBUG_TRACE;
      if (!initialised()) return false;
      // determines whether the socket is ready to receive
      fd_set read_set;
      FD_ZERO(&read_set);
      FD_SET(m_socket,&read_set);
      timeval timeout;
      timeout.tv_sec = wait/1000000;
      timeout.tv_usec = wait%1000000;
      int select_result = select(m_socket+1, &read_set, 0, 0, &timeout);
      IF_DEBUG(ferr << "select for socket " << m_socket << " returned " << select_result << endl);
      switch(select_result)
      {
      case SOCKET_ERROR:
        // select failed with an error - trap the error
        set_error(ERRNO);
        return false;
      case 0:
        // timeout exceeded without a connection appearing
        return false;
      default:
        // at least one connection is pending
        return true;
      }
    }

  bool receive (std::string& data)
    {
      DEBUG_TRACE;
      if (!initialised()) return false;
      if (!receive_ready(0)) return true;
      bool result = true;
      // determine how much data is available to read
      unsigned long bytes = 0;
      if (IOCTL(m_socket, FIONREAD, &bytes) == SOCKET_ERROR)
      {
        IF_DEBUG(ferr << "failed to extract the number of bytes from the socket" << endl);
        set_error(ERRNO);
        result = false;
      }
      else
      {
        IF_DEBUG(ferr << "detected " << bytes << " bytes ready to receive" << endl);
        // get the data up to the amount claimed to be present - this is non-blocking
        char* buffer = new char[bytes+1];
        int read = recv(m_socket, buffer, bytes, 0);
        if (read == SOCKET_ERROR)
        {
          IF_DEBUG(ferr << "failed to receive from the socket" << endl);
          set_error(ERRNO);
          close();
          result = false;
        }
        else if (read == 0)
        {
          IF_DEBUG(ferr << "got an end of data from the socket" << endl);
          close();
        }
        else
        {
          // this is binary data so copy the bytes including nulls
          data.append(buffer,read);
        }
        delete[] buffer;
      }
      return result;
    }

  bool close(void)
    {
      DEBUG_TRACE;
      bool result = true;
      if (initialised())
      {
        IF_DEBUG(ferr << "shutting down socket " << m_socket << endl);
        if (shutdown(m_socket,SHUT_RDWR) == SOCKET_ERROR)
        {
          set_error(ERRNO);
          result = false;
        }
        IF_DEBUG(ferr << "closing socket " << m_socket << endl);
        if (CLOSE(m_socket) == SOCKET_ERROR)
        {
          set_error(ERRNO);
          result = false;
        }
      }
      m_socket = INVALID_SOCKET;
      return result;
    }
};

////////////////////////////////////////////////////////////////////////////////
// Connection

class TCP_connection_data
{
private:
  friend class TCP_server_data;
  TCP_socket m_socket;
  unsigned long m_address;
  unsigned short m_port;

public:

  TCP_connection_data(void)
    {
      DEBUG_TRACE;
      m_address = 0;
      m_port = 0;
    }

  ~TCP_connection_data(void)
    {
      DEBUG_TRACE;
      close();
    }

  TCP_connection_data(const TCP_connection_data& data)
    {
      DEBUG_TRACE;
      m_address = 0;
      m_port = 0;
      *this = data;
    }

  void set_error (int error)
    {
      DEBUG_TRACE;
      m_socket.set_error(error);
    }

  int error(void) const
    {
      DEBUG_TRACE;
      return m_socket.error();
    }

  std::string message(void) const
    {
      DEBUG_TRACE;
      return m_socket.message();
    }

  bool initialise(const SOCKET& socket, unsigned long address, unsigned short port)
    {
      DEBUG_TRACE;
      bool result = true;
      close();
      m_socket.set(socket);
      m_address = address;
      m_port = port;
      return result;
    }

  bool initialised(void) const
    {
      DEBUG_TRACE;
      return m_socket.initialised();
    }

  unsigned long address(void) const
    {
      DEBUG_TRACE;
      return m_address;
    }

  unsigned short port(void) const
    {
      DEBUG_TRACE;
      return m_port;
    }

  bool send_ready(unsigned wait)
    {
      DEBUG_TRACE;
      return m_socket.send_ready(wait);
    }

  bool send (std::string& data)
    {
      DEBUG_TRACE;
      return m_socket.send(data);
    }

  bool receive_ready(unsigned wait)
    {
      DEBUG_TRACE;
      return m_socket.receive_ready(wait);
    }

  bool receive (std::string& data)
    {
      DEBUG_TRACE;
      return m_socket.receive(data);
    }

  bool close(void)
    {
      DEBUG_TRACE;
      m_address = 0;
      m_port = 0;
      return m_socket.close();
    }
};

////////////////////////////////////////
// exported functions

TCP_connection::TCP_connection(void) : m_data(TCP_connection_data())
{
  DEBUG_TRACE;
}

TCP_connection::~TCP_connection(void)
{
  DEBUG_TRACE;
}

TCP_connection::TCP_connection(const TCP_connection& connection)
{
  DEBUG_TRACE;
  *this = connection;
}

TCP_connection& TCP_connection::operator=(const TCP_connection& connection)
{
  DEBUG_TRACE;
  m_data = connection.m_data;
  return *this;
}

int TCP_connection::error(void) const
{
  DEBUG_TRACE;
  return m_data->error();
}

std::string TCP_connection::message(void) const
{
  DEBUG_TRACE;
  return m_data->message();
}

bool TCP_connection::initialised(void) const
{
  DEBUG_TRACE;
  return m_data->initialised();
}

unsigned long TCP_connection::address(void) const
{
  DEBUG_TRACE;
  return m_data->address();
}

unsigned short TCP_connection::port(void) const
{
  DEBUG_TRACE;
  return m_data->port();
}

bool TCP_connection::send_ready(unsigned wait)
{
  DEBUG_TRACE;
  return m_data->send_ready(wait);
}

bool TCP_connection::send (std::string& data)
{
  DEBUG_TRACE;
  return m_data->send(data);
}

bool TCP_connection::receive_ready(unsigned wait)
{
  DEBUG_TRACE;
  return m_data->receive_ready(wait);
}

bool TCP_connection::receive (std::string& data)
{
  DEBUG_TRACE;
  return m_data->receive(data);
}

bool TCP_connection::close(void)
{
  DEBUG_TRACE;
  return m_data->close();
}

////////////////////////////////////////////////////////////////////////////////
// Server

class TCP_server_data
{
private:
  TCP_socket m_socket;

public:

  TCP_server_data(void)
    {
      DEBUG_TRACE;
    }

  ~TCP_server_data(void)
    {
      DEBUG_TRACE;
    }

  TCP_server_data(const TCP_server_data& server)
    {
      DEBUG_TRACE;
      *this = server;
    }

  TCP_server_data& operator= (const TCP_server_data& server)
    {
      DEBUG_TRACE;
      m_socket = server.m_socket;
      return *this;
    }

  void set_error (int error)
    {
      DEBUG_TRACE;
      m_socket.set_error(error);
    }

  int error(void) const
    {
      DEBUG_TRACE;
      return m_socket.error();
    }

  std::string message(void) const
    {
      DEBUG_TRACE;
      return m_socket.message();
    }

  bool initialise(unsigned short port, unsigned short queue)
    {
      DEBUG_TRACE;
      close();
      if (!m_socket.initialise())
        return false;
      // name the socket and bind it to a port - this is a requirement for a server
      unsigned short network_port = htons((unsigned short)port);
      IF_DEBUG(ferr << "port " << port << " maps to network port " << network_port << endl);
      unsigned long network_address = htonl(INADDR_ANY);
      IF_DEBUG(ferr << "address " << INADDR_ANY << " maps to network address " << network_address << endl);
      sockaddr server;
      server.sa_family = AF_INET;
      memcpy(&server.sa_data[0], &network_port, sizeof(network_port));
      memcpy(&server.sa_data[2], &network_address, sizeof(network_address));
      IF_DEBUG(ferr << "binding to server " << server << endl);
      if (bind(m_socket.socket(), &server, sizeof(server)) == SOCKET_ERROR)
      {
        set_error(ERRNO);
        close();
        return false;
      }
      IF_DEBUG(ferr << "setting socket " << m_socket.socket() << " to listen" << endl);
      // now set the port to listen for incoming connections
      if (listen(m_socket.socket(), (int)queue) == SOCKET_ERROR)
      {
        set_error(ERRNO);
        close();
        return false;
      }
      return true;
    }

  bool initialised(void) const
    {
      DEBUG_TRACE;
      return m_socket.initialised();
    }

  bool close(void)
    {
      DEBUG_TRACE;
      return m_socket.close();
    }

  bool connection_ready(unsigned wait)
    {
      DEBUG_TRACE;
      return m_socket.receive_ready(wait);
    }

  TCP_connection connection(void)
    {
      DEBUG_TRACE;
      TCP_connection connection;
      // accept a connection: the return value is the socket and the address is filled in with the connection details in network order
      sockaddr address;
      SOCKLEN_T address_length = sizeof(address);
      SOCKET connection_socket = accept(m_socket.socket(), &address, &address_length);
      IF_DEBUG(ferr << "accepted connection on socket " << connection_socket << " from " << address << endl);
      if (connection_socket == INVALID_SOCKET)
        set_error(ERRNO);
      else
      {
        // extract the contents of the address the hard way
        unsigned short network_port = 0;
        memcpy(&network_port, &address.sa_data[0], sizeof(network_port));
        unsigned long network_address = 0;
        memcpy(&network_address, &address.sa_data[2], sizeof(network_address));
        IF_DEBUG(ferr << "network address is " << network_address << ":" << network_port << endl);
        connection.m_data->initialise(connection_socket, ntohl(network_address), ntohs(network_port));
      }
      return connection;
    }
};

////////////////////////////////////////
// exported functions

TCP_server::TCP_server(void) : m_data(TCP_server_data())
{
  DEBUG_TRACE;
}

TCP_server::TCP_server(unsigned short port, unsigned short queue) : m_data(TCP_server_data())
{
  DEBUG_TRACE;
  initialise(port,queue);
}

TCP_server::~TCP_server(void)
{
  DEBUG_TRACE;
}

TCP_server::TCP_server(const TCP_server& server)
{
  DEBUG_TRACE;
  *this = server;
}

TCP_server& TCP_server::operator=(const TCP_server& server)
{
  DEBUG_TRACE;
  m_data = server.m_data;
  return *this;
}

bool TCP_server::initialise(unsigned short port, unsigned short queue)
{
  DEBUG_TRACE;
  return m_data->initialise(port,queue);
}

bool TCP_server::initialised(void) const
{
  DEBUG_TRACE;
  return m_data->initialised();
}

int TCP_server::error(void) const
{
  DEBUG_TRACE;
  return m_data->error();
}

std::string TCP_server::message(void) const
{
  DEBUG_TRACE;
  return m_data->message();
}

bool TCP_server::close(void)
{
  DEBUG_TRACE;
  return m_data->close();
}

bool TCP_server::connection_ready(unsigned wait)
{
  DEBUG_TRACE;
  return m_data->connection_ready(wait);
}

TCP_connection TCP_server::connection(void)
{
  DEBUG_TRACE;
  return m_data->connection();
}

////////////////////////////////////////////////////////////////////////////////
// Client

class TCP_client_data
{
private:
  TCP_socket m_socket;
  unsigned long m_address;
  unsigned short m_port;

public:

  TCP_client_data(void)
    {
      DEBUG_TRACE;
      m_address = 0;
      m_port = 0;
    }

  ~TCP_client_data(void)
    {
      DEBUG_TRACE;
      close();
    }

  TCP_client_data(const TCP_client_data& client)
    {
      DEBUG_TRACE;
      m_address = 0;
      m_port = 0;
      *this = client;
    }

  TCP_client_data& operator= (const TCP_client_data& client)
    {
      DEBUG_TRACE;
      close();
      m_socket = client.m_socket;
      m_address = client.m_address;
      m_port = client.m_port;
      return *this;
    }

  void set_error (int error)
    {
      DEBUG_TRACE;
      m_socket.set_error(error);
    }

  int error(void) const
    {
      DEBUG_TRACE;
      return m_socket.error();
    }

  std::string message(void) const
    {
      DEBUG_TRACE;
      return m_socket.message();
    }

  bool initialise(const std::string& address, unsigned short port, unsigned int timeout=10000000)
    {
      DEBUG_TRACE;
      close();
      bool result = true;
      if (!m_socket.initialise())
      {
        IF_DEBUG(ferr << "socket initialisation failed" << endl);
        result = false;
      }
      else
      {
        // this DOES lookup IP address names as well (not according to MS help !!)
        hostent* host_info = gethostbyname(address.c_str());
        if (!host_info)
        {
          IF_DEBUG(ferr << "gethostbyname failed" << endl);
          set_error(HERRNO);
          result = false;
        }
        else
        {
          sockaddr connect_data;
          connect_data.sa_family = host_info->h_addrtype;
          unsigned short network_port = htons((unsigned short)port);
          memcpy(&connect_data.sa_data[0], &network_port, sizeof(network_port));
          memcpy(&connect_data.sa_data[2], host_info->h_addr, host_info->h_length);

          // fill in our copy of the server address and port too
          memcpy(&m_port, &connect_data.sa_data[0], sizeof(m_port));
          m_port = ntohs(m_port);
          memcpy(&m_address, &connect_data.sa_data[2], sizeof(m_address));
          m_address = ntohl(m_address);

          // the socket is non-blocking, so connect will almost certainly fail with EINPROGRESS
          if (connect(m_socket.socket(), &connect_data, sizeof(connect_data)) == SOCKET_ERROR)
          {
            int error = ERRNO;
            if (error == EINPROGRESS || error == EWOULDBLOCK)
            {
              IF_DEBUG(ferr << "connect is incomplete - waiting for completion" << endl);
              // the Linux docs say detect completion by selecting the socket for writing
              // TODO - do something more sensible with the timeout - defaults to 10s
              if (!m_socket.send_ready(timeout))
              {
                IF_DEBUG(ferr << "socket connect failed after timeout" << endl);
                // TODO - how do I get the real error?
                set_error(error);
                result = false;
              }
            }
            else
            {
              IF_DEBUG(ferr << "connect failed" << endl);
              set_error(error);
              result = false;
            }
          }
        }
      }
      if (!result) close();
      return result;
    }

  bool initialised(void) const
    {
      DEBUG_TRACE;
      return m_socket.initialised();
    }

  unsigned long address(void) const
    {
      DEBUG_TRACE;
      return m_address;
    }

  unsigned short port(void) const
    {
      DEBUG_TRACE;
      return m_port;
    }

  bool send_ready(unsigned wait)
    {
      DEBUG_TRACE;
      return m_socket.send_ready(wait);
    }

  bool send (std::string& data)
    {
      DEBUG_TRACE;
      return m_socket.send(data);
    }

  bool receive_ready(unsigned wait)
    {
      DEBUG_TRACE;
      return m_socket.receive_ready(wait);
    }

  bool receive (std::string& data)
    {
      DEBUG_TRACE;
      return m_socket.receive(data);
    }

  bool close(void)
    {
      DEBUG_TRACE;
      m_address = 0;
      m_port = 0;      
      return m_socket.close();
    }

};

////////////////////////////////////////
// exported functions

TCP_client::TCP_client(void) : m_data(TCP_client_data())
{
  DEBUG_TRACE;
}

TCP_client::TCP_client(const std::string& address, unsigned short port, unsigned int timeout) : m_data(TCP_client_data())
{
  DEBUG_TRACE;
  initialise(address,port,timeout);
}

TCP_client::~TCP_client(void)
{
  DEBUG_TRACE;
}

TCP_client::TCP_client(const TCP_client& client)
{
  DEBUG_TRACE;
  *this = client;
}

TCP_client& TCP_client::operator=(const TCP_client& client)
{
  DEBUG_TRACE;
  m_data = client.m_data;
  return *this;
}

int TCP_client::error(void) const
{
  DEBUG_TRACE;
  return m_data->error();
}

std::string TCP_client::message(void) const
{
  DEBUG_TRACE;
  return m_data->message();
}

bool TCP_client::initialise(const std::string& address, unsigned short port, unsigned int timeout)
{
  DEBUG_TRACE;
  return m_data->initialise(address, port, timeout);
}

bool TCP_client::initialised(void) const
{
  DEBUG_TRACE;
  return m_data->initialised();
}

unsigned long TCP_client::address(void) const
{
  DEBUG_TRACE;
  return m_data->address();
}

unsigned short TCP_client::port(void) const
{
  DEBUG_TRACE;
  return m_data->port();
}

bool TCP_client::send_ready(unsigned wait)
{
  DEBUG_TRACE;
  return m_data->send_ready(wait);
}

bool TCP_client::send (std::string& data)
{
  DEBUG_TRACE;
  return m_data->send(data);
}

bool TCP_client::receive_ready(unsigned wait)
{
  DEBUG_TRACE;
  return m_data->receive_ready(wait);
}

bool TCP_client::receive (std::string& data)
{
  DEBUG_TRACE;
  return m_data->receive(data);
}

bool TCP_client::close(void)
{
  DEBUG_TRACE;
  return m_data->close();
}

////////////////////////////////////////////////////////////////////////////////
