// Copyright (c) 2018 Daniel Zilles
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "configparser.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <stdexcept>

using namespace std;

ConfigParser::ConfigParser(string path) : mPathToConfig(path) {

  ifstream configFile;
  configFile.open( mPathToConfig );

  string line;
  string sectionName;
  string configName;
  string rawConfigContent;
  vector<string> configContent;

  size_t lineNbr = 1;
 
  while( getline(configFile, line) ) {

    line.erase(std::remove_if( line.begin(), line.end(),
    [l = std::locale {}](auto ch) { return std::isspace(ch, l); }), line.end());

    if (line.empty()) {
      lineNbr++;
      continue;
    }

    if (line.at(0) == ';') {
      lineNbr++;
      continue;
    }

    if(line.at(0) == '[' && line.at(line.length()-1) == ']') {
      sectionName = line.substr(1, line.length()-2);
      continue;
    }

    size_t equalSignPos = line.find('=');
    if(equalSignPos != string::npos) {
      configName = line.substr(0, equalSignPos);
      rawConfigContent = line.substr(equalSignPos+1, line.length()-1);
     
      stringstream ss(rawConfigContent);
      while (ss.good()) {
        string tmp;
        getline(ss, tmp, ',');
        configContent.push_back(tmp);
      }
    }

    else {
      string errorMessage = path + ":" + to_string(lineNbr) + ": parsing error \n" + line;
      throw std::runtime_error(errorMessage);
    }

    mConfigurations.insert(std::make_pair(sectionName + " - " + configName, configContent));
    configContent.clear();
    lineNbr++;
  }
} 

template <>
bool ConfigParser::aConfig<bool>(std::string section, std::string configName, size_t pos) {

  bool tmp;
  std::string config = mConfigurations[section + " - " + configName][pos];
  std::istringstream iss(config);

  if (config == "true" ||
      config == "TRUE" ||
      config == "1") {
    return true;
  }

  else if (config == "false" ||
           config == "FALSE" ||
      config == "0") {
    return false;
  }
  else
    return false;
}

template <>
std::vector<bool> ConfigParser::aConfigVec<bool>(std::string section, std::string configName) {

  std::vector<std::string> config;

  config = mConfigurations[section + " - " + configName];

  std::vector<bool>  tmp(config.size());
  for (unsigned i = 0; i < config.size(); i++) {

    if (config[i] == "true" ||
        config[i] == "TRUE" ||
        config[i] == "1") {
      tmp[i] = true;
    }

    else if (config[i] == "false" ||
             config[i] == "FALSE" ||
             config[i] == "0") {
      tmp[i] = false;
    }
    else
      tmp[i] = false;

  }
  return tmp;
}














