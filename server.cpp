#include <pthread.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <set>
#include <vector>
#include <cctype>
#include <cassert>
#include "message.h"
#include "connection.h"
#include "user.h"
#include "room.h"
#include "guard.h"
#include "server.h"

////////////////////////////////////////////////////////////////////////
// Server implementation data types
////////////////////////////////////////////////////////////////////////

// TODO: add any additional data types that might be helpful
//       for implementing the Server member functions

struct ConnInfo
{
  Server *server;
  Connection *connection;
  ~ConnInfo()
  {
    delete connection;
  }
};

////////////////////////////////////////////////////////////////////////
// Client thread functions
////////////////////////////////////////////////////////////////////////

namespace
{

  void *worker(void *arg)
  {
    pthread_detach(pthread_self());

    // use a static cast to convert arg from a void* to
    //    whatever pointer type describes the object(s) needed
    //    to communicate with a client (sender or receiver)

    ConnInfo *info = static_cast<ConnInfo *>(arg);

    Message login_msg = Message();
    Message response_msg = Message();

    // Handle login connection errors
    if (!(info->connection->receive(login_msg)))
    {
      if (info->connection->get_last_result() == Connection::EOF_OR_ERROR)
      {
        response_msg = Message(TAG_ERR, "Error: failed to receive  message");
      }
      else if (info->connection->get_last_result() == Connection::INVALID_MSG)
      {
        response_msg = Message(TAG_ERR, "Error: message format was invalid");
      }
      info->connection->send(response_msg);
      return nullptr; // e
    }

    // read login message (should be tagged either with
    //    TAG_SLOGIN or TAG_RLOGIN), send response

    if (login_msg.tag == TAG_SLOGIN) // login as sender
    {
      response_msg = Message(TAG_OK, "Logged in as sender");
      if (!info->connection->send(response_msg))
      {
        return nullptr;
      }
    }
    else if (login_msg.tag == TAG_RLOGIN) // login as receiver
    {
      response_msg = Message(TAG_OK, "Logged in as reciever");
      if (!info->connection->send(response_msg))
      {
        return nullptr;
      }
    }
    else // user must first login
    {
      response_msg = Message(TAG_ERR, login_msg.data);
      if (!info->connection->send(response_msg))
      {
        return nullptr;
      }
    }

    // depending on whether the client logged in as a sender or
    //    receiver, communicate with the client (implementing
    //    separate helper functions for each of these possibilities
    //    is a good idea)

    User *user = new User(login_msg.data);

    if (login_msg.tag == TAG_SLOGIN)
    {
      info->server->chat_with_sender(user, info->connection, info->server);
    }
    else if (login_msg.tag == TAG_RLOGIN)
    {
      info->server->chat_with_reciever(user, info->connection, info->server);
    }

    return nullptr;
  }

}

void Server::chat_with_sender(User *user, Connection *connection, Server *server)
{
  Room *room = nullptr;

  while (1)
  {
    Message sender_msg = Message();
    Message response_msg;

    if (!(connection->receive(sender_msg))) // check for errors when message fails to receive
    {
      if (connection->get_last_result() == Connection::EOF_OR_ERROR) // different errors for failed to receive and invalid format
      {
        response_msg = Message(TAG_ERR, "Error: failed to receive  message");
        connection->send(response_msg);
        return;
      }
      else if (connection->get_last_result() == Connection::INVALID_MSG)
      {
        response_msg = Message(TAG_ERR, "Error: message format was invalid");
        connection->send(response_msg);
        return;
      }
      else // we also check if the message is empty here and don't send anything if it is
      {
        response_msg = Message(TAG_ERR, "Error: message was not recieved");
        connection->send(response_msg);
      }
    }
    else
    {
      {
        if (sender_msg.tag == TAG_ERR) // error tag so we don't send anything
        {
          response_msg = Message(TAG_ERR, "Error tag recieved");
          connection->send(response_msg);
          connection->close();
          return;
        }
        else if (sender_msg.tag == TAG_QUIT) // quit tag so the sender leaves the connection
        {
          response_msg = Message(TAG_OK, "Quit message recieved");
          connection->send(response_msg);
          connection->close();
          break;
        }
        else if (sender_msg.data.length() >= Message::MAX_LEN) // message is larger than max length
        {
          response_msg = Message(TAG_ERR, "Error: Message is longer than max message length");
          connection->send(response_msg);
        }
        else if (sender_msg.tag == TAG_JOIN)
        {
          if (room != nullptr)
          {
            room->remove_member(user);                           // remove the room the user was in before
            room = server->find_or_create_room(sender_msg.data); // find or create a room based on message
            room->add_member(user);                              // add user to that room
            response_msg = Message(TAG_OK, "Success: user has switched rooms");
            if (!connection->send(response_msg))
            { // if a connection ever fails to send then leave
              return;
            }
          }
          else
          {
            room = server->find_or_create_room(sender_msg.data); // create the room user wants to join
            room->add_member(user);                              // add user to that room
            response_msg = Message(TAG_OK, "Success: user has joined room");
            if (!connection->send(response_msg))
            { // if a connection ever fails to send then leave
              return;
            }
          }
        }
        else if (sender_msg.tag == TAG_SENDALL)
        {
          if (room != nullptr)
          {
            room->broadcast_message(user->username, sender_msg.data);
            response_msg = Message(TAG_OK, "Success: message was broadcasted");
            if (!connection->send(response_msg))
            { // if a connection ever fails to send then leave
              return;
            }
          }
          else // need to join a room first
          {
            response_msg = Message(TAG_ERR, "Not in a room, join a room before attempting to broadcast");
            connection->send(response_msg);
          }
        }
        else if (sender_msg.tag == TAG_LEAVE)
        {
          if (room != nullptr)
          {
            room->remove_member(user);
            room = nullptr;
            response_msg = Message(TAG_OK, "User has left room");
            if (!connection->send(response_msg))
            { // if a connection ever fails to send then leave
              return;
            }
          }
          else // need to join a room first
          {
            response_msg = Message(TAG_ERR, "Not in a room, join a room before attempting to leave");
            connection->send(response_msg);
          }
        }
        else // non of the valid tags were found
        {
          response_msg = Message(TAG_ERR, "ERROR: Unknown tag");
          connection->send(response_msg);
        }
      }
    }
  }
  connection->close(); // close the connection with the server
  delete user;         // does not have a destructor so we must remove ourselves
  return;
}

void Server::chat_with_reciever(User *user, Connection *connection, Server *server)
{

  Room *room = nullptr;
  Message reciever_msg = Message();
  Message response_msg;

  // Handle login connection errors
  if (!(connection->receive(reciever_msg))) // makes sure the message was received
  {
    if (connection->get_last_result() == Connection::EOF_OR_ERROR) // different errors for failed to receive and invalid format
    {
      response_msg = Message(TAG_ERR, "Error: failed to receive  message");
    }
    else if (connection->get_last_result() == Connection::INVALID_MSG)
    {
      response_msg = Message(TAG_ERR, "Error: message format was invalid");
    }
    connection->send(response_msg); // send response message
    return;
  }

  // Need to first join the room before recieving any other tags
  if (reciever_msg.tag == TAG_JOIN)
  {
    room = server->find_or_create_room(reciever_msg.data); // find or make the room with tag join
    room->add_member(user);                                // add user to the room
    response_msg = Message(TAG_OK, "Success: joined room");
    if (!connection->send(response_msg))
    { // makes sure the messsage was sent
      return;
    }
  }
  else
  {
    response_msg = Message(TAG_ERR, "Error: must first join a room"); // error if there is something besides join
    connection->send(response_msg);
    return;
  }

  // Messages that send messages to the reciever once we have joined a room
  while (1)
  {
    Message *user_msg = user->mqueue.dequeue(); // removes the first message in the message queue

    if (user_msg != nullptr) // its possible to have an exmpty message in the queue so we need to check
    {
      if (!connection->send(*user_msg))
      { // if it fails, then break out and can return
        response_msg = Message(TAG_ERR, "Error: connection failed to send");
        connection->send(response_msg);
        break;
      }
    }
  }
  connection->close();
  room->remove_member(user); // remove user from the room
  return;
}

////////////////////////////////////////////////////////////////////////
// Server member function implementation
////////////////////////////////////////////////////////////////////////

Server::Server(int port)
    : m_port(port), m_ssock(-1)
{
  // TODO: initialize mutex
  pthread_mutex_init(&m_lock, nullptr);
}

Server::~Server()
{
  // destroy mutex
  pthread_mutex_destroy(&m_lock); // destroy mutex
}

bool Server::listen()
{
  // use open_listenfd to create the server socket, return true
  //       if successful, false if not
  m_ssock = open_listenfd(std::to_string(m_port).c_str());
  if (m_ssock < 0)
  {
    std::cerr << "Error: server socket could not be opened" << std::endl;
    return false;
  }
  return true; // socket successfully opened, so return true
}

void Server::handle_client_requests()
{
  // infinite loop calling accept or Accept, starting a new
  //       pthread for each connected client

  while (1)
  {
    int clientfd = Accept(m_ssock, nullptr, nullptr);
    if (clientfd < 0)
    {
      std::cerr << "Error: client could not accept connection" << std::endl;
      return;
    }

    struct ConnInfo *info = new ConnInfo();
    // Use Connection constructor with assumed TCP socket
    Connection *conn = new Connection(clientfd);
    info->connection = conn;
    info->server = this;

    pthread_t thr_id;
    // Handle the error appropriately, e.g., clean up resources
    if (pthread_create(&thr_id, nullptr, worker, info) != 0)
    {
      std::cerr << "Error: failed to create thread" << std::endl;
    }
  }
}

Room *Server::find_or_create_room(const std::string &room_name)
{
  // return a pointer to the unique Room object representing
  //       the named chat room, creating a new one if necessary

  Guard guard(m_lock);

  // Search for the room with the given name
  RoomMap::iterator it = m_rooms.find(room_name);

  if (it != m_rooms.end())
  {
    return it->second;
  }

  Room *new_room = new Room(room_name);
  m_rooms[room_name] = new_room;

  return new_room;
}
