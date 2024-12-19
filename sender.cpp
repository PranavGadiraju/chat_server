#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

int main(int argc, char **argv)
{
  if (argc != 4)
  {
    std::cerr << "Usage: ./sender [server_address] [port] [username]\n";
    return 1;
  }

  std::string server_hostname;
  int server_port;
  std::string username;

  server_hostname = argv[1];
  server_port = std::stoi(argv[2]);
  username = argv[3];

  Connection conn;
  conn.connect(server_hostname, server_port); // connect to server
  if (conn.is_open() == false)
  { // make sure connection is open
    std::cerr << "Error with connection";
    return 1;
  }

  Message slogin(TAG_SLOGIN, username); // send slogin message
  if (!conn.send(slogin))
  { // check if slgoin message was sent
    std::cerr << "Error sending slogin message\n";
    return 1;
  }

  Message slogin_response = Message(); // create new message object
  if (!conn.receive(slogin_response))
  { // check if response was received
    std::cerr << "Error receiving login response\n";
    return 1;
  }
  if (slogin_response.tag == TAG_ERR)
  { // if tag is error, output payload
    std::cerr << slogin_response.data << std::endl;
    return 1;
  }

  // TODO: loop reading commands from user, sending messages to
  //       server as appropriate

  while (1)
  { // loop reading commands from user

    std::string in;               // create string
    getline(std::cin, in);        // place input into string
    Message msg_in = Message();   // create message for input
    Message response = Message(); // create message for response

    if (in.substr(0, 6) == "/join ")
    { // look at just (0, 6) to check since join input string will also have payload after
      msg_in.tag = TAG_JOIN;
      msg_in.data = in.substr(6); // rest of string should go into data
    }
    else if (in == "/leave")
    { // leave room
      msg_in.tag = TAG_LEAVE;
    }
    else if (in == "/quit")
    { // quit
      msg_in.tag = TAG_QUIT;
      conn.send(msg_in);

      Message quit_msg = Message(); // create new quit message
      if (!conn.receive(quit_msg))
      { // check if message is received
        std::cerr << "Error receiving quit message\n";
        return 1;
      }
      if (quit_msg.tag == TAG_ERR)
      { // if there is an error tag, output payload
        std::cerr << quit_msg.data << std::endl;
      }
      break; // leave loop
    }
    else
    { // if it is none of the other messages we sent to all
      msg_in.tag = TAG_SENDALL;
      msg_in.data = in; // we put the message in payload
    }

    if (!conn.send(msg_in))
    { // make sure we send the message
      std::cerr << "Error sending message\n";
      return 1;
    }

    if (!conn.receive(response))
    { // make sure we receive the message
      std::cerr << "Error receiving response\n";
      return 1;
    }

    if (response.tag == TAG_ERR)
    { // if the tag is error we output the payload
      std::cerr << response.data << std::endl;
    }
  }
  conn.close(); // close connection
  return 0;
}
