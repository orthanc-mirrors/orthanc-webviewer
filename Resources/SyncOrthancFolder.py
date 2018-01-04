#!/usr/bin/python

#
# This maintenance script updates the content of the "Orthanc" folder
# to match the latest version of the Orthanc source code.
#

import multiprocessing
import os
import stat
import urllib2

TARGET = os.path.join(os.path.dirname(__file__), '..', 'Orthanc')
PLUGIN_SDK_VERSION = '0.9.5'
REPOSITORY = 'https://bitbucket.org/sjodogne/orthanc/raw'

FILES = [
    'Core/ChunkedBuffer.cpp',
    'Core/ChunkedBuffer.h',
    'Core/Enumerations.cpp',
    'Core/Enumerations.h',
    'Core/Endianness.h',
    'Core/DicomFormat/DicomMap.h',
    'Core/DicomFormat/DicomMap.cpp',
    'Core/DicomFormat/DicomTag.h',
    'Core/DicomFormat/DicomTag.cpp',
    'Core/DicomFormat/DicomValue.h',
    'Core/DicomFormat/DicomValue.cpp',
    'Core/FileStorage/FilesystemStorage.cpp',
    'Core/FileStorage/FilesystemStorage.h',
    'Core/FileStorage/IStorageArea.h',
    'Core/IDynamicObject.h',
    'Core/Images/ImageAccessor.cpp',
    'Core/Images/ImageAccessor.h',
    'Core/Images/ImageBuffer.cpp',
    'Core/Images/ImageBuffer.h',
    'Core/Images/ImageProcessing.cpp',
    'Core/Images/ImageProcessing.h',
    'Core/Logging.h',
    'Core/MultiThreading/SharedMessageQueue.cpp',
    'Core/MultiThreading/SharedMessageQueue.h',
    'Core/OrthancException.h',
    'Core/PrecompiledHeaders.cpp',
    'Core/PrecompiledHeaders.h',
    'Core/SQLite/Connection.cpp',
    'Core/SQLite/Connection.h',
    'Core/SQLite/FunctionContext.cpp',
    'Core/SQLite/FunctionContext.h',
    'Core/SQLite/IScalarFunction.h',
    'Core/SQLite/ITransaction.h',
    'Core/SQLite/NonCopyable.h',
    'Core/SQLite/OrthancSQLiteException.h',
    'Core/SQLite/SQLiteTypes.h',
    'Core/SQLite/Statement.cpp',
    'Core/SQLite/Statement.h',
    'Core/SQLite/StatementId.cpp',
    'Core/SQLite/StatementId.h',
    'Core/SQLite/StatementReference.cpp',
    'Core/SQLite/StatementReference.h',
    'Core/SQLite/Transaction.cpp',
    'Core/SQLite/Transaction.h',
    'Core/SystemToolbox.cpp',
    'Core/SystemToolbox.h',
    'Core/Toolbox.cpp',
    'Core/Toolbox.h',
    'Plugins/Samples/Common/ExportedSymbols.list',
    'Plugins/Samples/Common/VersionScript.map',
    'Plugins/Samples/GdcmDecoder/README',
    'Plugins/Samples/GdcmDecoder/GdcmImageDecoder.h',
    'Plugins/Samples/GdcmDecoder/GdcmImageDecoder.cpp',
    'Plugins/Samples/GdcmDecoder/GdcmDecoderCache.h',
    'Plugins/Samples/GdcmDecoder/GdcmDecoderCache.cpp',
    'Plugins/Samples/GdcmDecoder/OrthancImageWrapper.h',
    'Plugins/Samples/GdcmDecoder/OrthancImageWrapper.cpp',
    'Resources/CMake/AutoGeneratedCode.cmake',
    'Resources/CMake/BoostConfiguration.cmake',
    'Resources/CMake/Compiler.cmake',
    'Resources/CMake/DownloadPackage.cmake',
    'Resources/CMake/GoogleTestConfiguration.cmake',
    'Resources/CMake/JsonCppConfiguration.cmake',
    'Resources/CMake/SQLiteConfiguration.cmake',
    'Resources/CMake/UuidConfiguration.cmake',
    'Resources/Patches/boost-1.65.1-linux-standard-base.patch',
    'Resources/EmbedResources.py',
    'Resources/MinGW-W64-Toolchain32.cmake',
    'Resources/MinGW-W64-Toolchain64.cmake',
    'Resources/MinGWToolchain.cmake',
    'Resources/LinuxStandardBaseToolchain.cmake',
    'Resources/ThirdParty/VisualStudio/stdint.h',
    'Resources/ThirdParty/base64/base64.cpp',
    'Resources/ThirdParty/base64/base64.h',
    'Resources/WindowsResources.py',
    'Resources/WindowsResources.rc',
]

SDK = [
    'orthanc/OrthancCPlugin.h',
]   

EXE = [
    'Resources/EmbedResources.py',
    'Resources/WindowsResources.py',
]


def Download(x):
    branch = x[0]
    source = x[1]
    target = os.path.join(TARGET, x[2])
    print target

    try:
        os.makedirs(os.path.dirname(target))
    except:
        pass

    url = '%s/%s/%s' % (REPOSITORY, branch, source)

    with open(target, 'w') as f:
        f.write(urllib2.urlopen(url).read())


commands = []

for f in FILES:
    commands.append([ 'default', f, f ])

for f in SDK:
    commands.append([ 
        'Orthanc-%s' % PLUGIN_SDK_VERSION, 
        'Plugins/Include/%s' % f,
        'Sdk-%s/%s' % (PLUGIN_SDK_VERSION, f) 
    ])


pool = multiprocessing.Pool(10)  # simultaneous downloads
pool.map(Download, commands)


for exe in EXE:
    path = os.path.join(TARGET, exe)
    st = os.stat(path)
    os.chmod(path, st.st_mode | stat.S_IEXEC)

