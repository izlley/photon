#ifndef HBASEOP_H
#define HBASEOP_H

#include <Hbase.h>

#define MAX_ADDR_STR 20

class HBaseOp {
 private:
  static void printCell(const std::vector<apache::hadoop::hbase::thrift::TCell> &cellResult);
 public:
  static std::string gCreditTbl;
  static char ghostaddr[MAX_ADDR_STR];
  static int  ghostport; 
  
  static int getRow(std::string &aRes, std::string *aTbl, std::string *aRk,
                    const char *aCol);
  static void setHostAddr(char *aAddr);
  static void setHostPort(int aPort);
  static void setCreditTbl(char *aName);
};
#endif