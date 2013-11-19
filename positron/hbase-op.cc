#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <poll.h>
#include <string.h>
#include "hbase-op.h"
#include "common-macro.h"

#include <iostream>

#include <boost/lexical_cast.hpp>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/protocol/TBinaryProtocol.h>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
//using namespace apache::thrift::transport::TSocket;
using namespace apache::hadoop::hbase::thrift;

std::string HBaseOp::gCreditTbl;
char HBaseOp::ghostaddr[MAX_ADDR_STR];
int  HBaseOp::ghostport; 

int HBaseOp::getRow(std::string &aRes, std::string *aTbl, std::string *aRk,
                            const char *aCol) {
  bool isFramed = false;
  
  boost::shared_ptr<TTransport> socket(new TSocket(ghostaddr, ghostport));
  boost::shared_ptr<TTransport> transport;
  
  if (isFramed) {
    transport.reset(new TFramedTransport(socket));
  } else {
    transport.reset(new TBufferedTransport(socket));
  }
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  
  const std::map<Text, Text>  dummyAttributes;
  HbaseClient client(protocol);
  
  try {
    transport->open();
    std::vector<TCell> value;
    
    /*
     * void HbaseClient::get(std::vector<TCell> & _return, const Text& tableName, const Text& row, const Text& column, const std::map<Text, Text> & attributes)
     * {
     *  send_get(tableName, row, column, attributes);
     *  recv_get(_return);
     * }
     */
     
    client.get(value, *aTbl, *aRk, aCol, dummyAttributes);
    if (!value.size()) {
      std::cerr << "FATAL: get no value!" << std::endl;
      return FAILURE;
    } else {
      printCell(value);
    }
    
    transport->close();
    aRes = value.begin()->value;
    return SUCCESS;
  } catch (const TException &tx) {
    std::cerr << "ERROR: " << tx.what() << std::endl;
    return FAILURE;
  }
}

void HBaseOp::setHostAddr(char *aAddr) {
  snprintf(ghostaddr, MAX_ADDR_STR, aAddr);
}

void HBaseOp::setHostPort(int aPort) {
  ghostport = aPort;
}

void HBaseOp::setCreditTbl(char *aName) {
  gCreditTbl = std::string(aName);
}

void HBaseOp::printCell(const std::vector<TCell> &cellResult) {
  for (size_t i = 0; i < cellResult.size(); i++) {
    for (std::vector<TCell>::const_iterator it = cellResult.begin();
         it != cellResult.end(); ++it) {
        std::cout << "CELL" << " => " << it->value << "; ";
      }
      std::cout << std::endl;
    }
}
