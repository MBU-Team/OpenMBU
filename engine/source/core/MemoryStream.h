#pragma once
#include <cstdint>
#include <cstdbool>
#include <string>
#include "platform/types.h"
/*
*	Original implementation of MemoryStream in C++
*/
class MemoryStream
{
	U8* buffer;
	size_t bufferSize;
	size_t properSize;
	size_t position;

	bool checkEos(bool error = true);
	void reallocate();
public:
	MemoryStream();
	~MemoryStream();
	void createFromBuffer(U8* buffer, U32 count);
	void useBuffer(U8* buffer, U32 count);
	bool readBool();
	S64 readInt64();
	S32 readInt32();
	S16 readInt16();
	S8 readInt8();
	U64 readUInt64();
	U32 readUInt32();
	U16 readUInt16();
	U8 readUInt8();
	float readFloat();
	double readDouble();
	char readChar();
	unsigned char readUChar();
	std::string readString();

	void writeBool(bool);
	void writeInt64(S64);
	void writeInt32(S32);
	void writeInt16(S16);
	void writeInt8(S8);
	void writeUInt64(U64);
	void writeUInt32(U32);
	void writeUInt16(U16);
	void writeUInt8(U8);
	void writeFloat(float);
	void writeDouble(double);
	void writeChar(char);
	void writeUChar(unsigned char);
	void writeString(std::string);
	void writeBuffer(const char* data, U32 size);
	bool seek(U32 position);
	U32 tell();
	U32 length();
	U8* getBuffer();
};

