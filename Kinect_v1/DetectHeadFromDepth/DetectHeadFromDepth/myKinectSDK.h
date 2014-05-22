#pragma once

/// <TODO>
/// 追加のインクルードディレクトリに"$(KINECTSDK10_DIR)\inc"を追加
/// </TODO>

// NuiApi.hの前にWindows.hをインクルードする
#include <Windows.h>
#include <NuiApi.h>

// ライブラリのリンク（不要な物はコメントアウト）
#pragma comment(lib, "C:\\Program Files\\Microsoft SDKs\\Kinect\\v1.8\\lib\\x86\\Kinect10.lib")
