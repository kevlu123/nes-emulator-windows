#include "EmuFileException.h"

EmuFileException::EmuFileException(const std::string& msg) :
	msg(msg)
{
}

const char* EmuFileException::what() const noexcept
{
	return msg.c_str();
}