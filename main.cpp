/*
 * Capture image and rotate
 */

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;

int main(int argc, char **argv)
{
	VideoCapture camera(0);
	Point2f center = Point2f(camera.get(CV_CAP_PROP_FRAME_WIDTH / 2), camera.get(CV_CAP_PROP_FRAME_HEIGHT / 2));
	Mat matrix = getRotationMatrix2D(center, 45, 1.0);
	for (bool loop = camera.isOpened(); loop; )
	{
		if (waitKey(10) == 'q')
			break;
		Mat frame, rotated;
		camera >> frame;

		warpAffine(frame, rotated, matrix, frame.size(), INTER_LINEAR);
		imshow("Input", frame);
		imshow("Rotated", rotated);
	}

}
