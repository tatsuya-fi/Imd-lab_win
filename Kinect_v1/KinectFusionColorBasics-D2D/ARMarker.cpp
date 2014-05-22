#include "ARMarker.h"

#ifdef _WIN32
char			*vconf = "Data\\WDM_camera_flipV.xml";
#else
char			*vconf = "";
#endif

ARMarker::ARMarker()
{
	// OpenCV��p���ĉ摜���L���v�`�����Ă���̂ŕs�g�p
	if (!setupCamera(cparam_name, vconf, &gARTCparam))
	{
		fprintf(stderr, "main(): Unable to set up AR camera.\n");
		exit(-1);
	}
	if (!setupMarker(patt_name, &gPatt_id)) {
		fprintf(stderr, "main(): Unable to set up AR marker.\n");
		exit(-1);
	}
}


ARMarker::~ARMarker()
{
}

//
// OpenCV����ǂݍ��񂾉摜���g�����߃J�����摜�̃L���v�`���͍s��Ȃ�
//
int ARMarker::setupCamera(const char *cparam_name, char *vconf, ARParam *cparam)
{
	ARParam			wparam;
	int				xsize, ysize;

	// Open the video path.
	//if (arVideoOpen(vconf) < 0) {
	//	fprintf(stderr, "setupCamera(): Unable to open connection to camera.\n");
	//	return (FALSE);
	//}

	// Find the size of the window.
	if (arVideoInqSize(&xsize, &ysize) < 0) return (FALSE);
	fprintf(stdout, "Camera image size (x,y) = (%d,%d)\n", xsize, ysize);

	// Load the camera parameters, resize for the window and init.
	if (arParamLoad(cparam_name, 1, &wparam) < 0) {
		fprintf(stderr, "setupCamera(): Error loading parameter file %s for camera.\n", cparam_name);
		return (FALSE);
	}
	arParamChangeSize(&wparam, xsize, ysize, cparam);
	fprintf(stdout, "*** Camera Parameter ***\n");
	arParamDisp(cparam);

	arInitCparam(cparam);

	//if (arVideoCapStart() != 0) {
	//	fprintf(stderr, "setupCamera(): Unable to begin camera data capture.\n");
	//	return (FALSE);
	//}

	fprintf(stdout, "setupCamera done\n");
	return (TRUE);
}

int ARMarker::setupMarker(const char *patt_name, int *patt_id)
{

	if ((*patt_id = arLoadPatt(patt_name)) < 0) {
		fprintf(stderr, "setupMarker(): pattern load error !!\n");
		return (FALSE);
	}

	return (TRUE);
}