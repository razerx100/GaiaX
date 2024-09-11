#include <SimpleWindow.hpp>

WindowClass::WindowClass()
	: m_wndClass{
		.cbSize        = static_cast<UINT>(sizeof(m_wndClass)),
		.style         = CS_OWNDC,
		.lpfnWndProc   = SimpleWindow::HandleMsgSetup,
		.lpszClassName = GetName()
	} {}

WindowClass::~WindowClass() noexcept
{
	if (m_wndClass.hInstance)
		UnregisterClassA(GetName(), m_wndClass.hInstance);
}

void WindowClass::Register() noexcept
{
	m_wndClass.hInstance = GetModuleHandleA(nullptr);

	RegisterClassEx(&m_wndClass);
}

// Simple Window
SimpleWindow::SimpleWindow(
	std::uint32_t width, std::uint32_t height, const char* name
) : m_hWnd{ nullptr }, m_windowClass{}, m_windowRect{ 0, 0, 0, 0 }
{
	m_windowClass.Register();

	DWORD windowStyle = WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;

	RECT wr{
		.left   = 0,
		.top    = 0,
		.right  = static_cast<LONG>(width),
		.bottom = static_cast<LONG>(height)
	};

	AdjustWindowRect(&wr, windowStyle, FALSE);

	m_hWnd = CreateWindowExA(
		0,
		m_windowClass.GetName(), name,
		windowStyle,
		CW_USEDEFAULT, CW_USEDEFAULT,
		static_cast<int>(wr.right - wr.left),
		static_cast<int>(wr.bottom - wr.top),
		nullptr, nullptr, m_windowClass.GetHInstance(), this
	);

	GetWindowRect(m_hWnd, &m_windowRect);
}

SimpleWindow::~SimpleWindow() noexcept
{
	SelfDestruct();
}

void SimpleWindow::Show()
{
	ShowWindow(m_hWnd, SW_SHOWDEFAULT);
}

void SimpleWindow::SelfDestruct() noexcept
{
	if (m_hWnd)
		SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(DefWindowProcA));
}

LRESULT CALLBACK SimpleWindow::HandleMsgSetup(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam
) noexcept {
	if (msg == WM_NCCREATE)
	{
		auto pCreate = reinterpret_cast<CREATESTRUCTA*>(lParam);
		auto pWnd    = static_cast<SimpleWindow*>(pCreate->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
		SetWindowLongPtr(
			hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&SimpleWindow::HandleMsgWrap)
		);

		return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void SimpleWindow::SetWindowResolution(std::uint32_t width, std::uint32_t height)
{
	MoveWindow(m_hWnd, m_windowRect.left, m_windowRect.top, width, height, TRUE);

	GetWindowRect(m_hWnd, &m_windowRect);
}

LRESULT CALLBACK SimpleWindow::HandleMsgWrap(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam
) noexcept {
	auto pWnd = reinterpret_cast<SimpleWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK SimpleWindow::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	switch (msg) {
	case WM_CLOSE: {
		DestroyWindow(m_hWnd);
		return 0;
	}
	case WM_DESTROY: {
		PostQuitMessage(0);
		return 0;
	}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

std::optional<int> SimpleWindow::Update()
{
	MSG msg{};

	while (PeekMessageA(&msg, nullptr, 0u, 0u, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			return static_cast<int>(msg.wParam);

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return {};
}
