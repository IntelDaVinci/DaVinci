#ifndef MSER_HPP
#define MSER_HPP
#include "opencv/cv.h"
namespace DaVinci
{
    using namespace cv;
    using namespace std;
    void cvExtractMSER( CvArr* _img,
        CvArr* _mask,
        CvSeq** _contours,
        CvMemStorage* storage,
        CvMSERParams params,
        int direction); //MSER- (direction < 0)  MSER+ (direction > 0) MSER- and MSER+ (direction == 0)

    CvMSERParams cvMSERParams( int delta, int minArea, int maxArea, float maxVariation, float minDiversity);
}
#endif