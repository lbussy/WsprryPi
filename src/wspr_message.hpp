#ifndef WSPR_MESSAGE_H
#define WSPR_MESSAGE_H 

// This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.

/*
 * WsprryPi
 * Updated and maintained by Lee C. Bussy
 *
 * Originally forked from threeme3/WsprryPi (no longer active), this
 * project has been significantly updated, improved, and documented for
 * ease of use.
 *
 * Inspired by a conversation with Bruce Raymond of TAPR, this fork has
 * diverged substantially from its origins and operates as an independent
 * project.
 *
 * Contributors:
 *   - threeme3 (Original Author)
 *   - Bruce Raymond (Inspiration and Guidance)
 *   - Lee Bussy, aa0nt@arrl.net
 *
 * Copyright (C) 2023-2024 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-55ad7f3 [fix_57].
 */

#include <cstdint>
#include <cstring>
#include <cctype>

#define MSG_SIZE 162

class WsprMessage {

private:

  int getCharValue(unsigned char ch);
  int calculateParity(uint32_t ch);
  unsigned char reverseAddress(unsigned char &reverseAddressIndex);
  unsigned char reverseBits(unsigned char b);
  void generateSymbols(char * callsign, char * location, int power);

public:

  WsprMessage(char * callsign, char * location, int power);
  ~WsprMessage(); // Destructor to free allocated memory
  unsigned char * symbols;

  static constexpr int size = MSG_SIZE; // Static constant member
};

#endif // _WSPR_MESSAGE_HPP
