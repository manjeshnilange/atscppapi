/*
 * Copyright (c) 2013 LinkedIn Corp. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the license at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.
 *
 */

/**
 * @file Transaction.h
 * @author Brian Geffon
 * @author Manjesh Nilange
 */

#pragma once
#ifndef ATSCPPAPI_TRANSACTION_H_
#define ATSCPPAPI_TRANSACTION_H_

#include <sys/socket.h>
#include <stdint.h>
#include <list>
#include "atscppapi/Request.h"
#include "atscppapi/shared_ptr.h"
#include "atscppapi/ClientRequest.h"
#include "atscppapi/Response.h"

namespace atscppapi {

// forward declarations
class TransactionPlugin;
class TransactionState;
namespace utils { class internal; }

/**
 * @brief Transactions are the object containing all the state related to a HTTP Transaction
 *
 * @warning Transactions should never be directly created by the user, they will always be automatically
 * created and destroyed as they are needed. Transactions should never be saved beyond the
 * scope of the function in which they are delivered otherwise undefined behaviour will result.
 */
class Transaction: noncopyable {
public:
  /**
   * @brief ContextValues are a mechanism to share data between plugins using the atscppapi.
   *
   * Any data can be shared so long as it extends ContextValue, a simple example might
   * be:
   *
   * \code
   *     struct mydata : ContextValue {
   *       int id_;
   *       string foo_;
   *       mydata(int id, string foo) : id_(id), foo_(foo) { }
   *     }
   *
   *     Transaction.setContextValue("some-key", shared_ptr(new mydata(12, "hello")));
   *
   *     // From another plugin you'll have access to this contextual data:
   *     shared_ptr<Transaction.getContextValue("some-key")
   *
   * \endcode
   *
   * Because getContextValue() and setContextValue()
   * take shared pointers you dont have to worry about the cleanup as that will happen automatically so long
   * as you dont have shared_ptrs that cannot go out of scope.
   */
  class ContextValue {
  public:
    virtual ~ContextValue() { }
  };

  ~Transaction();

  /**
   * Context Values are a way to share data between plugins, the key is always a string
   * and the value can be a shared_ptr to any type that extends ContextValue.
   * @param key the key to search for.
   * @return Shared pointer that is correctly initialized if the
   *         value existed. It should be checked with .get() != NULL before use.
   */
  shared_ptr<ContextValue> getContextValue(const std::string &key);

  /**
   * Context Values are a way to share data between plugins, the key is always a string
   * and the value can be a shared_ptr to any type that extends ContextValue.
   * @param key the key to insert.
   * @param value a shared pointer to a class that extends ContextValue.
   */
  void setContextValue(const std::string &key, shared_ptr<ContextValue> value);

  /**
   * Causes the Transaction to continue on to other states in the HTTP state machine
   * If you do not call resume() on a Transaction it will remain in that state until
   * it's advanced out by a call to resume() or error().
   */
  void resume();

  /**
   * Causes the Transaction to advance to the error state in the HTTP state machine.
   * @see error(const std::string &)
   */
  void error();

  /**
   * Causes the Transaction to advance to the error state in the HTTP state machine with
   * a specific error message displayed. This is functionally equivalent to the following:
   *
   * \code
   * setErrorBody(content);
   * error();
   * \endcode
   *
   * @param content the error page body.
   */
  void error(const std::string &content);

  /**
   * Sets the error body page but this method does not advance the state machine to the error state.
   * To do that you must explicitally call error().
   *
   * @param content the error page content.
   */
  void setErrorBody(const std::string &content);

  /**
   * Get the clients address
   * @return The sockaddr structure representing the client's address
   * @see atscppapi::utils::getIpString() in atscppapi/utils.h
   * @see atscppapi::utils::getPort() in atscppapi/utils.h
   * @see atscppapi::utils::getIpPortString in atscppapi/utils.h
   */
  const sockaddr *getClientAddress() const;

  /**
   * Get the incoming address
   * @return The sockaddr structure representing the incoming address
   * @see atscppapi::utils::getIpString() in atscppapi/utils.h
   * @see atscppapi::utils::getPort() in atscppapi/utils.h
   * @see atscppapi::utils::getIpPortString in atscppapi/utils.h
   */
  const sockaddr *getIncomingAddress() const;

  /**
   * Get the server address
   * @return The sockaddr structure representing the server's address
   * @see atscppapi::utils::getIpString() in atscppapi/utils.h
   * @see atscppapi::utils::getPort() in atscppapi/utils.h
   * @see atscppapi::utils::getIpPortString in atscppapi/utils.h
   */
  const sockaddr *getServerAddress() const;

  /**
   * Get the next hop address
   * @return The sockaddr structure representing the next hop's address
   * @see atscppapi::utils::getIpString() in atscppapi/utils.h
   * @see atscppapi::utils::getPort() in atscppapi/utils.h
   * @see atscppapi::utils::getIpPortString in atscppapi/utils.h
   */
  const sockaddr *getNextHopAddress() const;


  /**
   * Set the incoming port on the Transaction
   *
   * @param port is the port to set as the incoming port on the transaction
   */
  bool setIncomingPort(uint16_t port);

  /**
   * Sets the server address on the Transaction to a populated sockaddr *
   *
   * @param sockaddr* the sockaddr structure populated as the server address.
   */
  bool setServerAddress(const sockaddr *);

  /**
   * Returns a boolean value if the request is an internal request.
   * A request is an internal request if it originates from within traffic server.
   * An example would be using TSFetchUrl (or the atscppapi equivalent of AsyncHttpFetch)
   * to make another request along with the original request. The secondary request
   * originated within traffic server and is an internal request.
   *
   * @return boolean value specifying if the request was an internal request.
   */
  bool isInternalRequest() const;

  /**
   * Returns the ClientRequest object for the incoming request from the client.
   *
   * @return ClientRequest object that can be used to manipulate the incoming request from the client.
   */
  ClientRequest &getClientRequest();

  /**
   * Returns a Request object which is the request from Traffic Server to the origin server.
   *
   * @return Request object that can be used to manipulate the outgoing request to the origin server.
   */
  Request &getServerRequest();

  /**
   * Returns a Response object which is the response coming from the origin server
   *
   * @return Response object that can be used to manipulate the incoming response from the origin server.
   */
  Response &getServerResponse();

  /**
   * Returns a Response object which is the response going to the client
   *
   * @return Response object that can be used to manipulate the outgoing response from the client.
   */
  Response &getClientResponse();

  /**
   * The available types of timeouts you can set on a Transaction.
   */
  enum TimeoutType {
    TIMEOUT_DNS = 0, /**< Timeout on DNS */
    TIMEOUT_CONNECT, /**< Timeout on Connect */
    TIMEOUT_NO_ACTIVITY, /**< Timeout on No Activity */
    TIMEOUT_ACTIVE /**< Timeout with Activity */
  };

  /**
   * Allows you to set various types of timeouts on a Transaction
   *
   * @param type The type of timeout
   * @param time_ms The timeout time in milliseconds
   * @see TimeoutType
   */
  void setTimeout(TimeoutType type, int time_ms);

  /**
   * Returns the TSHttpTxn related to the current Transaction
   *
   * @return a void * which can be cast back to a TSHttpTxn.
   */
  void *getAtsHandle() const;

  /**
   * Adds a TransactionPlugin to the current Transaction. This effectively transfers ownership and the
   * Transaction is now responsible for cleaning it up.
   *
   * @param TransactionPlugin* the TransactionPlugin that will be now bound to the current Transaction.
   */
  void addPlugin(TransactionPlugin *);

private:
  TransactionState *state_; //!< The internal TransactionState object tied to the current Transaction
  friend class TransactionPlugin; //!< TransactionPlugin is a friend so it can call addPlugin()
  friend class TransformationPlugin; //!< TransformationPlugin is a friend so it can call addPlugin()

  /**
   * @private
   *
   * @param raw_txn a void pointer that represents a TSHttpTxn
   */
  Transaction(void *);

  /**
   * Used to initialize the Request object for the Server.
   *
   * @private
   */
  void initServerRequest();

  /**
   * Used to initialize the Response object for the Server.
   *
   * @private
   */
  void initServerResponse();

  /**
   * Used to initialize the Response object for the Client.
   *
   * @private
   */
  void initClientResponse();

  /**
   * Returns a list of TransactionPlugin pointers bound to the current Transaction
   *
   * @private
   *
   * @return a std::list<TransactionPlugin *> which represents all TransactionPlugin bound to the current Transaction.
   */
  const std::list<TransactionPlugin *> &getPlugins() const;

  friend class utils::internal;
};

} /* atscppapi */

#endif /* ATSCPPAPI_TRANSACTION_H_ */
