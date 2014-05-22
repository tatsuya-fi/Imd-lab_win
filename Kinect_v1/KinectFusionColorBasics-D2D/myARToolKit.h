// <TODO>
// 追加のインクルードディレクトリに
// C:\ARToolKit\include;
// を追加する．
// </TODO>

#include <stdio.h>
#include <stdlib.h>	
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <AR/config.h>
#include <AR/video.h>
#include <AR/param.h>			// arParamDisp()
#include <AR/ar.h>
#include <AR/gsub_lite.h>


// ビルドモード
#ifdef _DEBUG
#define AR_EXT_STR "d.lib"
#else
#define AR_EXT_STR ".lib"
#endif

// ライブラリのリンク（不要な物はコメントアウト）
#pragma comment(lib, "C:\\ARToolKit\\lib\\libAR"			AR_EXT_STR)
#pragma comment(lib, "C:\\ARToolKit\\lib\\libARgsub"		AR_EXT_STR)
#pragma comment(lib, "C:\\ARToolKit\\lib\\libARgsubUtil"	AR_EXT_STR)
#pragma comment(lib, "C:\\ARToolKit\\lib\\libARgsub_lite"	AR_EXT_STR)
#pragma comment(lib, "C:\\ARToolKit\\lib\\libARMulti"		AR_EXT_STR)
#pragma comment(lib, "C:\\ARToolKit\\lib\\libARvideo"		AR_EXT_STR)
#pragma comment(lib, "C:\\ARToolKit\\lib\\libARvrml"		AR_EXT_STR)
