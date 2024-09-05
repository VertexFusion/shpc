////////////////////////////////////////////////////////////////////////////////////////////////////
// Name:        Main.cpp
// Application: Shape File Compiler
// Purpose:     The application itself
//
// Author:      Uwe Runtemund (2013-today)
// Modified by:
// Created:     31.01.2013
//
// Copyright:   (c) 2013 Jameo Software, Germany. https://jameo.de
//
// Licence:     The MIT License
//              Permission is hereby granted, free of charge, to any person obtaining a copy of this
//              software and associated documentation files (the "Software"), to deal in the
//              Software without restriction, including without limitation the rights to use, copy,
//              modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
//              and to permit persons to whom the Software is furnished to do so, subject to the
//              following conditions:
//
//              The above copyright notice and this permission notice shall be included in all
//              copies or substantial portions of the Software.
//
//              THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//              INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
//              PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//              HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
//              CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
//              OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <vector>

#include "core/Core.h"

const jm::String version = "Jameo SHP-Compiler V 1.3";
const jm::String err = "<ERROR> ";
const jm::String wrn = "<WARNING> ";
const jm::String inf = "<INFO> ";

/*!
 \brief The pointer to the file. Either when reading or writing.
 */
jm::File* file;

/*!
 \brief Status whether the end of the file was reached during reading. If it is set to "true", then
 the end was reached and the reading is finished.
 */
bool endOfFile;

/*!
 \brief Status whether detailed information is displayed.
 */
bool verbose;

/*!
 \brief Name of the input file.
 */
jm::String inputname;

/*!
 \brief Name of the output file.
 */
jm::String outputname;

/*!
 \brief Reference to the Windows encoding charset, because all strings are encoded with it in SHX
 files.
 */
jm::Charset* cs;

/*!
 \brief Helping structure for compiling.
 */
struct Shape
{
	//The shape number.
	uint16 number;

	// Pointer to current byte during generation.
	uint16 position;

	// Number of defbytes in shape.
	uint16 defBytes;

	// The name of the shape.
	jm::String name;

	// The data.
	uint8* buffer;

	Shape()
	{
		number = 0;
		position = 0;
		defBytes = 0;
		buffer = nullptr;
	}

	~Shape()
	{
		if(buffer != nullptr)delete[] buffer;
	}
};

/*!
 \brief String designation about the file type. This is also the string with which the file begins
 on the disk.
 */
jm::String filetype;

/*!
 \brief There are two variants of how the shapes are stored in the file. One is Unicode and the
 other is "normal". With Unicode all shapes are packed together. With "normal" first header data for
 all shapes are written and then the shape descriptions".
 */
bool isUnicode;

/*!
 \brief The shape that is currently being compiled.
 */
Shape* current;

/*!
 \brief List with all shapes.
 */
std::vector<Shape*> shapes;

/*!
 \brief Counts the shape definition lines during read-in
 */
uint32 shapeCount;

/*!
 \brief This method reads a line from the file and returns it. If the end of the file was reached,
 "endOfFile" is set to "true".
 */
jm::String readLine()
{
	jm::String line;

	while(true)
	{
		uint8 c;
		Integer size = file->read(&c, 1);
		if(size != 1)
		{
			endOfFile = true;
			file->close();
			return line;
		}

		if(c != 10 && c != 13)
		{
			line.append(c);
		}
		else return line;
	}

}

/*!
 \brief This method removes comments from a line of the file. A comment starts with a semicolon and
 ends at the end of the line. So everything from the semicolon on is truncated.
 */
jm::String stripComment(const jm::String &line)
{
	jm::String stripped;
	for(uint32 a = 0; a < line.size(); a++)
	{
		jm::Char c = line.charAt(a);

		// Everything after a semicolon is comment
		if(c == ';')return stripped;
		stripped.append(c);
	}
	return stripped;
}

/*!
 \brief This method parses a byte definition
 */
uint8 toSpecByte(jm::String token)
{
	token = token.trim();
	int32 sign = 1;
	if(token.charAt(0) == jm::Char('-'))
	{
		sign = -1;
		token = token.substring(1);
		token = token.trim();
	}

	int32 c=0;
	if(token.charAt(0) == jm::Char('0'))
	{
		c =  Integer::fromHex(token).Uint16();
		if(sign < 0)c |= 0x80;// Note: Yes exactly this operation.
	}
	else
	{
		c = Integer::valueOf(token).Uint16();
		if(sign < 0)c *= -1;
	}
	return c;
}

/*!
 \brief This method parses a byte definition
 */
uint16 toSpecShort(jm::String token)
{
	token = token.trim();

	if(token.charAt(0) == jm::Char('0'))
	{
		return Integer::fromHex(token).Uint16();
	}
	else
	{
		return Integer::valueOf(token).Uint16();
	}
}

/*!
 \brief This method evaluates the header of a shape. Each shape definition starts with it. There can
 be many shapes / characters in one file.
 */
void handleFirstLine(const jm::String &line)
{
	jm::StringTokenizer st = jm::StringTokenizer(line, ",", false);

	jm::String number = st.next();
	jm::String count = st.next();
	jm::String name = st.next();

	if(number.equalsIgnoreCase("*UNIFONT")) number = "0";
	else if(number.startsWith("*"))number = number.substring(1);
	else throw new jm::Exception("* expected.");

	if(verbose)
		std::cout << inf << "Shape: " << number << ", Spec Bytes: " << count << ", Name: " << name << std::endl;

	current = new Shape();

	if(number.startsWith("0"))current->number = Integer::fromHex(number).Uint16();
	else current->number = Integer::valueOf(number).Uint16();

	if(count.startsWith("0"))current->defBytes = Integer::fromHex(count).Uint16();
	else current->defBytes = Integer::valueOf(count).Uint16();

	current->name = name;
	current->buffer = new uint8[current->defBytes];
	current->position = 0;
	shapes.push_back(current);
}

/*!
 \brief This method evaluates a specbyte line. The specbyte lines contain the geometry information
 for a shape. A shape can contain several such lines.
 */
void handleDefinitionLine(const jm::String &line)
{
	if(current == nullptr)throw new jm::Exception("Corrupt file. Shape header not found.");

	jm::StringTokenizer st = jm::StringTokenizer(line, ",()", false);

	while(st.hasNext())
	{
		jm::String token = st.next().trim();

		if(current->position >= current->defBytes)
			throw new jm::Exception("Too many spec bytes in shape: " + current->name);

		if(token.size() > 4)
		{
			uint16 s = toSpecShort(token);
			current->buffer[current->position++] = (uint8)(s >> 8);
			current->buffer[current->position++] = (uint8)(s);
		}
		else current->buffer[current->position++] = toSpecByte(token);
	}
}

/*!
 \brief This method parses a compiled shape or its SpecBytes for errors in content.
 */
void parse(Shape* shape)
{
	int a = 0;
	int stack = 0;
	while(a < shape->defBytes)
	{
		uint8 c1, c2, c3, c4, c5;

		uint8 c = shape->buffer[a];

		switch(c)
		{
			case 0: // End of Shape
				// "End of Shape" may only be the last byte in a shape definition.
				if(a != (shape->defBytes - 1))
					throw new jm::Exception("In shape \""
					                    + shape->name
					                    + "\": End-Of-Shape-Command (0) before end of shape found.");
				break;

			case 1: // Pen Down
				// Here is nothing to do.
				break;

			case 2: // Pen Up
				// Here is nothing to do.
				break;

			case 3:
				if((a + 1) >= (shape->defBytes - 1))
					throw new jm::Exception("In shape \""
					                    + shape->name
					                    + "\": Scale-Down-Command (3) not complete.");
				c1 = shape->buffer[a + 1];
				//! \todo Are there limits to values ???
				a++;
				break;

			case 4:
				if((a + 1) >= (shape->defBytes - 1))
					throw new jm::Exception("In shape \""
					                    + shape->name
					                    + "\": Scale-Up-Command (4) not complete.");
				c1 = shape->buffer[a + 1];
				//! \todo Are there limits to values ???
				a++;
				break;

			case 5:
				stack++;
				if(stack > 4)
					throw new jm::Exception("In shape \"" + shape->name + "\": Too many Push-Commands (5).");
				break;

			case 6:
				stack--;
				if(stack < 0)
					throw new jm::Exception("In shape \"" + shape->name + "\": Too many Pop-Commands (6).");
				break;

			case 7:
				if(isUnicode)
				{
					if((a + 2) >= (shape->defBytes - 1))
						throw new jm::Exception("In shape \""
						                    + shape->name
						                    + "\": Subshape-Command (7) not complete.");
					c1 = shape->buffer[a + 1];
					c2 = shape->buffer[a + 2];
					//! \todo Are there limits to values ???
					a += 2;
				}
				else
				{
					if((a + 1) >= (shape->defBytes - 1))
						throw new jm::Exception("In shape \""
						                    + shape->name
						                    + "\": Subshape-Command (4) not complete.");
					c1 = shape->buffer[a + 1];
					//! \todo Are there limits to values ???
					a++;
				}
				break;

			case 8:
				if((a + 2) >= (shape->defBytes - 1))
					throw new jm::Exception("In shape \""
					                    + shape->name
					                    + "\": Line-To-Command (8) not complete.");
				c1 = shape->buffer[a + 1];
				c2 = shape->buffer[a + 2];
				//! \todo Are there limits to values ???
				a += 2;
				break;

			case 9:
				do
				{
					if((a + 2) >= (shape->defBytes - 1))
						throw new jm::Exception("In shape \""
						                    + shape->name
						                    + "\": Multi-Line-To-Command (9) not complete.");
					c1 = shape->buffer[a + 1];
					c2 = shape->buffer[a + 2];
					a += 2;
					//						if( ( c1 != 0 || c2 != 0 ) ) ;//Wertegrenezn für lineto
				}
				while(c1 != 0 || c2 != 0);
				break;

			case 10:
				if((a + 2) >= (shape->defBytes - 1))
					throw new jm::Exception("In shape \""
					                    + shape->name
					                    + "\": Octant-Arc-Command (10) not complete.");
				c1 = shape->buffer[a + 1];
				c2 = shape->buffer[a + 2];
				//! \todo Are there limits to values ???
				a += 2;
				break;

			case 11:
				if((a + 5) >= (shape->defBytes - 1))
					throw new jm::Exception("In shape \""
					                    + shape->name
					                    + "\": Fractional-Arc-Command (11) not complete.");
				c1 = shape->buffer[a + 1];
				c2 = shape->buffer[a + 2];
				c3 = shape->buffer[a + 3];
				c4 = shape->buffer[a + 4];
				c5 = shape->buffer[a + 5];
				//! \todo Are there limits to values ???
				a += 5;
				break;

			case 12:
				if((a + 3) >= (shape->defBytes - 1))
					throw new jm::Exception("In shape \""
					                    + shape->name
					                    + "\": Arc-To-Command (12) not complete.");
				c1 = shape->buffer[a + 1];
				c2 = shape->buffer[a + 2];
				c3 = shape->buffer[a + 3];
				//! \todo Are there limits to values ???
				a += 3;
				break;

			case 13:
				do
				{
					if((a + 2) >= (shape->defBytes - 1))
						throw new jm::Exception("In shape \""
						                    + shape->name
						                    + "\": Multi-Arc-To-Command (13) not complete.");
					c1 = shape->buffer[a + 1];
					c2 = shape->buffer[a + 2];
					c3 = shape->buffer[a + 3];
					a += 2;
					if((c1 != 0 || c2 != 0))
					{
						if((a + 1) >= (shape->defBytes - 1))
							throw new jm::Exception("In shape \""
							                    + shape->name
							                    + "\": Multi-Arc-To-Command (13) not complete.");
						a++;
					}
				}
				while(c1 != 0 || c2 != 0);
				break;

			case 14:
				if((a + 1) >= (shape->defBytes - 1))
					throw new jm::Exception("In shape \""
					                    + shape->name
					                    + "\": No command after Do-Next-Command (14).");
				break;

			default:
				c1 = c & 0x0F;
				c2 = (c >> 4) & 0x0F;
				// Can only occur for 0x0F
				if(c2 == 0x0)
					throw new jm::Exception("In shape \""
					                    + shape->name
					                    + "\": Vector length is zero (00).");
				break;
		}
		a++;
	}
}

/*!
 \brief This method checks if all names contain only numbers and capital letters.
 */
void checkShapeName(const jm::String &name)
{
	for(uint32 a = 0; a < name.size(); a++)
	{
		jm::Char c = name.charAt(a);

		if((c < '0' || c > '9') && (c < 'A' || c > 'Z'))
		{
			std::cout << wrn
			     << "In shape \""
			     << name
			     << "\": Characters of name should be upper case or numbers." << std::endl;
			return;
		}

	}
}

/*!
 \brief This method checks the shapes for potential errors.
 */
void check()
{
	int last = -1;

	if(shapeCount != shapes.size())throw new jm::Exception("Shape count differs from found shapes.");

	for(uint32 a = 0; a < shapes.size(); a++)
	{
		Shape* shape = shapes[a];
		// Check shape name for "non-fonts"
		if(shapes[0]->number != 0)checkShapeName(shape->name);

		// Check that each shape ends with 0.
		if(shape->buffer[shape->defBytes - 1] != 0)
			throw new jm::Exception("In shape \"" + shape->name + "\": Last spec byte must be 0.");

		// Check if the specified number of bytes matches the actually specified spec bytes.
		if(shape->defBytes != shape->position)
			throw new jm::Exception("In shape \"" + shape->name + "\": Wrong spec byte count.");

		// Check that the shape numbers are ascending (and unique).
		if(shape->number <= last)
			throw new jm::Exception("In shape \""
			                    + shape->name
			                    + "\": Number of shape is lower or equal than in shape before.");
		last = shape->number;

		parse(shape);
	}
}

/*!
 \brief Writes a 16-bit number LE (little endian) encoded
 */
void writeLE16(int16 value)
{
	uint8 c[2];
	c[0] = (uint8)value;
	c[1] = (uint8)(value >> 8);
	file->write(c, 2);
}

/*!
 \brief Writes a 16-bit number BE (big endian) encoded
 */
void writeBE16(int16 value)
{
	uint8 c[2];
	c[0] = (uint8)(value >> 8);
	c[1] = (uint8)value;
	file->write(c, 2);
}

/*!
 \brief This method writes the SHX file in Unicode format.
 */
void writeUnicodeSHX()
{
	if(verbose) std::cout << inf << "Write file in UNICODE file format." << std::endl;
	file = new jm::File(outputname);

	file->open(jm::kFmWrite);

	jm::ByteArray cstring = filetype.toCString(cs);
	file->write((uint8*)cstring.constData(), filetype.size());

	// Write number of shapes
	uint16 size = (uint16)shapes.size();
	writeLE16(size);

	// Write shapes
	for(uint32 a = 0; a < shapes.size(); a++)
	{
		Shape* shape = shapes[a];

		// Shape number
		writeLE16(shape->number);

		// Buffer length
		writeLE16(shape->defBytes + shape->name.size() + 1);

		// Shape name
		cstring = shape->name.toCString(cs);
		file->write((uint8*)cstring.constData(), shape->name.size() + 1);

		// Buffer
		file->write(shape->buffer, shape->defBytes);

	}

	if(verbose) std::cout << inf << shapes.size() << " Shapes compiled." << std::endl;
	if(verbose) std::cout << inf << "Output file created: " << outputname << std::endl;
}


/*!
 \brief This method writes the SHX file in normal format.
 */
void writeNormalSHX()
{
	if(verbose) std::cout << inf << "Write file in NORMAL file format." << std::endl;
	file = new jm::File(outputname);

	file->open(jm::kFmWrite);


	// Write file type
	jm::ByteArray cstring = filetype.toCString(cs);
	file->write((uint8*)cstring.constData(), filetype.size());

	//Write lowest shape number
	uint16 low = shapes[0]->number;
	writeLE16(low);

	// Write highest shape number
	uint16 high = shapes[shapes.size() - 1]->number;
	writeLE16(high);

	// Write number of shapes encoded
	uint16 size = (uint16)shapes.size();
	writeLE16(size);

	// Write shape header
	for(uint32 a = 0; a < shapes.size(); a++)
	{
		Shape* shape = shapes[a];

		// shape number
		writeLE16(shape->number);

		// buffer length
		writeLE16(shape->defBytes + shape->name.size() + 1);
	}

	// Write shape data
	for(uint32 a = 0; a < shapes.size(); a++)
	{
		Shape* shape = shapes[a];

		//Shapename
		cstring = shape->name.toCString(cs);
		file->write((uint8*)cstring.constData(), shape->name.size() + 1);

		//Puffer
		file->write(shape->buffer, shape->defBytes);

	}

	//Write End-Of-File
	file->write((uint8*)"EOF", 3);

	if(verbose) std::cout << inf << shapes.size() << " Shapes compiled: " << outputname << std::endl;
	if(verbose) std::cout << inf << "Output file created: " << outputname << std::endl;
}

/*!
 \brief This method traverses the file and controls the compilation as a whole.
 */
void compile()
{
	shapeCount = 0;

	while(!endOfFile)
	{
		jm::String line = readLine();
		if(line.size() > 128) std::cout << wrn << "Line is longer then 128 Bytes." << std::endl;
		line = stripComment(line);

		if(line.size() > 0)
		{
			if(line.charAt(0) == '*')
			{
				// First line in the SHP file determines what it is.
				//
				if(shapeCount == 0)
				{
					if(line.startsWith("*UNIFONT,"))
					{
						filetype = "AutoCAD-86 unifont 1.0\r\n\x1A";
						isUnicode = true;
					}
					else if(line.startsWith("*0,"))
					{
						filetype = "AutoCAD-86 shapes 1.1\r\n\x1A";
						isUnicode = false;
					}
					else
					{
						filetype = "AutoCAD-86 shapes 1.0\r\n\x1A";
						isUnicode = false;
					}
				}
				handleFirstLine(line);
				shapeCount++;
			}
			else handleDefinitionLine(line);
		}
	}

	check();
	file->close();
	delete file;

	if(isUnicode)writeUnicodeSHX();
	else writeNormalSHX();
}

/*!
\brief Cleans allocated memory
*/
void clean()
{

	// Clean shapes
	for(uint32 a = 0; a < shapes.size(); a++)
	{
		delete shapes[a];
	}

	// Clean file
	file->close();
	delete file;
	file = nullptr;
}

/*!
\brief Main method for program entry.
 */
int main(int argc, const char* argv[])
{
	current = nullptr;
	file = nullptr;
	jm::System::init("shpc");
	cs = jm::Charset::ForName("Windows-1252");
	std::cout << version << std::endl;

	bool printHelp = false;
	verbose = false;

	// Evaluate arguments
	for(int a = 0; a < argc; a++)
	{
		jm::String cmd = argv[a];
		if(cmd.equals("-o"))
		{
			if(a < argc - 1)
			{
				outputname = argv[++a];
			}
			else
			{
				std::cout << err << "No output file after -o" << std::endl;
				jm::System::quit();
				return 1;
			}
		}
		else if(cmd.equals("-v"))
		{
			verbose = true;
		}
		else if(cmd.equals("-h") || cmd.equals("-H"))
		{
			printHelp = true;
		}
		else if(a == (argc - 1))
		{
			inputname = cmd;
		}
	}

	// Check number of arguments
	if(argc < 2 || printHelp)
	{
		std::cout << std::endl;
		std::cout << "usage: shpc [options] *.shp" << std::endl;
		std::cout << "options:" << std::endl;
		std::cout << "-h,-H     : Print help." << std::endl;
		std::cout << "-v        : Print detailed information." << std::endl;
		std::cout << "-o <name> : Name of output file." << std::endl;
		std::cout << std::endl;
		std::cout << "For further help contact jameo.de" << std::endl;
		std::cout << std::endl;
		jm::System::quit();
		return 1;
	};

	// Determine the name of the output file
	if(outputname.size() < 1)
	{
		outputname = inputname;

		if(outputname.toLowerCase().endsWith(".shp"))
			outputname = outputname.substring(0, outputname.size() - 4);

		outputname.append(".shx");
	}

	if(verbose)
	{
		std::cout << inf << "input file: " << inputname << std::endl;
		std::cout << inf << "output file: " << outputname << std::endl;
	}

	if(inputname.size() > 1)
	{

		file = new jm::File(inputname);
		endOfFile = false;

		if(file->exists() == false)
		{
			std::cout << err << "Input file \"" << inputname << "\" does not exist" << std::endl;
			clean();
			jm::System::quit();
			return -1;
		}

		try
		{
			file->open(jm::kFmRead);
			compile();
			clean();
			std::cout << "Done." << std::endl;
		}
		catch(jm::Exception* e)
		{
			std::cout << err << e->GetErrorMessage() << std::endl;
			delete e;
			clean();
			jm::System::quit();
			return -1;
		}
	}
	else
	{
		std::cout << err << "No input file." << std::endl;
	}
	jm::System::quit();
	return 0;
}

