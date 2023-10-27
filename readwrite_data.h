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

// Find a sub-sequence in a sequence of objects.
// Type <T> must have operator==.
// Returns:
// - 0 if the sequence is empty.
// - [container_size] if the sequence is not found.
// - The location of the first element of the sequence if the sequence is found. 
template<typename T>
size_t find_sequence(
	const T* container_start,
	size_t container_size,
	const T* seq_start,
	size_t seq_size
) {
	if (seq_size == 0) {
		return 0;
	}

	size_t matched_size = 0;
	size_t found = 0;

	for (size_t current = 0; current < container_size;) {
		if (container_start[current++] == seq_start[matched_size]) {
			matched_size++;
		}
		else {
			matched_size = 0;
			found = current;
			continue;
		}

		if (matched_size == seq_size) {
			break;
		}
	}

	return found;
}

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

// Write log message to an output text file.
// Parameters:
//   [append] If true, open the file and append data
//   [size] (ASCII text only) If -1, write the whole line of text.
//   [start] The starting position in the file to write the data to. If -1, append data at end of file.
// Note:
// This function is useful for writing error logs into a text file. When terminate() or abort() is called, data written to existing file streams may not be written to files. This is a problem because when a fatal error occurs in an application, either function is called. No error information is written, so there's no way to diagnose fatal errors!
void log(
	const char* name,
	const char* message,
	bool overwrite = false
) {
	FILE* f = fopen(name, ((overwrite) ? "w" : "a"));
	if (f) {
		fputs(message, f);
		fclose(f);
	}
}

// Same as log(), but for std::string.
void log(
	const char* name,
	const std::string& message,
	bool overwrite = false
) {
	FILE* f = fopen(name, ((overwrite) ? "w" : "a"));
	if (f) {
		fwrite(message.data(), 1, message.size(), f);
		fclose(f);
	}
}

// Create strings by chaining operators.
struct concat
{
	operator const std::string&() const { return str; }
	operator const char*() const { return str.c_str(); }

	std::exception error() const { return std::exception(str.c_str()); }

	concat(const char* msg = "") { str += msg; }
	concat(const std::string& msg) { str += msg; }

	// Append string to the string.
	concat& operator()(const char* msg)
	{
		str += msg;
		return *this;
	}

	// Append string to the string.
	concat& operator()(const std::string& msg)
	{
		str += msg;
		return *this;
	}

	// Append [count] characters to the string.
	concat& operator()(char c, size_t count = 1)
	{
		str.resize(count + str.size(), c);
		return *this;
	}
	
	// Append a value to the string.
	// <T> must have 'to_string' function that takes [val] as the parameter and returns std::string.
	template<typename T>
	concat& operator()(const T& val)
	{
		using std::to_string;

		str += to_string(val);
		return *this;
	}

	std::string str;
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

	// Find a string in the data, starting from the read position pointer location.
	// Returns:
	// - -1 if [str] is empty, or if the read position pointer value is bigger than the size of the stream.
	// - The size of the stream if [str] is not found.
	// - The location of the first character of [str] if [str] is found.
	size_t find(const std::string& str) const
	{
		return pos + find_sequence(data + pos, (size < pos) ? 0 : size - pos, str.data(), str.size());
	}

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
	
	// Read the data in range [read position pointer; _pos).
	std::string readUntil(size_t _pos)
	{
		if (_pos < pos) {
			return "";
		}

		std::string str(_pos - pos, '\0');

		read(str.data(), str.size());
		return str;
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



namespace stn
{

// Parse simple text (.txt) notation files in memory.
std::map<std::string, std::string> parse(rw::ReadStream rs)
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
			if (line == "[MULTILINE]") {
				line = rs.readUntil(rs.find("\n[END_MULTILINE]\n"));
			}

			attrs[key] = line;
			key.clear();
		}
	}

	return attrs;
}

// Parse simple text (.txt) notation files.
std::map<std::string, std::string> parse(const char* filename)
{
	std::string str = rw::readfile(filename);
	return parse({ str.data(), str.size() });
}

} // |===|   END namespace stn   |===|