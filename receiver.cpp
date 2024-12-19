
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

int main(int argc, char **argv)
{
  if (argc != 5)
  {
    std::cerr << "Usage: ./receiver [server_address] [port] [username] [room]\n";
    return 1;
  }

  std::string server_hostname = argv[1];
  int server_port = std::stoi(argv[2]);
  std::string username = argv[3];
  std::string room_name = argv[4];

  Connection conn;

  // connect to server
  conn.connect(server_hostname, server_port);
  if (conn.is_open() == false)
  { // make sure connection is open
    std::cerr << "Error: failed to connect" << std::endl;
    return 1;
  }

  // TODO: send rlogin and join messages (expect a response from
  //       the server for each one)

  Message rlogin(TAG_RLOGIN, username); // send rlogin message
  if (!conn.send(rlogin))
  { // make sure rlogin message is sent
    std::cerr << "Error sending rlogin message\n";
    return 1;
  }

  Message response = Message(); // Create new message object
  if (!conn.receive(response))
  { // make sure that the connection receives message
    std::cerr << "Error receiving login response\n";
    return 1;
  }

  if (response.tag == TAG_ERR)
  { // if tag is error, output payload
    std::cerr << response.data << std::endl;
    return 1;
  }

  Message join_msg(TAG_JOIN, room_name); // send join message
  if (!conn.send(join_msg))
  { // make sure join message is sent
    std::cerr << "Error sending join message\n";
    return 1;
  }

  Message join_response = Message(); // Create new message object
  if (!conn.receive(join_response))
  { // make sure that the connection receives message
    std::cerr << "Error receiving join response\n";
    return 1;
  }

  if (join_response.tag == TAG_ERR)
  { // if tag is error, output payload
    std::cerr << join_response.data << std::endl;
    return 1;
  }

  // TODO: loop waiting for messages from server
  //       (which should be tagged with TAG_DELIVERY)

  while (1)
  {                                   // loop waiting for messages from server
    Message received_msg = Message(); // Create new message object
    if (!conn.receive(received_msg))
    { // make sure that the connection receives message
      std::cerr << "Error receiving message\n";
      return 1;
    }

    if (received_msg.tag == TAG_DELIVERY)
    {

      // Split the message into sender, message, and room based on :
      std::string sender;
      std::string message;
      std::string room;
      std::stringstream ss(received_msg.data);
      std::getline(ss, room, ':');
      std::getline(ss, sender, ':');
      std::getline(ss, message, '\n');

      std::cout << sender << ": " << message << std::endl; // print sender and message
    }
  }
  conn.close(); // close connection

  return 0;
}
