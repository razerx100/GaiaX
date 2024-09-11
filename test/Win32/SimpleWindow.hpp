#ifndef SIMPLE_WINDOW_HPP_
#define SIMPLE_WINDOW_HPP_
#include <cstdint>
#include <utility>
#include <optional>
#include <D3DHeaders.hpp>

class WindowClass
{
public:
	WindowClass();
	~WindowClass() noexcept;

	void Register() noexcept;

	[[nodiscard]]
	static const char* GetName() noexcept { return wndClassName; }

	[[nodiscard]]
	HINSTANCE GetHInstance() const noexcept { return m_wndClass.hInstance; }

private:
	WNDCLASSEX m_wndClass;

	static constexpr const char* wndClassName = "Luna";

public:
	WindowClass(const WindowClass&) = delete;
	WindowClass& operator=(const WindowClass&) = delete;

	WindowClass(WindowClass&& other) noexcept : m_wndClass{ other.m_wndClass }
	{
		other.m_wndClass.hInstance = nullptr;
	}
	WindowClass& operator=(WindowClass&& other) noexcept
	{
		m_wndClass                 = other.m_wndClass;
		other.m_wndClass.hInstance = nullptr;

		return *this;
	}
};

class SimpleWindow
{
public:
	SimpleWindow(std::uint32_t width, std::uint32_t height, const char* name);
	~SimpleWindow() noexcept;

	[[nodiscard]]
	void* GetWindowHandle() const noexcept { return m_hWnd; }
	[[nodiscard]]
	void* GetModuleInstance() const noexcept { return m_windowClass.GetHInstance(); }

	void SetWindowResolution(std::uint32_t width, std::uint32_t height);

	void Show();

	static LRESULT CALLBACK HandleMsgSetup(
		HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam
	) noexcept;
	static LRESULT CALLBACK HandleMsgWrap(
		HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam
	) noexcept;

	[[nodiscard]]
	LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	[[nodiscard]]
	std::optional<int> Update();

private:
	void SelfDestruct() noexcept;

private:
	HWND        m_hWnd;
	WindowClass m_windowClass;
	RECT        m_windowRect;

public:
	SimpleWindow(const SimpleWindow&) = delete;
	SimpleWindow& operator=(const SimpleWindow&) = delete;

	SimpleWindow(SimpleWindow&& other) noexcept
		: m_hWnd{ other.m_hWnd }, m_windowClass{ std::move(other.m_windowClass) },
		m_windowRect{ other.m_windowRect }
	{
		other.m_hWnd = nullptr;
	}
	SimpleWindow& operator=(SimpleWindow&& other) noexcept
	{
		SelfDestruct();

		m_hWnd        = other.m_hWnd;
		m_windowClass = std::move(other.m_windowClass);
		m_windowRect  = other.m_windowRect;
		other.m_hWnd  = nullptr;

		return *this;
	}
};
#endif
