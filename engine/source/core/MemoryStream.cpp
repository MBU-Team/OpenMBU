#include "MemoryStream.h"
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <platform/platform.h>
MemoryStream::MemoryStream()
{
    this->buffer = (U8*) dMalloc(256);
	this->bufferSize = 256;
	this->properSize = 0;
	this->position = 0;
}

MemoryStream::~MemoryStream()
{
	if (this->buffer != NULL)
		dFree(this->buffer);
		// delete[] this->buffer;
}

void MemoryStream::createFromBuffer(U8* buffer, U32 count)
{
	dFree(this->buffer);
	this->buffer = (U8*)dMalloc(count);
	this->bufferSize = count;
	this->properSize = count;
	this->position = 0;
	dMemcpy(this->buffer, buffer, count);
}

void MemoryStream::useBuffer(U8* buffer, U32 count)
{
	dFree(this->buffer);
	this->buffer = buffer;
	this->bufferSize = count;
	this->properSize = count;
	this->position = 0;
}

char MemoryStream::readChar()
{
	unsigned char chr = readUChar();
	return *(char*)&chr;
}

S8 MemoryStream::readInt8()
{
	U8 n = readUInt8();
	return *(S8*)&n;
}

S16 MemoryStream::readInt16()
{
	S16 n = readUInt16();
	return *(S16*)&n;
}

S32 MemoryStream::readInt32()
{
	S32 n = readUInt32();
	return *(S32*)&n;
}

S64 MemoryStream::readInt64()
{
	S64 n = readUInt64();
	return *(S64*)&n;
}

unsigned char MemoryStream::readUChar()
{
	return readUInt8();
}

bool MemoryStream::readBool()
{
	return readUInt8();
}

U8 MemoryStream::readUInt8()
{
	checkEos();
	U8 ret = this->buffer[this->position];
	this->position++;
	return ret;
}

U16 MemoryStream::readUInt16()
{
	checkEos();
	U16 num = 0;
	for (int i = 0; i < sizeof(U16); i++)
	{
		U8 byte = readUInt8();
		num += (static_cast<U16>(byte) << (8 * i));
	}
	return num;
}

U32 MemoryStream::readUInt32()
{
	checkEos();
	U32 num = 0;
	for (int i = 0; i < sizeof(U32); i++)
	{
		U8 byte = readUInt8();
		num += (static_cast<U32>(byte) << (8 * i));
	}
	return num;
}

U64 MemoryStream::readUInt64()
{
	checkEos();
	U64 num = 0;
	for (int i = 0; i < sizeof(U64); i++)
	{
		U8 byte = readUInt8();
		num += (static_cast<U64>(byte) << (8 * i));
	}
	return num;
}

float MemoryStream::readFloat()
{
	checkEos();
	U8 bytes[] = { readUInt8(), readUInt8(), readUInt8(), readUInt8() };
	float ret;
	dMemcpy(&ret, bytes, 4);
	return ret;
}

double MemoryStream::readDouble()
{
	checkEos();
	U64 bytes = readUInt64();
	double ret = *reinterpret_cast<double*>(&bytes);
	return ret;
}

std::string MemoryStream::readString()
{
	int len = readChar();
	char* str = new char[len + 1];
	for (int i = 0; i < len; i++)
		str[i] = readChar();
	str[len] = '\0';
	return std::string(str);
}

void MemoryStream::reallocate()
{
	while (this->position >= this->bufferSize)
	{
		this->bufferSize *= 1.5;
		this->buffer = (U8*)dRealloc(this->buffer, this->bufferSize);
		//U8* newBuffer = new U8[this->bufferSize]();
		//memcpy(newBuffer, this->buffer, this->bufferSize - this->REALLOCATE_SIZE);
		//delete[] this->buffer;
		//this->buffer = newBuffer;
	}
}

void MemoryStream::writeChar(char chr)
{
	if (this->position == this->properSize) {
		reallocate();
		this->properSize++;
	}
	this->buffer[this->position] = chr;
	this->position++;
}

void MemoryStream::writeInt8(S8 i8)
{
	writeChar(i8);
}

void MemoryStream::writeBool(bool b)
{
	writeUInt8(b);
}

void MemoryStream::writeUChar(unsigned char chr)
{
	if (this->position == this->properSize) {
		reallocate();
		this->properSize++;
	}
	this->buffer[this->position] = chr;
	this->position++;
}

void MemoryStream::writeInt16(S16 i16)
{
	reallocate();

	for (int i = 0; i < sizeof(S16); i++)
	{
		U8 byte = (i16 >> (8 * i)) & 0xFF;
		writeUInt8(byte);
	}
}

void MemoryStream::writeInt32(S32 i32)
{
	reallocate();
	for (int i = 0; i < sizeof(S32); i++)
	{
		U8 byte = (i32 >> (8 * i)) & 0xFF;
		writeUInt8(byte);
	}
}

void MemoryStream::writeInt64(S64 i64)
{
	reallocate();
	for (int i = 0; i < sizeof(S64); i++)
	{
		U8 byte = (i64 >> (8 * i)) & 0xFF;
		writeUInt8(byte);
	}
}

void MemoryStream::writeUInt8(U8 i8)
{
	writeUChar(i8);
}

void MemoryStream::writeUInt16(U16 i16)
{
	reallocate();
	for (int i = 0; i < sizeof(U16); i++)
	{
		U8 byte = (i16 >> (8 * i)) & 0xFF;
		writeUInt8(byte);
	}
}

void MemoryStream::writeUInt32(U32 i32)
{
	reallocate();
	for (int i = 0; i < sizeof(U32); i++)
	{
		U8 byte = (i32 >> (8 * i)) & 0xFF;
		writeUInt8(byte);
	}
}

void MemoryStream::writeUInt64(U64 i64)
{
	reallocate();
	for (int i = 0; i < sizeof(U64); i++)
	{
		U8 byte = (i64 >> (8 * i)) & 0xFF;
		writeUInt8(byte);
	}
}

void MemoryStream::writeFloat(float f)
{
	reallocate();
	union {
		float a;
		U8 bytes[4];
	} floatbytes;
	floatbytes.a = f;
	writeUInt8(floatbytes.bytes[0]);
	writeUInt8(floatbytes.bytes[1]);
	writeUInt8(floatbytes.bytes[2]);
	writeUInt8(floatbytes.bytes[3]);
}

void MemoryStream::writeDouble(double d)
{
	reallocate();
	U64 reinterpreted = *reinterpret_cast<U64*>(&d);
	writeUInt64(reinterpreted);
}

void MemoryStream::writeString(std::string s)
{
	reallocate();
	int len = s.length();
	writeUInt32(len);
	for (int i = 0; i < s.length(); i++)
	{
		writeChar(s[i]);
	}
}

void MemoryStream::writeBuffer(const char* buf, U32 size)
{
	int curpos = this->position;
	this->position += size;
	this->reallocate();
	dMemcpy(&this->buffer[curpos], buf, size);

	if (curpos <= this->properSize && curpos + size >= this->properSize) {
		this->properSize = curpos + size;
	}
}

bool MemoryStream::seek(U32 position)
{
    U32 oldPos = this->position;
	this->position = position;
	bool didEos = checkEos(false);
    if (didEos)
        this->position = oldPos;
    return !didEos;
}

U32 MemoryStream::tell()
{
	return this->position;
}

U32 MemoryStream::length()
{
	return this->properSize;
}

U8* MemoryStream::getBuffer()
{
	return this->buffer;
}

bool MemoryStream::checkEos(bool error)
{
	if (this->position > this->properSize || this->position < 0)
		if (error)
			throw std::runtime_error("End of stream!");
		else
			return true;
	else
		return false;
}