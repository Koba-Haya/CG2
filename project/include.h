#pragma once

#include <cassert>
#include <chrono> //時間を扱うライブラリ
#include <cstdint>
#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <filesystem> //ファイルやディレクトリに関する操作を行うライブラリ
#include <format>
#include <fstream> //ファイルに書いたり読んだりするライブラリ
#include <string>
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/imgui//imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
