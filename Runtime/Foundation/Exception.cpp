#include "Exception.h"
#include "DebugBreak.h"

#pragma region Exception

Exception::Exception(const std::string &message, const std::source_location &sourceLocation)
: _line(sourceLocation.line()), _file(sourceLocation.file_name())
, _func(sourceLocation.function_name())
{
	_message = message;
	_whatBuffer = fmt::format("[Message]: {}\n[Function]: {}\n[File]: {}\n[Line]: {}\n",
		message, _func, _file, _line
	);
}

const char *Exception::what() const noexcept {
	return _whatBuffer.c_str();
}

int Exception::GetLine() const noexcept {
	return _line;
}

const char *Exception::GetFile() const noexcept {
	return _file;
}

auto Exception::GetFunc() const noexcept -> const char * {
	return _func;
}

const std::string &Exception::GetMessage() const noexcept {
	return _message;
}

void Exception::ThrowException(const Exception &exception) noexcept(false) {
	DEBUG_BREAK;
	throw exception;
}

#pragma endregion

#pragma region NotImplementedException

NotImplementedException::NotImplementedException(const char *func) : _func(func) {

}

const char * NotImplementedException::what() const noexcept {
	if (_whatBuffer.empty())
		_whatBuffer = fmt::format("Function: {} not implemented!", _func);
	return _whatBuffer.c_str();
}

#pragma endregion