#pragma once

#include "myARToolKit.h"

class ARMarker
{
private:
	// Data directories
	const char *cparam_name = "Data/camera_para.dat";
	const char *patt_name	= "Data/patt.hiro";
	// Prefereces
	const int	prefWindowed = TRUE;
	const int	prefDepth = 32;
	const int	preRefresh = 0;
	int			prefWidth;
	int			prefHeight;
	

	// Image acquisition.
	ARUint8			*gARTImage;

	// Marker detection.
	const int			gARTThreshhold = 100;
	const long			gCallCountMarkerDetect = 0;

	// Transformation matrix retrieval.
	const double		gPatt_width = 80.0;	// Per-marker, but we are using only 1 marker.
	double				gPatt_centre[2]; // Per-marker, but we are using only 1 marker.
	double				gPatt_trans[3][4];		// Per-marker, but we are using only 1 marker.
	int					gPatt_found = FALSE;	// Per-marker, but we are using only 1 marker.
	int					gPatt_id;				// Per-marker, but we are using only 1 marker.

	// Drawing.
	ARParam				gARTCparam;
	ARGL_CONTEXT_SETTINGS_REF gArglSettings = NULL;
	int gDrawRotate = FALSE;
	float gDrawRotateAngle = 0;			// For use in drawing.

	int setupCamera(const char *cparam_name, char *vconf, ARParam *cparam);
	int setupMarker(const char *patt_name, int *patt_id);


public:
	ARMarker();
	~ARMarker();

	void setDisplaySize(DWORD width, DWORD height);
};

