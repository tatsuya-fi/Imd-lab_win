#pragma once

/// <TODO>
/// 追加のインクルードディレクトリに以下のディレクトリを追加
/// $(PCL_ROOT)\3rdParty\VTK\include\vtk-5.8;$(PCL_ROOT)\3rdParty\Boost\include;$(PCL_ROOT)\3rdParty\Eigen\include;$(PCL_ROOT)\include\pcl-1.6;
/// </TODO>

#include <pcl/point_types.h>
#include <pcl/visualization/cloud_viewer.h>

// ビルドモード
#ifdef _DEBUG
#define PCL_EXT_STR "debug.lib"
#else
#define PCL_EXT_STR "release.lib"
#endif


// ライブラリのリンク（不要な物はコメントアウト）
#pragma comment(lib, "C:\\Program Files (x86)\\PCL 1.6.0\\lib\\pcl_apps_"				PCL_EXT_STR)
#pragma comment(lib, "C:\\Program Files (x86)\\PCL 1.6.0\\lib\\pcl_common_"				PCL_EXT_STR)
#pragma comment(lib, "C:\\Program Files (x86)\\PCL 1.6.0\\lib\\pcl_features_"			PCL_EXT_STR)
#pragma comment(lib, "C:\\Program Files (x86)\\PCL 1.6.0\\lib\\pcl_filters_"			PCL_EXT_STR)
#pragma comment(lib, "C:\\Program Files (x86)\\PCL 1.6.0\\lib\\pcl_io_ply_"				PCL_EXT_STR)
#pragma comment(lib, "C:\\Program Files (x86)\\PCL 1.6.0\\lib\\pcl_io_"					PCL_EXT_STR)
#pragma comment(lib, "C:\\Program Files (x86)\\PCL 1.6.0\\lib\\pcl_kdtree_"				PCL_EXT_STR)
#pragma comment(lib, "C:\\Program Files (x86)\\PCL 1.6.0\\lib\\pcl_keypoints_"			PCL_EXT_STR)
#pragma comment(lib, "C:\\Program Files (x86)\\PCL 1.6.0\\lib\\pcl_octree_"				PCL_EXT_STR)
#pragma comment(lib, "C:\\Program Files (x86)\\PCL 1.6.0\\lib\\pcl_registration_"		PCL_EXT_STR)
#pragma comment(lib, "C:\\Program Files (x86)\\PCL 1.6.0\\lib\\pcl_sample_consensus_"	PCL_EXT_STR)
#pragma comment(lib, "C:\\Program Files (x86)\\PCL 1.6.0\\lib\\pcl_search_"				PCL_EXT_STR)
#pragma comment(lib, "C:\\Program Files (x86)\\PCL 1.6.0\\lib\\pcl_segmentation_"		PCL_EXT_STR)
#pragma comment(lib, "C:\\Program Files (x86)\\PCL 1.6.0\\lib\\pcl_surface_"			PCL_EXT_STR)
#pragma comment(lib, "C:\\Program Files (x86)\\PCL 1.6.0\\lib\\pcl_tracking_"			PCL_EXT_STR)
#pragma comment(lib, "C:\\Program Files (x86)\\PCL 1.6.0\\lib\\pcl_visualization_"		PCL_EXT_STR)
