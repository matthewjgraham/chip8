#pragma once

#define firstNibble$(inst) ((inst & 0b1111000000000000) >> 12)
#define secondNibble$(inst) ((inst & 0b0000111100000000) >> 8)
#define thirdNibble$(inst) ((inst & 0b0000000011110000) >> 4)
#define fourthNibble$(inst) (inst & 0b0000000000001111)

#define decodeX$(inst) secondNibble$(inst)
#define decodeY$(inst) thirdNibble$(inst)
#define decodeN$(inst) fourthNibble$(inst)
#define decodeNN$(inst) (inst & 0b0000000011111111)
#define decodeKK$(inst) decodeNN$(inst)
#define decodeNNN$(inst) (inst & 0b0000111111111111)

