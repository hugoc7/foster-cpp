#pragma once

#include "struct/struct.h"
#include "SDL_Net.h"
#include <cassert>

//PACKING.H
//Provide utiliy functions to pack/unpack C++ data types (float, int, char[]) in a byte buffer
//Packing means converting a data in a binary representation indepedant of the machine

//TODO: check return errors !!! 

namespace Packing {
	void WriteUint8(Uint8 n, Uint8* buffer);
	void WriteUint16(Uint16 n, Uint8* buffer);
	void WriteUint32(Uint32 n, Uint8* buffer);
	void WriteFloat(float f, Uint8* buffer);
	float ReadFloat(Uint8* buffer);
	Uint32 ReadUint32(Uint8* buffer);
	Uint16 ReadUint16(Uint8* buffer);
	Uint8 ReadUint8(Uint8* buffer);
}