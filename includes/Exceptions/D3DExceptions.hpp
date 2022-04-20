#ifndef D3D_EXCEPTIONS_HPP_
#define D3D_EXCEPTIONS_HPP_
#include <Exception.hpp>
#include <vector>

class HrException : public Exception {
public:
	HrException(int line, const char* file, long hr) noexcept;
	HrException(
		int line, const char* file,
		long hr, const std::vector<std::string>& infoMsgs
	) noexcept;

	static std::string TranslateErrorCode(long hr) noexcept;
	[[nodiscard]]
	long GetErrorCode() const noexcept;
	[[nodiscard]]
	std::string GetErrorString() const noexcept;

	void GenerateWhatBuffer() noexcept override;
	[[nodiscard]]
	const char* what() const noexcept override;
	[[nodiscard]]
	const char* GetType() const noexcept override;
	[[nodiscard]]
	std::string GetErrorInfo() const noexcept;

private:
	long m_hr;
	std::string m_info;
};

class DeviceRemovedException final : public HrException {
public:
	using HrException::HrException;

	[[nodiscard]]
	const char* GetType() const noexcept override;
};

class InfoException final : public Exception {
public:
	InfoException(int line, const char* file,
		const std::vector<std::string>& infoMsgs
	) noexcept;

	void GenerateWhatBuffer() noexcept override;

	[[nodiscard]]
	const char* what() const noexcept override;
	[[nodiscard]]
	const char* GetType() const noexcept override;
	[[nodiscard]]
	std::string GetErrorInfo() const noexcept;

private:
	std::string m_info;
};
#endif