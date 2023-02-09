#include <string>
#include <vector>
// Include the configparser header
#include "configparser.hpp"

using namespace std;

template<typename T>
void coutVector(vector<T> c, string comparison) {

  cout << "Output should be " << comparison << ": {";

  for (unsigned i = 0; i < c.size(); i++) {
    cout << c[i];
    if(i < c.size()-1) 
      cout << ", ";
    else
      cout << "}" << endl;
  }
}

int main() {

  string path = "config.ini";
  // Initialize the parser with a path to the configuration file
  ConfigParser parser = ConfigParser(path);
  
  // Read the configuration file entries:
  //
  // It is possible to read a single entry,
  string c11 = parser.aConfig<string>("Section1", "example1");
  cout << "Output should be string: " << c11 << endl; 

  int c12 = parser.aConfig<int>("Section1", "example2");
  cout << "Output should be 4: " << c12 << endl; 

  bool c13 = parser.aConfig<bool>("Section1", "example3");
  cout << "Output should be true or 1: " << c13 << endl; 

  string c16 = parser.aConfig<string>("Section1", "example6");
  cout << "Output should be \"test!?$§\": " << c16 << endl; 

  // a whole vector 
  vector<string> c14 = parser.aConfigVec<string>("Section1", "example4");
  coutVector(c14, "{string1, string2, string3}");

  vector<bool> c15 = parser.aConfigVec<bool>("Section1", "example5");
  coutVector(c15, "{true or 1, false or 0, true or 1}");

  vector<long long> c21 = parser.aConfigVec<long long>("Section2", "example1");
  coutVector(c21, "{1, 2, 3, 4}");

  vector<double> c22 = parser.aConfigVec<double>("Section2", "example2");
  coutVector(c22, "{5.1, 6.3, 7.5}");

  // or a single entry of a vector
  double c222 = parser.aConfig<double>("Section2", "example2", 2);
  cout << "Output should be 7.5: " << c222 << endl; 

}





















