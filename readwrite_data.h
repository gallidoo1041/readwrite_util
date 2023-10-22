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
};

} // |===|   END namespace rw   |===|