#pragma once

/// <TODO>
/// 追加のインクルードディレクトリに"D:\opencv248\opencv\build\include"を追加
/// </TODO>

#include <fstream>

#include <opencv2/opencv.hpp>

// バージョン取得
#define CV_VERSION_STR CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) CVAUX_STR(CV_SUBMINOR_VERSION)

// ビルドモード
#ifdef _DEBUG
#define CV_EXT_STR "d.lib"
#else
#define CV_EXT_STR ".lib"
#endif

// ライブラリのリンク（不要な物はコメントアウト）
#pragma comment(lib, "D:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc12\\lib\\opencv_core"       CV_VERSION_STR CV_EXT_STR)
#pragma comment(lib, "D:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc12\\lib\\opencv_highgui"    CV_VERSION_STR CV_EXT_STR)
#pragma comment(lib, "D:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc12\\lib\\opencv_imgproc"    CV_VERSION_STR CV_EXT_STR)
#pragma comment(lib, "D:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc12\\lib\\opencv_calib3d"    CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "D:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc12\\lib\\opencv_gpu"        CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "D:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc12\\lib\\opencv_video"      CV_VERSION_STR CV_EXT_STR)
#pragma comment(lib, "D:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc12\\lib\\opencv_objdetect"  CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "D:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc12\\lib\\opencv_features2d" CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "D:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc12\\lib\\opencv_flann"      CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "D:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc12\\lib\\opencv_ffmpeg"     CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "D:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc12\\lib\\opencv_ts"         CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "D:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc12\\lib\\opencv_contrib"    CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "D:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc12\\lib\\opencv_ml"         CV_VERSION_STR CV_EXT_STR)
//#pragma comment(lib, "D:\\opencv" CV_VERSION_STR "\\opencv\\build\\x86\\vc12\\lib\\opencv_legacy"     CV_VERSION_STR CV_EXT_STR)


/////////////////////////////////
// Additional option
#define KEY_ESC		0x1b
#define KEY_SPACE	0x20
