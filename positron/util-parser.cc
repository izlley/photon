
#include <stdio.h>
#include <stdlib.h>
#include "util-parser.h"
#include "common-macro.h"

int UtilParser::splitUri(const char *aUri,
                         const char aDelimiter,
                         int aInd[]) {
  int i;
  int j = 0;
  
  for (i = 0; aUri[i] != '\0'; i++) {
    if (aUri[i] == aDelimiter) {
      aInd[j++] = i;
    }
  }
  for (;j < MAX_URI_SPLIT; j++)
    aInd[j] = -1;

  return SUCCESS;
}

// TODO: need more optimization for performance
int UtilParser::splitKVs(const char *aUri, std::map<std::string,std::string> *sKv) {
  int i = 0;
  int sBeginInd = 0;
  char sCh;
  bool sIsEq = false;
  std::string sUri(aUri);
  std::string sKey, sVal;
  
  while ((sCh = sUri[i++]) != '\0') {
    switch (sCh) {
      case '=':
        sIsEq = true;
        sKey = sUri.substr(sBeginInd, i - sBeginInd - 1);
        sBeginInd = i;
        break;
      case '&':
        sIsEq = false;
        sVal = sUri.substr(sBeginInd, i - sBeginInd - 1);
        sBeginInd = i;
        if (sKey.length() > 0) {
          sKv->insert(std::pair<std::string,std::string>(sKey, sVal));
        }
        break;
    }
  }
  if (sIsEq = true && sKey.length() > 0) {
    sVal = sUri.substr(sBeginInd, i - sBeginInd - 1);
    sKv->insert(std::pair<std::string,std::string>(sKey, sVal));
  }
  
  return SUCCESS;
}