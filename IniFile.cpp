/************************************************************************************//**
 * @file IniFile.cpp
 * @brief Implementation of the CIniFile class.
 * @author Adam Clauss
 *//*
 * Copyright (c) Adam Clauss
 * Email: pandcc3@comwerx.net
 * You may use this class/code as you wish in your programs. Feel free to
 * distribute it, and email suggested changes to me.
 ****************************************************************************************/

#include "CFileManager.h"
#include "IniFile.h"
#include "RGFConstants.h"
#include "Utilities.h"
#include <iostream>
#include <sstream>
#include <fstream>


/* ------------------------------------------------------------------------------------ */
// Construction/Destruction
// default constructor
/* ------------------------------------------------------------------------------------ */
CIniFile::CIniFile() :
	numkey(0),
	entrykey(0),
	keyindex(0)
{
}

/* ------------------------------------------------------------------------------------ */
// constructor, can specify pathname here instead of using SetPath later
/* ------------------------------------------------------------------------------------ */
CIniFile::CIniFile(const std::string& inipath) :
	path(inipath),
	numkey(0),
	entrykey(0),
	keyindex(0)
{
}

/* ------------------------------------------------------------------------------------ */
// default destructor
/* ------------------------------------------------------------------------------------ */
CIniFile::~CIniFile()
{
}

/* ------------------------------------------------------------------------------------ */
// Public Functions
/* ------------------------------------------------------------------------------------ */


/* ------------------------------------------------------------------------------------ */
// sets path of ini file to read and write from
/* ------------------------------------------------------------------------------------ */
void CIniFile::SetPath(const std::string& newpath)
{
	path = newpath;
}


/* ------------------------------------------------------------------------------------ */
// reads ini file specified using CIniFile::SetPath()
// returns true if successful, false otherwise
/* ------------------------------------------------------------------------------------ */
bool CIniFile::ReadFile()
{
	geVFile *MainFS;

	if(!CFileManager::GetSingletonPtr()->OpenRFFile(&MainFS, kInstallFile, path.c_str(), GE_VFILE_OPEN_READONLY))
		return false;

	std::string keyname;
	char szInputLine[132];

	while(geVFile_GetS(MainFS, szInputLine, 132) == GE_TRUE)
	{
		if(strlen(szInputLine) <= 1)
			continue;

		std::string readinfo(szInputLine);

		TrimRight(readinfo);

		if(!readinfo.empty())
		{
			// if a section heading
			if(readinfo[0] == '[' && readinfo[readinfo.length()-1] == ']')
			{
				keyname = readinfo;
				TrimLeft(keyname, "[");
				TrimRight(keyname, "]");
			}
			else
			{
				// if not a comment line
				if(readinfo[0] != ';')
				{
					std::string valuename, value;

					std::size_t found = readinfo.find("=");
					if(found != std::string::npos)
					{
						valuename = readinfo.substr(0, found);
						if(readinfo.size() > valuename.size() + 1)
							value = readinfo.substr(found + 1);
					}
					else
					{
						valuename = readinfo;
					}
					TrimLeft(valuename);
					TrimRight(valuename);
					TrimLeft(value);

					SetValue(keyname, valuename, value);
				}
			}
		}
	}

	geVFile_Close(MainFS);

	return true;
}

/* ------------------------------------------------------------------------------------ */
// writes data stored in class to ini file
/* ------------------------------------------------------------------------------------ */
void CIniFile::WriteFile() const
{
	std::ofstream inifile;
	inifile.open(path.c_str());

	for(unsigned int keynum=0; keynum<names.size(); ++keynum)
	{
		if(keys[keynum].names.size() != 0)
		{
			inifile << '[' << names[keynum] << ']' << std::endl;

			for(unsigned int valuenum=0; valuenum<keys[keynum].names.size(); ++valuenum)
			{
				inifile << keys[keynum].names[valuenum] << "=" << keys[keynum].values[valuenum] << std::endl;
			}

			inifile << std::endl;
		}
	}

	inifile.close();
}

/* ------------------------------------------------------------------------------------ */
// deletes all stored ini data
/* ------------------------------------------------------------------------------------ */
void CIniFile::Reset()
{
	keys.resize(0);
	names.resize(0);
}

/* ------------------------------------------------------------------------------------ */
// returns number of keys currently in the ini
/* ------------------------------------------------------------------------------------ */
int CIniFile::GetNumKeys() const
{
	return keys.size();
}

/* ------------------------------------------------------------------------------------ */
// returns number of values stored for specified key, or -1 if key found
/* ------------------------------------------------------------------------------------ */
int CIniFile::GetNumValues(const std::string& keyname) const
{
	int keynum = FindKey(keyname);

	if(keynum == -1)
		return -1;
	else
		return keys[keynum].names.size();
}

/* ------------------------------------------------------------------------------------ */
// gets value of [keyname] valuename =
// overloaded to return std::string, int, double, and bool
/* ------------------------------------------------------------------------------------ */
std::string CIniFile::GetValue(const std::string& keyname, const std::string& valuename)
{
	int keynum = FindKey(keyname);

	if(keynum == -1)
	{
		error = "Unable to locate specified key.";
		return "";
	}

	int valuenum = FindValue(keynum, valuename);

	if(valuenum == -1)
	{
		error = "Unable to locate specified value.";
		return "";
	}

	return keys[keynum].values[valuenum];
}

/* ------------------------------------------------------------------------------------ */
// gets value of [keyname] valuename =
// overloaded to return std::string, int, double, and bool
/* ------------------------------------------------------------------------------------ */
int CIniFile::GetValueI(const std::string& keyname, const std::string& valuename)
{
	return atoi(GetValue(keyname, valuename).c_str());
}

/* ------------------------------------------------------------------------------------ */
// gets value of [keyname] valuename =
// overloaded to return std::string, int, double, and bool
/* ------------------------------------------------------------------------------------ */
double CIniFile::GetValueF(const std::string& keyname, const std::string& valuename)
{
	return atof(GetValue(keyname, valuename).c_str());
}

/* ------------------------------------------------------------------------------------ */
// gets value of [keyname] valuename =
// overloaded to return std::string, int, double, and bool
/* ------------------------------------------------------------------------------------ */
bool CIniFile::GetValueB(const std::string& keyname, const std::string& valuename)
{
	std::string bval(GetValue(keyname, valuename));
	if(bval.empty() || bval == "false" || bval == "False" || bval == "FALSE" || bval == "0")
		return false;

	return true;
}

/* ------------------------------------------------------------------------------------ */
// sets value of [keyname] valuename =.
// specify the optional paramter as false (0) if you do not want it to create
// the key if it doesn't exist. Returns true if data entered, false otherwise
// overloaded to accept std::string, int, double, and bool
/* ------------------------------------------------------------------------------------ */
bool CIniFile::SetValue(const std::string& keyname, const std::string& valuename, const std::string& value, bool create)
{
	int keynum = FindKey(keyname), valuenum = 0;

	// find key
	if(keynum == -1)	// if key doesn't exist
	{
		if(!create)		// and user does not want to create it,
			return 0;	// stop entering this key

		names.resize(names.size() + 1);
		keys.resize(keys.size() + 1);
		keynum = names.size()-1;
		names[keynum] = keyname;
	}

	// find value
	valuenum = FindValue(keynum, valuename);
	if(valuenum == -1)
	{
		if(!create)
			return 0;

		keys[keynum].names.resize(keys[keynum].names.size() + 1);
		keys[keynum].values.resize(keys[keynum].names.size() + 1);
		valuenum = keys[keynum].names.size() - 1;
		keys[keynum].names[valuenum] = valuename;
	}

	keys[keynum].values[valuenum] = value;
	return 1;
}

/* ------------------------------------------------------------------------------------ */
// sets value of [keyname] valuename =.
// specify the optional paramter as false (0) if you do not want it to create
// the key if it doesn't exist. Returns true if data entered, false otherwise
// overloaded to accept std::string, int, double, and bool
/* ------------------------------------------------------------------------------------ */
bool CIniFile::SetValueI(const std::string& keyname, const std::string& valuename, int value, bool create)
{
	std::ostringstream oss;
	oss << value;

	return SetValue(keyname, valuename, oss.str(), create);
}

/* ------------------------------------------------------------------------------------ */
// sets value of [keyname] valuename =.
// specify the optional paramter as false (0) if you do not want it to create
// the key if it doesn't exist. Returns true if data entered, false otherwise
// overloaded to accept std::string, int, double, and bool
/* ------------------------------------------------------------------------------------ */
bool CIniFile::SetValueF(const std::string& keyname, const std::string& valuename, double value, bool create)
{
	std::ostringstream oss;
	oss << value;

	return SetValue(keyname, valuename, oss.str(), create);
}

/* ------------------------------------------------------------------------------------ */
// sets value of [keyname] valuename =.
// specify the optional paramter as false (0) if you do not want it to create
// the key if it doesn't exist. Returns true if data entered, false otherwise
// overloaded to accept std::string, int, double, and bool
/* ------------------------------------------------------------------------------------ */
bool CIniFile::SetValueB(const std::string& keyname, const std::string& valuename, bool value, bool create)
{
	std::ostringstream oss;
	oss << std::boolalpha << value;

	return SetValue(keyname, valuename, oss.str(), create);
}

/* ------------------------------------------------------------------------------------ */
// deletes specified value
// returns true if value existed and deleted, false otherwise
/* ------------------------------------------------------------------------------------ */
bool CIniFile::DeleteValue(const std::string& keyname, const std::string& valuename)
{
	int keynum = FindKey(keyname), valuenum = FindValue(keynum, valuename);

	if(keynum == -1 || valuenum == -1)
		return 0;

	keys[keynum].names.erase(keys[keynum].names.begin() + valuenum);
	keys[keynum].values.erase(keys[keynum].values.begin() + valuenum);
	return 1;
}

/* ------------------------------------------------------------------------------------ */
// deletes specified key and all values contained within
// returns true if key existed and deleted, false otherwise
/* ------------------------------------------------------------------------------------ */
bool CIniFile::DeleteKey(const std::string& keyname)
{
	int keynum = FindKey(keyname);

	if(keynum == -1)
		return 0;

	keys.erase(keys.begin() + keynum);
	names.erase(names.begin() + keynum);
	return 1;
}

/* ------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------ */
std::string CIniFile::FindFirstKey()
{
	numkey = 0;

	if(numkey < keys.size())
		return names[numkey];
	else
		return "";
}

/* ------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------ */
std::string CIniFile::FindNextKey()
{
	++numkey;

	if(numkey < keys.size())
		return names[numkey];
	else
		return "";
}

/* ------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------ */
std::string CIniFile::FindFirstName(const std::string& keyname)
{
	entrykey = 0;
	keyindex = FindKey(keyname);

	if(keyindex == -1)
		return "";

	if(entrykey < keys[keyindex].names.size())
		return keys[keyindex].names[entrykey];

	return "";
}

/* ------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------ */
std::string CIniFile::FindFirstValue() const
{
	if(keyindex == -1)
		return "";

	if(entrykey < keys[keyindex].values.size())
		return keys[keyindex].values[entrykey];

	return "";
}

/* ------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------ */
std::string CIniFile::FindNextName()
{
	if(keyindex == -1)
		return "";

	++entrykey;

	if(entrykey < keys[keyindex].names.size())
		return keys[keyindex].names[entrykey];

	return "";
}

/* ------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------ */
std::string CIniFile::FindNextValue() const
{
	if(keyindex == -1)
		return "";

	if(entrykey < keys[keyindex].values.size())
		return keys[keyindex].values[entrykey];

	return "";
}


/* ------------------------------------------------------------------------------------ */
// Private Functions
/* ------------------------------------------------------------------------------------ */


/* ------------------------------------------------------------------------------------ */
// returns index of specified key, or -1 if not found
/* ------------------------------------------------------------------------------------ */
int CIniFile::FindKey(const std::string& keyname) const
{
	unsigned int keynum = 0;

	while(keynum < keys.size() && names[keynum] != keyname)
		++keynum;

	if(keynum == keys.size())
		return -1;

	return keynum;
}

/* ------------------------------------------------------------------------------------ */
// returns index of specified value, in the specified key, or -1 if not found
/* ------------------------------------------------------------------------------------ */
int CIniFile::FindValue(int keynum, const std::string& valuename) const
{
	if(keynum == -1)
		return -1;

	unsigned int valuenum = 0;

	while(valuenum < keys[keynum].names.size() && keys[keynum].names[valuenum] != valuename)
		++valuenum;

	if(valuenum == keys[keynum].names.size())
		return -1;

	return valuenum;
}

/* ----------------------------------- END OF FILE ------------------------------------ */
