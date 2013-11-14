
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include "exec-get.h"

int ExecGet::execGet(const char *aUri, struct evbuffer *aEvb) {
  char sCh;
  char *sSrc = NULL;
  char *sVal = NULL;
  int i = 1;
  int sSplit[MAX_URI_SPLIT] = {0};
  std::map<std::string, std::string> sKv;
  
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
    
    sSrc = (char *)sKv.at(std::string("src")).c_str();
    sVal = (char *)sKv.at(std::string("val")).c_str();
    
    //debuging
    printf("src = %s, val = %s\n", sSrc, sVal);
    //
    
    /*
     * HBase get request (ver. hbase-0.94.6-cdh4.3.0)
     */
    
    // copy result to event buffer
    evbuffer_add_printf(aEvb, "<html><head>test-test</head></html>\n");
    
    
    return SUCCESS;
  } else {
    printf("unknown request : %s\n", aUri);
    return FAILURE;
  }
}
