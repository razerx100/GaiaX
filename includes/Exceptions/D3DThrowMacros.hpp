#ifndef D3D_THROW_MACROS_HPP_
#define D3D_THROW_MACROS_HPP_

#include <D3DExceptions.hpp>

#define D3D_THROW_NOINFO(hr, hrCall) if(FAILED(hr = (hrCall))) throw HrException(__LINE__, __FILE__, hr)
#define D3D_GENERIC_THROW(errorMsg) throw GenericException(__LINE__, __FILE__, errorMsg)

#ifdef _DEBUG
#include <DebugInfoManager.hpp>
#include <Gaia.hpp>
#define DEBUG_INFO_MAN Gaia::debugInfo
#define D3D_THROW_NO_HR(funCall) DEBUG_INFO_MAN->Set(); try {funCall;} catch(...){ std::vector<std::string> vec = DEBUG_INFO_MAN->GetMessages(); if(!vec.empty()) throw InfoException(__LINE__, __FILE__, vec);}
#define D3D_THROW(hr) throw HrException(__LINE__, __FILE__, hr, DEBUG_INFO_MAN->GetMessages())
#define D3D_THROW_FAILED(hr, hrCall) DEBUG_INFO_MAN->Set(); if(FAILED(hr = (hrCall))) D3D_THROW(hr)
#define D3D_DEVICE_REMOVED_EXCEPT(hr) DeviceRemovedException(__LINE__, __FILE__, hr, DEBUG_INFO_MAN->GetMessages())
#else
#define D3D_THROW_NO_HR(funCall) funCall;
#define D3D_THROW(hr) throw HrException(__LINE__, __FILE__, hr)
#define D3D_THROW_FAILED(hr, hrCall) D3D_THROW_NOINFO(hr, hrCall)
#define D3D_DEVICE_REMOVED_EXCEPT(hr) DeviceRemovedException(__LINE__, __FILE__, hr)
#endif

#endif