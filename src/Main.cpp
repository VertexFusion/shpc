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

#include "VertexFusion.h"

using std::cout;
using std::endl;
using std::vector;
using namespace jm;

const String version = "Jameo SHP-Compiler V 1.3";
const String err = "<ERROR> ";
const String wrn = "<WARNING> ";
const String inf = "<INFO> ";

/*!
 \brief The pointer to the file. Either when reading or writing.
 */
File* file;

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
String inputname;

/*!
 \brief Name of the output file.
 */
String outputname;

/*!
 \brief Reference to the Windows encoding charset, because all strings are encoded with it in SHX
 files.
 */
Charset* cs;

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
	String name;

	// The data.
	uint8* buffer;

	Shape()
	{
		number = 0;
		position = 0;
		defBytes = 0;
		buffer = NULL;
	}

	~Shape()
	{
		if(buffer != NULL)delete[] buffer;
	}
};

/*!
 \brief String designation about the file type. This is also the string with which the file begins
 on the disk.
 */
String filetype;

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
vector<Shape*> shapes;

/*!
 \brief Counts the shape definition lines during read-in
 */
uint32 shapeCount;

/*!
 \brief This method reads a line from the file and returns it. If the end of the file was reached,
 "endOfFile" is set to "true".
 */
String ReadLine()
{
	String line;

	while(true)
	{
		uint8 c;
		Integer size = file->Read(&c, 1);
		if(size != 1)
		{
			endOfFile = true;
			file->Close();
			return line;
		}

		if(c != 10 && c != 13)
		{
			line.Append(c);
		}
		else return line;
	}

}

/*!
 \brief This method removes comments from a line of the file. A comment starts with a semicolon and
 ends at the end of the line. So everything from the semicolon on is truncated.
 */
String StripComment(const String &line)
{
	String stripped;
	for(uint32 a = 0; a < line.Length(); a++)
	{
		jm::Char c = line.CharAt(a);

		// Everything after a semicolon is comment
		if(c == ';')return stripped;
		stripped.Append(c);
	}
	return stripped;
}

/*!
 \brief This method parses a byte definition
 */
uint8 ToSpecByte(String token)
{
	token = token.Trim();
	int32 sign = 1;
	if(token.CharAt(0) == Char('-'))
	{
		sign = -1;
		token = token.Substring(1);
		token = token.Trim();
	}

	int32 c=0;
	if(token.CharAt(0) == Char('0'))
	{
		c =  Integer::FromHex(token).Uint16();
		if(sign < 0)c *= -1;
	}
	else
	{
		c = Integer::ValueOf(token).Uint16();
		if(sign < 0)c *= -1;
	}
	return c;
}

/*!
 \brief This method parses a byte definition
 */
uint16 ToSpecShort(String token)
{
	token = token.Trim();

	if(token.CharAt(0) == Char('0'))
	{
		return Integer::FromHex(token).Uint16();
	}
	else
	{
		return Integer::ValueOf(token).Uint16();
	}
}

/*!
 \brief This method evaluates the header of a shape. Each shape definition starts with it. There can
 be many shapes / characters in one file.
 */
void HandleFirstLine(const String &line)
{
	StringTokenizer st = StringTokenizer(line, ",", false);

	String number = st.NextToken();
	String count = st.NextToken();
	String name = st.NextToken();

	if(number.EqualsIgnoreCase("*UNIFONT")) number = "0";
	else if(number.StartsWith("*"))number = number.Substring(1);
	else throw new Exception("* expected.");

	if(verbose)
		cout << inf << "Shape: " << number << ", Spec Bytes: " << count << ", Name: " << name << endl;

	current = new Shape();

	if(number.StartsWith("0"))current->number = Integer::FromHex(number).Uint16();
	else current->number = Integer::ValueOf(number).Uint16();

	if(count.StartsWith("0"))current->defBytes = Integer::FromHex(count).Uint16();
	else current->defBytes = Integer::ValueOf(count).Uint16();

	current->name = name;
	current->buffer = new uint8[current->defBytes];
	current->position = 0;
	shapes.push_back(current);
}

/*!
 \brief This method evaluates a specbyte line. The specbyte lines contain the geometry information
 for a shape. A shape can contain several such lines.
 */
void HandleDefinitionLine(const String &line)
{
	if(current == NULL)throw new Exception("Corrupt file. Shape header not found.");

	StringTokenizer st = StringTokenizer(line, ",()", false);

	while(st.HasMoreTokens())
	{
		String token = st.NextToken().Trim();

		if(current->position >= current->defBytes)
			throw new Exception("Too many spec bytes in shape: " + current->name);

		if(token.Length() > 4)
		{
			uint16 s = ToSpecShort(token);
			current->buffer[current->position++] = (uint8)(s >> 8);
			current->buffer[current->position++] = (uint8)(s);
		}
		else current->buffer[current->position++] = ToSpecByte(token);
	}
}

/*!
 \brief This method parses a compiled shape or its SpecBytes for errors in content.
 */
void Parse(Shape* shape)
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
					throw new Exception("In shape \""
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
					throw new Exception("In shape \""
					                    + shape->name
					                    + "\": Scale-Down-Command (3) not complete.");
				c1 = shape->buffer[a + 1];
				//! \todo Are there limits to values ???
				a++;
				break;

			case 4:
				if((a + 1) >= (shape->defBytes - 1))
					throw new Exception("In shape \""
					                    + shape->name
					                    + "\": Scale-Up-Command (4) not complete.");
				c1 = shape->buffer[a + 1];
				//! \todo Are there limits to values ???
				a++;
				break;

			case 5:
				stack++;
				if(stack > 4)
					throw new Exception("In shape \"" + shape->name + "\": Too many Push-Commands (5).");
				break;

			case 6:
				stack--;
				if(stack < 0)
					throw new Exception("In shape \"" + shape->name + "\": Too many Pop-Commands (6).");
				break;

			case 7:
				if(isUnicode)
				{
					if((a + 2) >= (shape->defBytes - 1))
						throw new Exception("In shape \""
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
						throw new Exception("In shape \""
						                    + shape->name
						                    + "\": Subshape-Command (4) not complete.");
					c1 = shape->buffer[a + 1];
					//! \todo Are there limits to values ???
					a++;
				}
				break;

			case 8:
				if((a + 2) >= (shape->defBytes - 1))
					throw new Exception("In shape \""
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
						throw new Exception("In shape \""
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
					throw new Exception("In shape \""
					                    + shape->name
					                    + "\": Octant-Arc-Command (10) not complete.");
				c1 = shape->buffer[a + 1];
				c2 = shape->buffer[a + 2];
				//! \todo Are there limits to values ???
				a += 2;
				break;

			case 11:
				if((a + 5) >= (shape->defBytes - 1))
					throw new Exception("In shape \""
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
					throw new Exception("In shape \""
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
						throw new Exception("In shape \""
						                    + shape->name
						                    + "\": Multi-Arc-To-Command (13) not complete.");
					c1 = shape->buffer[a + 1];
					c2 = shape->buffer[a + 2];
					c3 = shape->buffer[a + 3];
					a += 2;
					if((c1 != 0 || c2 != 0))
					{
						if((a + 1) >= (shape->defBytes - 1))
							throw new Exception("In shape \""
							                    + shape->name
							                    + "\": Multi-Arc-To-Command (13) not complete.");
						a++;
					}
				}
				while(c1 != 0 || c2 != 0);
				break;

			case 14:
				if((a + 1) >= (shape->defBytes - 1))
					throw new Exception("In shape \""
					                    + shape->name
					                    + "\": No command after Do-Next-Command (14).");
				break;

			default:
				c1 = c & 0x0F;
				c2 = (c >> 4) & 0x0F;
				// Can only occur for 0x0F
				if(c2 == 0x0)
					throw new Exception("In shape \""
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
void CheckShapeName(const String &name)
{
	for(uint32 a = 0; a < name.Length(); a++)
	{
		jm::Char c = name.CharAt(a);

		if((c < '0' || c > '9') && (c < 'A' || c > 'Z'))
		{
			cout << wrn
			     << "In shape \""
			     << name
			     << "\": Characters of name should be upper case or numbers." << endl;
			return;
		}

	}
}

/*!
 \brief This method checks the shapes for potential errors.
 */
void Check()
{
	int last = -1;

	if(shapeCount != shapes.size())throw new Exception("Shape count differs from found shapes.");

	for(uint32 a = 0; a < shapes.size(); a++)
	{
		Shape* shape = shapes[a];
		// Check shape name for "non-fonts"
		if(shapes[0]->number != 0)CheckShapeName(shape->name);

		// Check that each shape ends with 0.
		if(shape->buffer[shape->defBytes - 1] != 0)
			throw new Exception("In shape \"" + shape->name + "\": Last spec byte must be 0.");

		// Check if the specified number of bytes matches the actually specified spec bytes.
		if(shape->defBytes != shape->position)
			throw new Exception("In shape \"" + shape->name + "\": Wrong spec byte count.");

		// Check that the shape numbers are ascending (and unique).
		if(shape->number <= last)
			throw new Exception("In shape \""
			                    + shape->name
			                    + "\": Number of shape is lower or equal than in shape before.");
		last = shape->number;

		Parse(shape);
	}
}

/*!
 \brief Writes a 16-bit number LE (little endian) encoded
 */
void WriteLE16(int16 value)
{
	uint8 c[2];
	c[0] = (uint8)value;
	c[1] = (uint8)(value >> 8);
	file->Write(c, 2);
}

/*!
 \brief Writes a 16-bit number BE (big endian) encoded
 */
void WriteBE16(int16 value)
{
	uint8 c[2];
	c[0] = (uint8)(value >> 8);
	c[1] = (uint8)value;
	file->Write(c, 2);
}

/*!
 \brief This method writes the SHX file in Unicode format.
 */
void WriteUnicodeSHX()
{
	if(verbose) cout << inf << "Write file in UNICODE file format." << endl;
	file = new File(outputname);

	file->Open(jm::kFmWrite);

	jm::ByteArray cstring = filetype.ToCString(cs);
	file->Write((uint8*)cstring.ConstData(), filetype.Length());

	// Write number of shapes
	uint16 size = (uint16)shapes.size();
	WriteLE16(size);

	// Write shapes
	for(uint32 a = 0; a < shapes.size(); a++)
	{
		Shape* shape = shapes[a];

		// Shape number
		WriteLE16(shape->number);

		// Buffer length
		WriteLE16(shape->defBytes + shape->name.Length() + 1);

		// Shape name
		cstring = shape->name.ToCString(cs);
		file->Write((uint8*)cstring.ConstData(), shape->name.Length() + 1);

		// Buffer
		file->Write(shape->buffer, shape->defBytes);

	}

	if(verbose) cout << inf << shapes.size() << " Shapes compiled." << endl;
	if(verbose) cout << inf << "Output file created: " << outputname << endl;
}


/*!
 \brief This method writes the SHX file in normal format.
 */
void WriteNormalSHX()
{
	if(verbose) cout << inf << "Write file in NORMAL file format." << endl;
	file = new File(outputname);

	file->Open(jm::kFmWrite);


	// Write file type
	jm::ByteArray cstring = filetype.ToCString(cs);
	file->Write((uint8*)cstring.ConstData(), filetype.Length());

	//Write lowest shape number
	uint16 low = shapes[0]->number;
	WriteLE16(low);

	// Write highest shape number
	uint16 high = shapes[shapes.size() - 1]->number;
	WriteLE16(high);

	// Write number of shapes encoded
	uint16 size = (uint16)shapes.size();
	WriteLE16(size);

	// Write shape header
	for(uint32 a = 0; a < shapes.size(); a++)
	{
		Shape* shape = shapes[a];

		// shape number
		WriteLE16(shape->number);

		// buffer length
		WriteLE16(shape->defBytes + shape->name.Length() + 1);
	}

	// Write shape data
	for(uint32 a = 0; a < shapes.size(); a++)
	{
		Shape* shape = shapes[a];

		//Shapename
		cstring = shape->name.ToCString(cs);
		file->Write((uint8*)cstring.ConstData(), shape->name.Length() + 1);

		//Puffer
		file->Write(shape->buffer, shape->defBytes);

	}

	//Write End-Of-File
	file->Write((uint8*)"EOF", 3);

	if(verbose) cout << inf << shapes.size() << " Shapes compiled: " << outputname << endl;
	if(verbose) cout << inf << "Output file created: " << outputname << endl;
}

/*!
 \brief This method traverses the file and controls the compilation as a whole.
 */
void Compile()
{
	shapeCount = 0;

	while(!endOfFile)
	{
		String line = ReadLine();
		if(line.Length() > 128) cout << wrn << "Line is longer then 128 Bytes." << endl;
		line = StripComment(line);

		if(line.Length() > 0)
		{
			if(line.CharAt(0) == '*')
			{
				// First line in the SHP file determines what it is.
				//
				if(shapeCount == 0)
				{
					if(line.StartsWith("*UNIFONT,"))
					{
						filetype = "AutoCAD-86 unifont 1.0\r\n\x1A";
						isUnicode = true;
					}
					else if(line.StartsWith("*0,"))
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
				HandleFirstLine(line);
				shapeCount++;
			}
			else HandleDefinitionLine(line);
		}
	}

	Check();
	file->Close();
	delete file;

	if(isUnicode)WriteUnicodeSHX();
	else WriteNormalSHX();
}

/*!
\brief Cleans allocated memory
*/
void Clean()
{

	// Clean shapes
	for(uint32 a = 0; a < shapes.size(); a++)
	{
		delete shapes[a];
	}

	// Clean file
	file->Close();
	delete file;
	file = NULL;
}

/*!
\brief Main method for program entry.
 */
int main(int argc, const char* argv[])
{
	current = NULL;
	file = NULL;
	System::Init("shpc");
	cs = jm::Charset::ForName("Windows-1252");
	cout << version << endl;

	bool printHelp = false;
	verbose = false;

	// Evaluate arguments
	for(int a = 0; a < argc; a++)
	{
		String cmd = argv[a];
		if(cmd.Equals("-o"))
		{
			if(a < argc - 1)
			{
				outputname = argv[++a];
			}
			else
			{
				cout << err << "No output file after -o" << endl;
				System::Quit();
				return 1;
			}
		}
		else if(cmd.Equals("-v"))
		{
			verbose = true;
		}
		else if(cmd.Equals("-h") || cmd.Equals("-H"))
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
		cout << endl;
		cout << "usage: shpc [options] *.shp" << endl;
		cout << "options:" << endl;
		cout << "-h,-H     : Print help." << endl;
		cout << "-v        : Print detailed information." << endl;
		cout << "-o <name> : Name of output file." << endl;
		cout << endl;
		cout << "For further help contact jameo.de" << endl;
		cout << endl;
		System::Quit();
		return 1;
	};

	// Determine the name of the output file
	if(outputname.Length() < 1)
	{
		outputname = inputname;

		if(outputname.ToLowerCase().EndsWith(".shp"))
			outputname = outputname.Substring(0, outputname.Length() - 4);

		outputname.Append(".shx");
	}

	if(verbose)
	{
		cout << inf << "input file: " << inputname << endl;
		cout << inf << "output file: " << outputname << endl;
	}

	if(inputname.Length() > 1)
	{

		file = new File(inputname);
		endOfFile = false;

		if(file->Exists() == false)
		{
			cout << err << "Input file \"" << inputname << "\" does not exist" << endl;
			Clean();
			System::Quit();
			return -1;
		}

		try
		{
			file->Open(jm::kFmRead);
			Compile();
			Clean();
			cout << "Done." << endl;
		}
		catch(Exception* e)
		{
			cout << err << e->GetErrorMessage() << endl;
			delete e;
			Clean();
			System::Quit();
			return -1;
		}
	}
	else
	{
		cout << err << "No input file." << endl;
	}
	System::Quit();
	return 0;
}

