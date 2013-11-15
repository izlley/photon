#include <stdio.h>
#include <string.h>
#include "hbase-op.h"

#include <iostream>

#include <boost/lexical_cast.hpp>
#include <protocol/TBinaryProcotol.h>
#include <transport/TSocket.h>
#include <transport/TTransportUtils.h>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::hadoop::hbase::thrift;

static void HBaseOp::getRow(std::string *aTbl, std::string *aRk,
                            const char *aCol) {
  bool isFramed = false;
  
  boost::shared_ptr<TTransport> socket(new TSocket(hostaddr, hostport));
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
     
    client.get(value, *aTbl, *aRk, "entry:foo", dummyAttributes);
    if (!value.size()) {
      std::cerr << "FATAL: get no value!" << std::endl;
    } else {
      printCell(value);
    }
    
    transport->close();
  } catch (const TException &tx) {
    std::cerr << "ERROR: " << tx.what() << std::endl;
  }
  
  
  
}

static void HBaseOp::setHostAddr(char *aAddr) {
  snprintf(hostaddr, MAX_ADDR_STR, aAddr);
}

static void HBaseOp::setHostPort(int aPort) {
  hostport = aPort;
}

static void HBaseOp::setCreditTbl(char *aName) {
  gCreditTbl = std::string(aName);
}

static void HBaseOp::printCell(const std::vector<TCell> &cellResult) {
  for (size_t i = 0; i < cellResult.size(); i++) {
    for (std::vector<TCell>::iterator it = cellResult.begin();
         it != cellResult.end(); ++it) {
        std::cout << "CELL" << " => " << it->value << "; ";
      }
      std::cout << std::endl;
    }
}
