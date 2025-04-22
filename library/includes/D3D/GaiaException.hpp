#ifndef GAIA_EXCEPTION_HPP_
#define GAIA_EXCEPTION_HPP_
#include <exception>
#include <string>

class Exception : public std::exception
{
public:
	Exception(std::string errorType, std::string errorMessage)
		: m_exceptionType{ std::move(errorType) }, m_errorMessage{ std::move(errorMessage) }
	{}

	[[nodiscard]]
	const char* GetType() const noexcept { return std::data(m_exceptionType); }
	[[nodiscard]]
	const char* what() const noexcept override { return std::data(m_errorMessage); }

private:
	std::string m_exceptionType;
	std::string m_errorMessage;
};
#endif
