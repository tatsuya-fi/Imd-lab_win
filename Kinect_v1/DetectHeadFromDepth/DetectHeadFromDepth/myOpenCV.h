#pragma once

/// <TODO>
/// 追加のインクルードディレクトリに"C:\opencv249\opencv\build\include"を追加
/// </TODO>

#include <opencv2/opencv.hpp>

// バージョン取得
#define CV_VERSION_STR CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) CVAUX_STR(CV_SUBMINOR_VERSION)

// ビルドモード
#ifdef _DEBUG
#define CV_EXT_STR "d.lib"
#else
#define CV_EXT_STR ".lib"
#endif

/// <Summary>
/// ライブラリのリンク（不要な物はコメントアウト）
///　</Summary>
//　作業PC用
#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc11\\lib\\opencv_core"       CV_VERSION_STR CV_EXT_STR)
#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc11\\lib\\opencv_highgui"    CV_VERSION_STR CV_EXT_STR)
#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc11\\lib\\opencv_imgproc"    CV_VERSION_STR CV_EXT_STR)
#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc11\\lib\\opencv_calib3d"    CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc11\\lib\\opencv_gpu"        CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc11\\lib\\opencv_video"      CV_VERSION_STR CV_EXT_STR)
#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc11\\lib\\opencv_objdetect"  CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc11\\lib\\opencv_features2d" CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc11\\lib\\opencv_flann"      CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc11\\lib\\opencv_ffmpeg"     CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc11\\lib\\opencv_ts"         CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc11\\lib\\opencv_contrib"    CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc11\\lib\\opencv_ml"         CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc11\\lib\\opencv_legacy"     CV_VERSION_STR CV_EXT_STR)
// ミーティング室用
//#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\build\\x86\\vc11\\lib\\opencv_core"       CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\build\\x86\\vc11\\lib\\opencv_highgui"    CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\build\\x86\\vc11\\lib\\opencv_imgproc"    CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\build\\x86\\vc11\\lib\\opencv_calib3d"    CV_VERSION_STR CV_EXT_STR)
////#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\build\\x86\\vc11\\lib\\opencv_gpu"        CV_VERSION_STR CV_EXT_STR)
////#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\build\\x86\\vc11\\lib\\opencv_video"      CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\build\\x86\\vc11\\lib\\opencv_objdetect"  CV_VERSION_STR CV_EXT_STR)
////#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\build\\x86\\vc11\\lib\\opencv_features2d" CV_VERSION_STR CV_EXT_STR)
////#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\build\\x86\\vc11\\lib\\opencv_flann"      CV_VERSION_STR CV_EXT_STR)
////#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\build\\x86\\vc11\\lib\\opencv_ffmpeg"     CV_VERSION_STR CV_EXT_STR)
////#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\build\\x86\\vc11\\lib\\opencv_ts"         CV_VERSION_STR CV_EXT_STR)
////#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\build\\x86\\vc11\\lib\\opencv_contrib"    CV_VERSION_STR CV_EXT_STR)
////#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\build\\x86\\vc11\\lib\\opencv_ml"         CV_VERSION_STR CV_EXT_STR)
////#pragma comment(lib, "C:\\opencv" CV_VERSION_STR "\\build\\x86\\vc11\\lib\\opencv_legacy"     CV_VERSION_STR CV_EXT_STR)




/////////////////////////////////
// Additional option

// キー入力
#define KEY_ESC		0x1b
#define KEY_SPACE	0x20


/// マウスイベント
//コールバック関数からのイベント情報を取得するための構造体
typedef struct MouseParam{
	unsigned int x;
	unsigned int y;
	int event;
	int flags;
} MouseParam;

