#ifndef HBASEOP_H
#define HBASEOP_H

#include <Hbase.h>

#define MAX_ADDR_STR 20

class HBaseOp {
 public:
  static void getRow(std::string *aTbl, std::string *aRk,
                     const char *aCol);
  static void setHostAddr(char *aAddr);
  static void setHostPort(int aPort);
  static void setCreditTbl(char *aName);
  static char* getHostAddr(){ return hostaddr; };
  static int   getHostPort(){ return hostport; };
  static std::string *getCreditTbl(){ return &gCreditTbl; };
  
 private:
  static char hostaddr[MAX_ADDR_STR];
  static int  hostport;
  static std::string gCreditTbl; 
  static void printCell(const std::vector<TCell> &cellResult);
};
#endif