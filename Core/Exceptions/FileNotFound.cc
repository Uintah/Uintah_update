/*
 *  FileNotFound.h: Generic exception for internal errors
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   July 1999
 *
 *  Copyright (C) 1999 SCI Group
 */

#include <Core/Exceptions/FileNotFound.h>

namespace SCIRun {

FileNotFound::FileNotFound(const std::string& message)
    : d_message(message)
{
}

FileNotFound::FileNotFound(const FileNotFound& copy)
    : d_message(copy.d_message)
{
}

FileNotFound::~FileNotFound()
{
}

const char* FileNotFound::message() const
{
    return d_message.c_str();
}

const char* FileNotFound::type() const
{
    return "FileNotFound";
}

} // End namespace SCIRun
