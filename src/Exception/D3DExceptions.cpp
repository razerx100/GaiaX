#include <D3DExceptions.hpp>
#include <sstream>
#include <comdef.h>

// HR EXCEPTION
HrException::HrException(int line, const char* file, long hr) noexcept
	: Exception(line, file), m_hr(hr) {
	GenerateWhatBuffer();
}

HrException::HrException(int line, const char* file, long hr,
	const std::vector<std::string>& infoMsgs) noexcept
	: Exception(line, file), m_hr(hr) {
	for (const std::string& m : infoMsgs) {
		m_info += m;
		m_info.append("\n");
	}

	if (!m_info.empty())
		m_info.pop_back();

	GenerateWhatBuffer();
}

void HrException::GenerateWhatBuffer() noexcept {
	std::ostringstream oss;
	oss << GetType() << "\n"
		<< "[Error code] 0x" << std::hex << std::uppercase << GetErrorCode()
		<< std::dec << " (" << static_cast<std::uint64_t>(GetErrorCode()) << ")\n\n"
		<< "[Error String] " << GetErrorString() << "\n";
	if (!m_info.empty())
		oss << "[Error Info]\n" << GetErrorInfo() << "\n\n";
	oss << GetOriginString();
	m_whatBuffer = oss.str();
}

const char* HrException::what() const noexcept {
	return m_whatBuffer.c_str();
}

const char* HrException::GetType() const noexcept {
	return "Graphics Exception";
}

std::string HrException::GetErrorInfo() const noexcept {
	return m_info;
}

std::string HrException::TranslateErrorCode(long hr) noexcept {
	return _com_error(hr).ErrorMessage();
}

long HrException::GetErrorCode() const noexcept {
	return m_hr;
}

std::string HrException::GetErrorString() const noexcept {
	return TranslateErrorCode(m_hr);
}

// DEVICE REMOVED EXCEPTION
const char* DeviceRemovedException::GetType() const noexcept {
	return "Graphics Exception [Device Removed]";
}

// INFO EXCEPTION
InfoException::InfoException(int line, const char* file,
	const std::vector<std::string>& infoMsgs) noexcept
	: Exception(line, file) {

	for (const std::string msg : infoMsgs) {
		m_info += msg;
		m_info.append("\n");
	}

	if (!m_info.empty())
		m_info.pop_back();

	GenerateWhatBuffer();
}

void InfoException::GenerateWhatBuffer() noexcept {
	std::ostringstream oss;
	oss << GetType() << "\n\n";
	if (!m_info.empty())
		oss << "[Error Info]\n" << GetErrorInfo() << "\n\n";
	oss << GetOriginString();
	m_whatBuffer = oss.str();
}

const char* InfoException::what() const noexcept {
	return m_whatBuffer.c_str();
}

const char* InfoException::GetType() const noexcept {
	return "Graphics Info Exception";
}

std::string InfoException::GetErrorInfo() const noexcept {
	return m_info;
}
