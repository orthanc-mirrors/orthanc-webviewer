#!/usr/bin/python

#
# This maintenance script updates the content of the "Orthanc" folder
# to match the latest version of the Orthanc source code.
#

import os
import shutil

SOURCE = '/home/jodogne/Subversion/Orthanc/Core'
TARGET = os.path.join(os.path.dirname(__file__), '..', 'Orthanc')

FILES = [
    'ChunkedBuffer.cpp',
    'ChunkedBuffer.h',
    'Enumerations.cpp',
    'Enumerations.h',
    'FileStorage/FilesystemStorage.cpp',
    'FileStorage/FilesystemStorage.h',
    'FileStorage/IStorageArea.h',
    'IDynamicObject.h',
    'ImageFormats/ImageAccessor.cpp',
    'ImageFormats/ImageAccessor.h',
    'ImageFormats/ImageBuffer.cpp',
    'ImageFormats/ImageBuffer.h',
    'ImageFormats/ImageProcessing.cpp',
    'ImageFormats/ImageProcessing.h',
    'ImageFormats/PngWriter.cpp',
    'ImageFormats/PngWriter.h',
    'MultiThreading/SharedMessageQueue.cpp',
    'MultiThreading/SharedMessageQueue.h',
    'OrthancException.cpp',
    'OrthancException.h',
    'PrecompiledHeaders.cpp',
    'PrecompiledHeaders.h',
    'SQLite/Connection.cpp',
    'SQLite/Connection.h',
    'SQLite/FunctionContext.cpp',
    'SQLite/FunctionContext.h',
    'SQLite/IScalarFunction.h',
    'SQLite/ITransaction.h',
    'SQLite/NonCopyable.h',
    'SQLite/OrthancSQLiteException.h',
    'SQLite/Statement.cpp',
    'SQLite/Statement.h',
    'SQLite/StatementId.cpp',
    'SQLite/StatementId.h',
    'SQLite/StatementReference.cpp',
    'SQLite/StatementReference.h',
    'SQLite/Transaction.cpp',
    'SQLite/Transaction.h',
    'Toolbox.cpp',
    'Toolbox.h',
    'Uuid.cpp',
    'Uuid.h',
]

for f in FILES:
    source = os.path.join(SOURCE, f)
    target = os.path.join(TARGET, f)
    try:
        os.makedirs(os.path.dirname(target))
    except:
        pass

    shutil.copy(source, target)
