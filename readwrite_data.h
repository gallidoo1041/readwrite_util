#pragma once

/* This file is placed into the public domain. Use at your own risk. */

// Uitlities to read from and write data into memory like a file, and other stuff

#include <vector>
#include <string>
#include <functional>
#include <map>

// Visual Studio does not like fopen.
#if defined(_MSC_VER)
#pragma warning (disable : 4996)
#endif

namespace rw
{

// Read file as a string and return it.
// Parameters:
//   [name] The name of the file.
//   [openmode_binary] If true, open file in binary mode, otherwise text mode.
//   [start] The starting position in the file to read from.
//   [readsize] If -1, read whole file from [start] (Note: be careful when trying to read very large files, reading them as a whole is not recommended).
std::string readfile(
	const char* name,
	bool openmode_binary = 0,
	long start = 0,
	size_t readsize = -1
) {
	std::string str;
	FILE* f = fopen(name, ((openmode_binary) ? "rb" : "r"));

	if (f) {
		if (readsize == -1) {
			fseek(f, 0, SEEK_END);
			str.resize(ftell(f) - start);
		}
		else {
			str.resize(readsize);
		}
		fseek(f, start, SEEK_SET);
		fread(str.data(), 1, str.size(), f);
		fclose(f);
	}

	return str;
}

// A log file structure for error logging.
// Each write operation reopens the same file, allowing instant update of the file.
// Note:
// This structure is useful for writing error logs into a text file. When terminate() or abort() is called, data written to existing file streams may not be written to files. This is a problem because when a fatal error occurs in an application, either function is called. No error information is written, so there's no way to diagnose fatal errors!
struct LogFile
{
	// Creates the log file.
	// Note: The log file should not be used if it fails to be opened or created (for some particular reason).
	LogFile(const char* filename, bool overwrite = false) :
		filename(filename)
	{
		f = fopen(filename, ((overwrite) ? "w" : "a"));
	}

	// Check if the log file is valid. If not valid, the application itself should reopen the log file.
	operator bool() const { return f; }

	// Write a double-precision floating point to the log file.
	LogFile& f64(double val)
	{
		fprintf(f, "%f", val);
		f = freopen(filename, "a", f); // Self-reopen a.k.a update file immediately.
		return *this;
	}

	// Write a 64-bit signed integer to the log file.
	LogFile& s64(int64_t val)
	{
		fprintf(f, "%lld", val);
		f = freopen(filename, "a", f);
		return *this;
	}

	// Write a 64-bit unsigned integer to the log file.
	LogFile& u64(uint64_t val)
	{
		fprintf(f, "%llu", val);
		f = freopen(filename, "a", f);
		return *this;
	}

	// Write string to the log file.
	LogFile& operator()(const char* str)
	{
		fputs(str, f);
		f = freopen(filename, "a", f);
		return *this;
	}

	~LogFile() { fclose(f); }

private:
	FILE* f;
	const char* filename;
};

// A read-only stream of data with a position pointer, data pointer and size.
// Read operations move the position pointer forward.
// This is used to read embedded data without extra copying costs.
struct ReadStream
{
	const char* data;
	size_t size;
	size_t pos = 0; // The current read position.

	ReadStream(const char* data, size_t size) : data(data), size(size) {}

	// Check if the read position pointer is not at or past the end of the stream.
	operator bool() const { return pos < size; }

	// Read a character, returns 0 if the read position is out of bounds.
	char getchr() { return (pos < size) ? data[pos++] : 0; }

	// Read a value.
	template<typename T>
	T read()
	{
		T t{};
		read(&t, sizeof T);
		pos += sizeof T;
		return t;
	}

	// Read bytes into destination, returns the number of bytes read.
	size_t read(void* destination, size_t readsize)
	{
		char* dest = (char*)destination;
		size_t read = pos;

		for (size_t i = 0; i < readsize && pos < size; i++, pos++)
			dest[i] = data[pos];

		return pos - read;
	}

	// Read the data all the way until [rule] is not met.
	// Note: The position pointer will be incremented, regardless if [rule] is met.
	std::string readWhile(const std::function<bool(char)>& rule)
	{
		size_t begin = pos;
		size_t chars = 0;

		while (pos < size) {
			if (!rule(data[pos++])) {
				break;
			}
			else {
				chars++;
			}
		}

		return std::string(&data[begin], chars);
	}
};

// Write stream containing a buffer that can be read.
// Write operations move the position index forward.
struct WriteStream : public std::vector<uint8_t>
{
	size_t pos = 0; // Byte position index.
	size_t bit_pos = 0; // Bit position index.
	bool writebit_reversed = false; // If true, writebit() writes bits in big-endian order in little-endian systems and vice versa.

	void expand(size_t num_bytes) { resize(size() + num_bytes); }

	// Write padding bytes to the buffer.
	void pad(size_t num_bytes) { expand(num_bytes); pos = size(); }

	// Write a byte to the buffer.
	void put(uint8_t b)
	{
		if (pos + 1 > size())
			expand(1);

		operator[](pos) = b;
		pos++;
	}

	// Write a value to the buffer in binary form.
	template<typename T>
	void write(const T& value) { write((uint8_t*)&value, sizeof(T)); }

	// Write [writesize] bytes from source to the buffer.
	void write(const void* source, size_t writesize)
	{
		const uint8_t* src = (uint8_t*)source;

		if (writesize + pos > size())
			resize(writesize + pos);

		for (size_t i = 0; i < writesize; i++, pos++)
			operator[](pos) = src[i];
	}

	// Write a bit to the buffer. [bit_pos] is moved forward.
	// If the current buffer is smaller than the byte position of [bit_pos], the buffer is expanded and [pos] is moved to the end of the buffer.
	void writebit(bool bit)
	{
		size_t byte_pos = bit_pos / 8;
		size_t byte_bit_pos = bit_pos - byte_pos * 8;

		if (byte_pos >= size()) {
			resize(byte_pos + 1);
			pos = size();
		}

		if (writebit_reversed)
			byte_bit_pos = 7 - byte_bit_pos;

		operator[](byte_pos) = (operator[](byte_pos) & ~(1 << byte_bit_pos)) | (bit << byte_bit_pos);
		bit_pos++;
	}
};



// 0----[|+=============================+]>

// Simple Text Notation format.
//
// Reason:
// [*] Creating a quick and very simple text notation format that can easily be understood.
// [*] JSON specification is too complex. In fact, the most popular JSON parser for C++ (https://github.com/nlohmann/json), which is a single header file that can be #include'd (https://github.com/nlohmann/json/blob/develop/single_include/nlohmann/json.hpp), has almost 25k lines of code! That is a little too much for getting some arbitrary values out of a text file, isn't it?
// [*] JSON does not allow comments. Sometimes, comments are helpful for informing the user of what an application will do based on the values of attributes of the file when the application parses it.
// 
//----|] Specification:
// [*] Structure:
// The format is a plain text (.txt) file in ASCII encoding, comprised of attribute-value pairs as follows:
// 
//---------------|BEGIN
/*
attribute
attribute_value

attribute2
attribute_value2

attribute3
attribute_value3

attribute4
attribute_value4

...
*/
//---------------|END
// 
// Attribute names and values are interpreted as strings, and any delimiter or indentation in front of them are treated as part of them.
// Attributes and their values are separated by 1 newline character.
// If an attribute value is an empty line, it will be treated as a null value:
// 
//---------------|BEGIN
/*
attribute

^The line above is an empty line, therefore the attribute value is a null value.
*/
//---------------|END
//
// Attributes can have duplicate names; in this case, the value of the most recent occurence of that duplicate is the value of that attribute:
// 
//---------------|BEGIN
/*
attribute1
hello

attribute1
world
^ 'world' is the value of the attribute 'attribute1', not 'hello'
*/
//---------------|END
// 
// Attribute-value pairs are separated by any number of newline characters:
// 
//---------------|BEGIN
/*
attribute1
blablabla







attribute2
hehehe
*/
//---------------|END
// 
// [*] Comments:
// The format supports single line comments, with the following condition:
// Single-line comments start with a hash (#) character and are allowed before an attribute name:
// 
//---------------|BEGIN
/*
# This is a comment.
attribute
attribute_value

attribute2
# This is an attribute value, not a comment.

		# This is also not a comment, but an attribute.
		# Trust me!
*/
//---------------|END
//
//----|] End of Simple Text Notation specification.

// 0----[|+=============================+]>



// Parse simple text (.txt) notation files in memory.
std::map<std::string, std::string> parseSimpleTextNotation(ReadStream rs)
{
	std::map<std::string, std::string> attrs;

	std::string key;

	while (rs) {
		std::string line = rs.readWhile([](char c) { return c != '\n'; });

		if (line.empty() || line[0] == '#') {
			if (!key.empty()) {
				attrs[key] = line;
				key.clear();
			}
			continue;
		}

		if (key.empty()) {
			key = line;
		}
		else {
			attrs[key] = line;
			key.clear();
		}
	}

	return attrs;
}

// Parse simple text (.txt) notation files.
std::map<std::string, std::string> parseSimpleTextNotation(const char* filename)
{
	std::string str = readfile(filename);
	return parseSimpleTextNotation({ str.data(), str.size() });
}

} // |===|   END namespace rw   |===|