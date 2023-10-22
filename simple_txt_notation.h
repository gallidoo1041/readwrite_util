#pragma once

#include "readwrite_data.h"

#include <map>

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

}