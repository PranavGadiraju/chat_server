#include <sstream>
#include <cctype>
#include <cassert>
#include "csapp.h"
#include "message.h"
#include "connection.h"

Connection::Connection()
    : m_fd(-1), m_last_result(SUCCESS)
{
}

Connection::Connection(int fd)
    : m_fd(fd), m_last_result(SUCCESS)
{
  rio_readinitb(&m_fdbuf, fd); // initialize the rio_t object
}

void Connection::connect(const std::string &hostname, int port)
{

  m_fd = open_clientfd(hostname.c_str(), std::to_string(port).c_str()); // call open_clientfd to connect to the server

  if (m_fd < 0)
  { // Connection failed
    m_last_result = EOF_OR_ERROR;
  }
  else
  {
    rio_readinitb(&m_fdbuf, m_fd); // call rio_readinitb to initialize the rio_t object
  }
}

Connection::~Connection()
{
  // close the socket if it is open
  if (is_open())
  {
    close();
  }
}

bool Connection::is_open() const
{
  // return true if the connection is open
  return m_fd >= 0;
}

void Connection::close()
{
  // close the connection if it is open
  if (is_open())
  {
    Close(m_fd);
    m_fd = -1; // change file descriptor accordingly
  }
}

bool Connection::send(const Message &msg)
{

  // make sure that m_last_result is set appropriately

  std::string str_msg = msg.tag + ":" + msg.data + "\n";                    // create message
  ssize_t bytes_read = rio_writen(m_fd, str_msg.c_str(), str_msg.length()); // send message

  if (bytes_read == -1)
  { // if we fail to send, send error message and return false
    m_last_result = EOF_OR_ERROR;
    return false;
  }

  m_last_result = SUCCESS; // return true and success message
  return true;
}

bool Connection::receive(Message &msg)
{

  char buffer[Message::MAX_LEN + 1];                                          // need +1 for the null terimator / new line character
  ssize_t bytes_received = Rio_readlineb(&m_fdbuf, buffer, Message::MAX_LEN); // receive a message and storing into buffer

  if (bytes_received <= 0)
  { // if we fail to recieve, set error message and return false
    m_last_result = EOF_OR_ERROR;
    return false;
  }

  std::stringstream ss(buffer);   // other wise split the string using the :
  std::getline(ss, msg.tag, ':'); // use getline to put approriate string into tag and data
  std::getline(ss, msg.data);

  m_last_result = SUCCESS; // return true and set success message
  return true;
}
