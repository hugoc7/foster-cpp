#include "Packing.h"

namespace Packing {

	void Write(Uint8 n, Uint8* buffer) {
		assert(buffer != nullptr);
		struct_pack(buffer, "B", n);
	}
	void Write(Uint16 n, Uint8* buffer) {
		assert(buffer != nullptr);
		SDLNet_Write16(n, buffer);
	}
	void Write(Uint32 n, Uint8* buffer) {
		assert(buffer != nullptr);
		SDLNet_Write32(n, buffer);
	}
	void Write(float f, Uint8* buffer) {
		assert(buffer != nullptr);
		struct_pack(buffer, "!f", f);
	}
	float ReadFloat(Uint8* buffer) {
		assert(buffer != nullptr);
		float res;
		struct_unpack(buffer, "!f", &res);
		return res;
	}
	Uint32 ReadUint32(Uint8* buffer) {
		assert(buffer != nullptr);
		return SDLNet_Read32(buffer);
	}
	Uint16 ReadUint16(Uint8* buffer) {
		assert(buffer != nullptr);
		return SDLNet_Read16(buffer);
	}
	Uint8 ReadUint8(Uint8* buffer) {
		assert(buffer != nullptr);
		Uint8 n;
		struct_unpack(buffer, "B", &n);
		return n;
	}
}