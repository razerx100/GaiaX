#ifndef __D3D_EXCEPTIONS_HPP__
#define __D3D_EXCEPTIONS_HPP__
#include <Win32BaseException.hpp>
#include <vector>

class HrException : public Win32BaseException {
public:
	HrException(int line, const char* file, long hr) noexcept;
	HrException(
		int line, const char* file,
		long hr, const std::vector<std::string>& infoMsgs
	) noexcept;

	void GenerateWhatBuffer() noexcept override;
	const char* what() const noexcept override;
	const char* GetType() const noexcept override;
	std::string GetErrorInfo() const noexcept;

private:
	long m_hr;
	std::string m_info;
};

class DeviceRemovedException : public HrException {
public:
	using HrException::HrException;

	const char* GetType() const noexcept override;
};

class InfoException : public Exception {
public:
	InfoException(int line, const char* file,
		const std::vector<std::string>& infoMsgs
	) noexcept;

	void GenerateWhatBuffer() noexcept override;
	const char* what() const noexcept override;
	const char* GetType() const noexcept override;
	std::string GetErrorInfo() const noexcept;

private:
	std::string m_info;
};
#endif