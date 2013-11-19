
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include "exec-get.h"
#include "hbase-op.h"
#include "common-macro.h"

int ExecGet::execGet(const char *aUri, struct evbuffer *aEvb) {
  char sCh;
  //char *sSrc = NULL;
  //char *sVal = NULL;
  int i = 1;
  int sSplit[MAX_URI_SPLIT] = {0};
  int sResLen = 0;
  int sRes;
  char sResult[MAX_CREDIT_RES_SIZE] = {0};
  std::map<std::string, std::string> sKv;
  std::string sGetRes, sSrc, sVal;
  
  printf("execGet : %s\n", aUri); 
  // TODO:error check
  UtilParser::splitUri(aUri, '/', sSplit);
  
  if (!strncmp("credit", aUri + sSplit[0] + 1,
              (sSplit[1]==-1)? sizeof("credit"): sSplit[1]-sSplit[0]-1)) {
    printf("get credit requested : %s\n", aUri);
    switch (aUri[sSplit[1] + 1]) {
      case '?':
        UtilParser::splitKVs((const char *)(aUri + sSplit[1] + 2), &sKv);
        break;
      default:
        return FAILURE;
    }
    
    //debuging
    std::map<std::string,std::string>::iterator it = sKv.begin();
    for (it=sKv.begin(); it!=sKv.end(); ++it) {
      printf("K=%s, V=%s\n", (it->first).c_str(), (it->second).c_str());
    }
    //
    
    sSrc = sKv.at(std::string("src"));
    sVal = sKv.at(std::string("val"));
    
    //debuging
    printf("src = %s, val = %s\n", sSrc.c_str(), sVal.c_str());
    //
    
    /*
     * HBase get request (ver. hbase-0.94.6-cdh4.3.0)
     */
    sRes = HBaseOp::getRow(sGetRes, &(HBaseOp::gCreditTbl), &sVal, "f:lvl");
    
    if (sRes == SUCCESS) {
      sResLen = sSrc.length() + sGetRes.length() + 4 + 16;
      if (sResLen < MAX_CREDIT_RES_SIZE) {
        snprintf(sResult, MAX_CREDIT_RES_SIZE, "{\"src\"=\"%s\",\"data\"=\"%s\"}",
                 sVal.c_str(), sGetRes.c_str());
      } else {
        // size error
        snprintf(sResult, MAX_CREDIT_RES_SIZE, "{\"error\"=\"result size is too big\"}");
      }
    } else {
      // error
      snprintf(sResult, MAX_CREDIT_RES_SIZE, "{\"error\"=\"core error\"}");
    }
    
    // copy result to event buffer
    evbuffer_add_printf(aEvb, sResult);
    
    
    return SUCCESS;
  } else {
    printf("unknown request : %s\n", aUri);
    return FAILURE;
  }
}

