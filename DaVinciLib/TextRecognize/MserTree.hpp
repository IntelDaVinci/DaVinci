#ifndef MSER_TREE_HPP
#define MSER_TREE_HPP

#include "opencv/cv.h"
#include <vector>
namespace DaVinci
{
    using namespace cv;
    using namespace std;
    class MSERTree
    {
    public:
        explicit MSERTree( int delta=5, int minArea=60, int maxArea=14400,
            double maxVariation=0.5, double minDiversity=.33 );
       // explicit MSERTree( CvMSERParams param );
       // void operator() ( const Mat& img, vector<vector<Point>>& pts, vector<Rect>& rects );
        void operator()( const Mat& img, vector<CvContour*>& blackMsers, vector<CvContour*>& whiteMsers );
    private:
        cv::Ptr<CvMemStorage> blackStorage;
        CvSeq *blackContours;
        cv::Ptr<CvMemStorage> whiteStorage;
        CvSeq *whiteContours;
        int delta;
        int minArea;
        int maxArea;
        double maxVariation;
        double minDiversity;
    };
}
#endif