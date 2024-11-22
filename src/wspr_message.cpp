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

#include "wspr_message.hpp"
#include <cstdint>
#include <iostream>   // For std::cout, std::endl
#include "utils.hpp"

/*
162 bit synchronisation vector
*/
const unsigned char sync[] = {

  1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0,
  1, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0,
  0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1,
  0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0,
  1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1,
  1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1,
  0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1,
  1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0
  };


WsprMessage::WsprMessage(char *callsign, char *location, int power) {
    // Ensure callsign and location are in uppercase
    to_upper(callsign);
    to_upper(location);

    // Generate symbols
    generateSymbols(callsign, location, power);
}
void WsprMessage::generateSymbols(char * callsign, char * location, int power) {

  symbols = new unsigned char[MSG_SIZE];

  //=================================================================
  //call sign encoding (28 bits)
  //=================================================================

  /*
    Tidy the call sign and check for 1 or 2 letter prefix.
    Return a 6 char call sign with leading space if necessary.

    The third character is forced to be always a number, therefore a space is
    prepended as necessary. For example, G6AML will become [sp]G6AML.
  */

  char call[6];
  for (int i = 0; i < 6; i++) call[i] = ' ';

  if (isdigit(callsign[1])) {
    for (int i = 0; i < (int)strlen(callsign); i++)
    call[1 + i] = callsign[i];
  }
  else if (isdigit(callsign[2])) {
    for (int i = 0; i < (int)strlen(callsign); i++)
    call[i] = callsign[i];
  }

  /*
    The 37 allowed characters are allocated values from 0 to 36 
    such that "0" - "9" give 0 - 9,
    "A" to "Z" give 10 to 35 and [space] 
    is given the value 36. Further coding rules on
    callsigns mean that the final three characters (of the now padded out callsign) can only
    be letters or [sp] so will only take the values 10 to 36. This allows the callsign
    to be becompressed into a single integer 'N'. e.g.

    N1 = [Ch 1] The first character can take on any of the 37 values including[sp],
    N2 = N1 * 36 + [Ch 2] but the second character cannot then be a space so can have 36 values
    N3 = N2 * 10 + [Ch 3] The third character must always be a number, so only 10 values are possible.
    N4 = 27 * N3 + [Ch 4] - 10]
    N5 = 27 * N4 + [Ch 5] - 10] Characters at the end cannot be numbers,
    N6 = 27 * N5 + [Ch 6] - 10] so only 27 values are possible.
  */

  uint32_t N = getCharValue(call[0]);
  N *= 36; N += getCharValue(call[1]);
  N *= 10; N += getCharValue(call[2]);

  //the following can not be numbers so only 27 values are possible
  N *= 27; N += getCharValue(call[3]) - 10;
  N *= 27; N += getCharValue(call[4]) - 10;
  N *= 27; N += getCharValue(call[5]) - 10;

  //=================================================================
  //location encoding (15 bits)
  //=================================================================
  /*
    Designating the four locator characters as[Loc 1] to[Loc 4], the first two can each take
    on the 18 values 'A' to 'R' only so are allocated numbers from 0 - 17. The second pair
    can take only the values 0 - 9.
    Another integer M is formed from :

    M1 = (179 - 10 * [Loc 1] - [Loc 3]) * 180 + 10 * [Loc 2] + [Loc 4]

    Which gives a range of values from 'AA00' = 32220 to 'RR99' = 179, which comfortably
    fit into a 15 bit representation(215 = 32768), leaving spare codes for further
    enhancements.
  */

  uint32_t M1 = (179 - 10 * (location[0] - 'A') - (location[2] - '0')) * 180
    + (10 * (location[1] - 'A')) + (location[3] - '0');

  //=================================================================
  //power encoding (7 bits)
  //=================================================================
  /*
    Power level, [Pwr] is taken as a value from 0 - 60. Although only certain values will work
    with the WSJT / WSPR software(just those ending in 0, 3 or 7) any of the possible 61
    values will be encoded; Illegal values are labeled when decoded. Power is encoded
    into M by : M = M1 * 128 + [Pwr] + 64
  */

  uint32_t M = M1 * 128 + power + 64;

  //Summary: N has call sign (28 bits), M has location and power (22 bits) 

  //=================================================================
  //convolution encoding and interleaving
  //=================================================================
  /*
    The data is now expanded to add FEC with a convolutional
    encoder.

    The 81 bits (including the 31 trailing zeros) are  clocked simultaneously into the
    right hand side, or least significant position, of a 32 bit shift register.
    The shift register feeds an ExclusiveOR parity generator from feedback taps described
    respectively by the 32 bit values 0xF2D05351 and 0xE4613C47.

    Parity generation starts immediately the first bit appears in the registers and continues
    until the registers are flushed by the final 31st zero being clocked into them. 

    Each of the 81 bits shifted in generates a parity bit
    from each of the generators, a total of 162 bits in all.For each bit shifted in,
    the resulting two parity bits are taken in turn, in the order the two feedback tap
    positions values are given, to give a stream of 162 output bits.

    The interleaving process is performed by taking a bit reversal of the address (array index)
    to reorder them.
  */

  int i;
  uint32_t reg = 0;
  unsigned char reverseAddressIndex = 0;

  /*
    We need to merge the source with the synchronisation bits
    so transfer these across first as this allows us to merge,
    encode and interleave at the same time
  */
  for (i = 0; i < MSG_SIZE; i++) {
     
    symbols[i] = sync[i];
  }

  //move N (callsign) into the symbols byte array
  for (i = 27; i >= 0; i--) {

    //make room for the next register bit
    reg <<= 1; // same as reg = reg << 1

    //if bit 'i' of Nc (call sign) is a '1', set LSB of the register
    //with the bitwise OR operator
    if (N & ((uint32_t)1 << i)) reg |= 1;

    /*
      These next two lines add the parity data to the symbols array
      but in the interleved order using a reverse of the address
      the constant values (feedback taps) come directly from the
      specification.
    */
    symbols[reverseAddress(reverseAddressIndex)] += 2 * calculateParity(reg & 0xf2d05351L);
    symbols[reverseAddress(reverseAddressIndex)] += 2 * calculateParity(reg & 0xe4613c47L);
  }

  //the register (reg) should be maintained ready for the next loop

  //move M (location and power) into the symbols array in the same way
  //as N above
  for (i = 21; i >= 0; i--) {
    reg <<= 1;
    if (M & ((uint32_t)1 << i)) reg |= 1;
    symbols[reverseAddress(reverseAddressIndex)] += 2 * calculateParity(reg & 0xf2d05351L);
    symbols[reverseAddress(reverseAddressIndex)] += 2 * calculateParity(reg & 0xe4613c47L);
  }

  //pad with 31 zero bits
  for (i = 30; i >= 0; i--) {
    reg <<= 1;
    symbols[reverseAddress(reverseAddressIndex)] += 2 * calculateParity(reg & 0xf2d05351L);
    symbols[reverseAddress(reverseAddressIndex)] += 2 * calculateParity(reg & 0xe4613c47L);
  }
}

int WsprMessage::getCharValue(unsigned char ch)
{
  if (isdigit(ch)) return ch - '0';
  if (isalpha(ch)) return 10 + toupper(ch) - 'A';
  if (ch == ' ') return 36;

  //if we get to here then call sign is no good
  //throw exception;
  return 0;
}

unsigned char WsprMessage::reverseAddress(unsigned char &reverseAddressIndex) {
  /*
    The index of each bit within the 162 bit message is passed
    to this function and the bits are reversed.

    E.g. 1 becomes 128 2 become 64 etc. Where the reversed
    value is greater than 161 it is ignored and we move the index
    on. For example bit 3 would become 192 when reversed, therefore
    we move on and use the value 4 for bit 3 and 5 for bit 4
    and so on. e have 255 addresses we can use to get the 162 address
    values we need.
  */
  unsigned char result = reverseBits(reverseAddressIndex++);

  while (result > 161) {
    result = reverseBits(reverseAddressIndex++);
  }
  return result;
}

unsigned char WsprMessage::reverseBits(unsigned char b) {
  /*
    This function reverses the bits in a byte. I.e. 1 becomes
    128, 2 becomes 64, 3 becomes 192 and so on.

    The left four bits are swapped with the right four bits,
    all adjacent pairs are swapped, all adjacent single bits
    are swapped.

    Thanks to http://stackoverflow.com/users/56338/sth for
    this snippet of code
  */
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;

  return b;
}

int WsprMessage::calculateParity(uint32_t x)
{
  //generate XOR parity bit (returned as 0 or 1)
  int even = 0;
  while (x) {
    even = 1 - even;
    x = x & (x - 1);
  }
  return even;
}

WsprMessage::~WsprMessage() {
    delete[] symbols; // Free the dynamically allocated memory
}

// Debug only main()
#ifdef DEBUG_MAIN_MESSAGE
int main() {
    // Example test data
    char callsign[] = "AA0NT"; // Callsign example
    char location[] = "EM18";  // Maidenhead grid location
    int power = 20;            // Power level in dBm

    // Create a WsprMessage object
    WsprMessage message(callsign, location, power);

    // Output the provided data
    std::cout << "Callsign: " << callsign << std::endl;
    std::cout << "Location: " << location << std::endl;
    std::cout << "Power: " << power << " dBm" << std::endl;

    // Output the generated symbols
    std::cout << "Generated WSPR symbols (" << WsprMessage::size << "):" << std::endl;

    for (int i = 0; i < WsprMessage::size; ++i) {
        std::cout << static_cast<int>(message.symbols[i]);
        if (i < WsprMessage::size - 1) {
            std::cout << ",";
        }
    }
    std::cout << std::endl;

    return 0;
}
#endif
