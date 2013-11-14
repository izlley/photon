#ifndef UTILPARSER_H
#define UTILPARSER_H

#include <ostream>
#include <map>
#include <string>

class UtilParser {
 public:
  static int splitUri(const char *aUri, char aDelimiter, int aInd[]);
  static int splitKVs(const char *aUri, std::map<std::string,std::string> *sKv);
};
#endif