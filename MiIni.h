/*
Made by Mauricius

Part of my MUtilize repo: https://github.com/LegendaryMauricius/MUtilize
*/

#pragma once
#ifndef _MIINI_H
#define _MIINI_H

#include <string.h>
#include <map>
#include <istream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <filesystem>

namespace MiIniUtils
{
    template <class _IStreamT, class _IFStreamT, class _StringT, class _PathT>
    class readFunc
    {
    public:
        void operator()(const _StringT& filename, std::function<void(_IStreamT&)> streamReader) const
        {
            _IFStreamT file;
            file.open(_PathT(filename), std::ios::in);

            if (file.good()) {
                streamReader(file);
                file.close();
            }
        }
    };

    template <class _OStreamT, class _OFStreamT, class _StringT, class _PathT>
    class writeFunc
    {
    public:
        void operator()(const _StringT& filename, std::function<void(_OStreamT&)> streamWriter) const
        {
            _OFStreamT file;
            file.open(_PathT(filename), std::ios::out);

            if (file.good()) {
                streamWriter(file);
                file.close();
            }
            else {
                std::string reasons = strerror(errno);
                std::string text = ((std::stringstream&)(
                                        std::stringstream() <<
                                            "Can't open ini file \"" << std::string(filename.begin(), filename.end()) <<
                                            "\" because " << reasons << "!"
                                    )).str();
                throw std::runtime_error(text);
            }
        }
    };
}

/*
A simple class for ini files.
The template allows you to specify the type used as a string, as well as the string, input, output and file stream types.
Default is std::string.
*/
template<
    class _StringT = std::string,
    class _SStreamT = std::basic_stringstream<typename _StringT::value_type>,
    class _IStreamT = std::basic_istream<typename _StringT::value_type>,
    class _OStreamT = std::basic_ostream<typename _StringT::value_type>,
    class _PathT = std::filesystem::path,
    class _ReadFunctorT = MiIniUtils::readFunc<_IStreamT, std::basic_ifstream<typename _StringT::value_type>, _StringT, _PathT>,
    class _WriteFunctorT = MiIniUtils::writeFunc<_OStreamT, std::basic_ofstream<typename _StringT::value_type>, _StringT, _PathT>
>
class MiIni
{
public:
	using String		= _StringT;
	using StringStream	= _SStreamT;
	using InputStream	= _IStreamT;
    using OutputStream	= _OStreamT;
    using Path	= _PathT;

private:
	String mFilename;
    bool mAutoSync;
    _ReadFunctorT mReadFunctor;
    _WriteFunctorT mWriteFunctor;

public:

	struct FileError : public std::runtime_error {
		FileError() {}
		FileError(const std::string& what) : std::runtime_error(what) {}
	};

	struct FormatException : public std::runtime_error {
		FormatException() {}
		FormatException(const std::string& what, size_t line) :
			std::runtime_error(what),
			line(line)
		{}

		size_t line;
	};

	std::map<String, std::map<String, String> > dataMap;// dataMap[section][key] = value

	MiIni(): mAutoSync(false) {}

	/*
	@param filename The name of the file to open and read
	@param autosync If enabled, the file will automatically be synced to this MinIni's content before being closed
	*/
	MiIni(String filename, bool autosync) {
        open(filename, autosync);
	}
	
	// Default copy-constructors and assignments are okay

	~MiIni() {
		close();
	}

	/*
	The name of the file that's linked to this MinIni.
	Note that the filename will be linked only if it's specified in the call to open() or in the constructor.
	Using read() to read a file will not link the file.
	*/
	String filename() const {
		return mFilename;
	}

	// If enabled, the linked file will automatically be synced to this MinIni's content before being closed or destroyed
	bool autoSyncEnabled() const {
		return mAutoSync;
	}

	void setFilename(String filename) {
		mFilename = filename;
	}

	void enableAutoSync(bool enable = true) {
		mAutoSync = enable;
	}

	// Returns the String value if it exists. If not, inserts the default value (def) and returns it
	String getStr(String sect, String key, String def = String()) {
		auto& keyvalmap = dataMap[sect];

		auto it = keyvalmap.find(key);
		if (it == keyvalmap.end()) {
			keyvalmap.insert(std::make_pair(key, def));
			return def;
		}
		else {
			return it->second;
		}
	}

	// Sets the value to the String
	void setStr(String sect, String key, String val) {
		dataMap[sect][key] = val;
	}

	// Returns the value if it exists. If not, inserts the default value (def) and returns it. val must be streamable to and from a StringStream.
	template<class _T>
	_T get(String sect, String key, _T def = _T()) {
		StringStream ss;
		auto& keyvalmap = dataMap[sect];

		auto it = keyvalmap.find(key);
		if (it == keyvalmap.end()) {
			ss << def;
			keyvalmap.insert(std::make_pair(key, ss.str()));
			return def;
		}
		else {
			ss << it->second;
			_T ret;
			ss >> ret;
			return ret;
		}
	}

	std::string get(String sect, String key, const char* def = "") {
		return get(sect, key, std::string(def));
	}

    std::string get(String sect, String key, const wchar_t* def = L"") {
		return get(sect, key, std::wstring(def));
	}

	// Sets the value to val. val must be streamable to a StringStream.
	template<class _T>
	void set(String sect, String key, _T val) {
		StringStream ss;
		ss << val;
		setStr(sect, key, ss.str());
	}

	// Returns whether a section exists
	bool exists(String sect) const {
		return (dataMap.find(sect) != dataMap.end());
	}

	// Returns whether a value exists with the key in the section sect
	bool exists(String sect, String key) const {
		auto it = dataMap.find(sect);
		return (it != dataMap.end() && it->second.find(key) != it->second.end());
	}

	/*
	Reads the content from the stream, adding it to the already existing content.
	The stream needs to be formatted as an ini file.If not, a FormatException will be thrown, unless ignoreErrors argument is enabled.
	In that case the improper line will be skipped, and the rest of the file will be read normally.
	Note that specifying a FileStream as the input stream won't link the file, i.e. the filename won't be changed.
	*/
	void readMore(InputStream& is, bool ignoreErrors = false) {
		String sect;
		String ln;
		size_t line = 0;
		while (std::getline(is, ln)) {
			line++;
			size_t commentPos;

			StringStream spacesSS;
			spacesSS << " \t\r\n";
			String spaces = spacesSS.str();

			ln.erase(0, ln.find_first_not_of(spaces));
			ln.erase(ln.find_last_not_of(spaces) + 1);
			if ((commentPos = ln.find_first_of('#')) != String::npos) {
				ln.erase(commentPos);
			}

			if (ln.length()) {
				if (ln[0] == '[') {
					sect = ln.substr(1, ln.find_first_of(']') - 1);
					sect.erase(0, sect.find_first_not_of(spaces));
					sect.erase(sect.find_last_not_of(spaces) + 1);
					dataMap[sect];
				}
				else {
					size_t eqpos = ln.find_first_of('=');
					if (eqpos == String::npos) {
						if (!ignoreErrors)
							throw FormatException(((std::stringstream&)(std::stringstream() <<
								"Wrong ini file format at line " << line << "!"
								)).str(), line);
						continue;
					}

					String key = ln.substr(0, eqpos);
					key.erase(key.find_last_not_of(spaces) + 1);
					String val = ln.substr(eqpos + 1, String::npos);
					val.erase(0, val.find_first_not_of(spaces));

					dataMap[sect][key] = val;
				}
			}
		}
	}

	// Clears the content and reads it from the stream using readMore().
	void read(InputStream& is, bool ignoreErrors = false) {
		dataMap.clear();
		readMore(is, ignoreErrors);
	}

	// Writes the content to the output stream, formatted as an ini file
	void write(OutputStream& os) const {
		for (auto& sect : dataMap) {
			if (sect.first != String()) {
				os << "[" << sect.first << "]\n";
			}
			for (auto& keyval : sect.second) {
				os << keyval.first << " = " << keyval.second << std::endl;
			}
			os << std::endl;
		}
	}

	/*
	Reads the file and links it to this MinIni
	@param filename The name of the file to open
	@param autosync If enabled, the file will automatically be synced to this MinIni's content before being closed
	*/
	void open(String filename, bool autosync, bool ignoreErrors = false) {
		mFilename = filename;
		mAutoSync = autosync;

        mReadFunctor(
                    mFilename, [this, ignoreErrors](InputStream &ss) {
                        this->read(ss, ignoreErrors);
                    }
        );
	}

	// Writes the content to the linked file. Throws FileError if the file can't be opened or isn't linked.
	void sync() const {
        if (mFilename.length())
        {
            mWriteFunctor(
                        mFilename, [this](OutputStream &ss) {
                            this->write(ss);
                        }
            );
        }
        else
        {
            throw std::runtime_error("No linked file specified to be synced to this MinIni!");
        }
	}

	// Resets the MinIni and syncs the linked file before closing if autoSync is enabled.
	void close() {
		if (mFilename.length() && mAutoSync) {
			sync();
		}
		dataMap.clear();
		mFilename = String();
		mAutoSync = false;
	}
};

// ascii version of the default MinIni template
using AMiIni = MiIni<>;

// widechar version of the default MinIni template
using WMiIni = MiIni<std::wstring>;

#endif
