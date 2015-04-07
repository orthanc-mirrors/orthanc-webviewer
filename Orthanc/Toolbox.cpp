/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2015 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "PrecompiledHeaders.h"
#include "Toolbox.h"

#include "OrthancException.h"

#include <stdint.h>
#include <string.h>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <algorithm>
#include <ctype.h>

#if defined(_WIN32)
#include <windows.h>
#include <process.h>   // For "_spawnvp()"
#else
#include <unistd.h>    // For "execvp()"
#include <sys/wait.h>  // For "waitpid()"
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include <mach-o/dyld.h> /* _NSGetExecutablePath */
#include <limits.h>      /* PATH_MAX */
#endif

#if defined(__linux) || defined(__FreeBSD_kernel__)
#include <limits.h>      /* PATH_MAX */
#include <signal.h>
#include <unistd.h>
#endif

#if BOOST_HAS_LOCALE != 1
#error Since version 0.7.6, Orthanc entirely relies on boost::locale
#endif

#include <boost/locale.hpp>



namespace Orthanc
{
  void Toolbox::TokenizeString(std::vector<std::string>& result,
                               const std::string& value,
                               char separator)
  {
    result.clear();

    std::string currentItem;

    for (size_t i = 0; i < value.size(); i++)
    {
      if (value[i] == separator)
      {
        result.push_back(currentItem);
        currentItem.clear();
      }
      else
      {
        currentItem.push_back(value[i]);
      }
    }

    result.push_back(currentItem);
  }


  void Toolbox::CreateNewDirectory(const std::string& path)
  {
    if (boost::filesystem::exists(path))
    {
      if (!boost::filesystem::is_directory(path))
      {
        throw OrthancException("Cannot create the directory over an existing file: " + path);
      }
    }
    else
    {
      if (!boost::filesystem::create_directories(path))
      {
        throw OrthancException("Unable to create the directory: " + path);
      }
    }
  }


  bool Toolbox::IsExistingFile(const std::string& path)
  {
    return boost::filesystem::exists(path);
  }


  void Toolbox::ReadFile(std::string& content,
                         const std::string& path) 
  {
    boost::filesystem::ifstream f;
    f.open(path, std::ifstream::in | std::ifstream::binary);
    if (!f.good())
    {
      throw OrthancException(ErrorCode_InexistentFile);
    }

    // http://www.cplusplus.com/reference/iostream/istream/tellg/
    f.seekg(0, std::ios::end);
    std::streamsize size = f.tellg();
    f.seekg(0, std::ios::beg);

    content.resize(size);
    if (size != 0)
    {
      f.read(reinterpret_cast<char*>(&content[0]), size);
    }

    f.close();
  }


  void Toolbox::WriteFile(const std::string& content,
                          const std::string& path)
  {
    boost::filesystem::ofstream f;
    f.open(path, std::ofstream::binary);
    if (!f.good())
    {
      throw OrthancException(ErrorCode_CannotWriteFile);
    }

    if (content.size() != 0)
    {
      f.write(content.c_str(), content.size());
    }

    f.close();
  }



  void Toolbox::RemoveFile(const std::string& path)
  {
    if (boost::filesystem::exists(path))
    {
      if (boost::filesystem::is_regular_file(path))
        boost::filesystem::remove(path);
      else
        throw OrthancException("The path is not a regular file: " + path);
    }
  }



  void Toolbox::USleep(uint64_t microSeconds)
  {
#if defined(_WIN32)
    ::Sleep(static_cast<DWORD>(microSeconds / static_cast<uint64_t>(1000)));
#elif defined(__linux) || defined(__APPLE__) || defined(__FreeBSD_kernel__) || defined(__FreeBSD__)
    usleep(microSeconds);
#else
#error Support your platform here
#endif
  }


  Endianness Toolbox::DetectEndianness()
  {
    // http://sourceforge.net/p/predef/wiki/Endianness/

    uint8_t buffer[4];

    buffer[0] = 0x00;
    buffer[1] = 0x01;
    buffer[2] = 0x02;
    buffer[3] = 0x03;

    switch (*((uint32_t *)buffer)) 
    {
      case 0x00010203: 
        return Endianness_Big;

      case 0x03020100: 
        return Endianness_Little;
        
      default:
        throw OrthancException(ErrorCode_NotImplemented);
    }
  }


  std::string Toolbox::StripSpaces(const std::string& source)
  {
    size_t first = 0;

    while (first < source.length() &&
           isspace(source[first]))
    {
      first++;
    }

    if (first == source.length())
    {
      // String containing only spaces
      return "";
    }

    size_t last = source.length();
    while (last > first &&
           isspace(source[last - 1]))
    {
      last--;
    }          
    
    assert(first <= last);
    return source.substr(first, last - first);
  }
}

