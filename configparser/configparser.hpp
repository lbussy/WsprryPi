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
//
// Repo:  https://github.com/dzilles/configparser

#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>

typedef std::map<std::string, std::vector<std::string> > configList; 

class ConfigParser {

  public:
  ConfigParser(std::string path);

    template<typename T>
    T aConfig(std::string section, std::string name, size_t pos = 0);
    template<typename T>
    std::vector<T> aConfigVec(std::string section, std::string name);

  private:

    const std::string mPathToConfig;
    configList mConfigurations;

};

template <>
bool ConfigParser::aConfig<bool>(std::string section, std::string name, size_t pos);

template <typename T>
T ConfigParser::aConfig(std::string section, std::string configName, size_t pos) {

  T tmp;
  std::string* config = &mConfigurations[section + " - " + configName][pos];
  std::istringstream iss(*config);

  if (config->find( "0x" ) != std::string::npos)
    iss >> std::hex >> tmp;
  else
    iss >> std::dec >> tmp;

  return tmp;
}

template <>
std::vector<bool> ConfigParser::aConfigVec<bool>(std::string section, std::string name);

template <typename T>
std::vector<T> ConfigParser::aConfigVec(std::string section, std::string configName) {

  std::vector<std::string> config;

  config = mConfigurations[section + " - " + configName];

  std::vector<T>  tmp(config.size());
  for (unsigned i = 0; i < config.size(); i++) {

    std::istringstream iss(config[i]);

    if (config[i].find( "0x" ) != std::string::npos)
      iss >> std::hex >> tmp[i];
    else
      iss >> std::dec >> tmp[i];
  }
  return tmp;
}
#endif
