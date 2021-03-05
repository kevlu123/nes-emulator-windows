#pragma once
#include <exception>
#include <string>

class EmuFileException : std::exception
{
public:
	EmuFileException(const std::string& msg);
	virtual const char* what() const noexcept override;
private:
	std::string msg;
};
