#ifndef __STATICOBJECTDETECTER__
#define __STATICOBJECTDETECTER__

#include "BlobExtractor.hpp"
#include "FeatureMatcher.hpp"
#include <vector>
#include <cmath>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "boost/shared_array.hpp"
#include <unordered_set>

namespace DaVinci
{
    using namespace cv;
    using namespace std;
    typedef cv::Vec3f Circle;
    /// <summary>
    /// An object detecter that detects any meaningful object from a static image.
    /// TODO: abstract and return detected objects instead of Rectangle or ConvexHull.
    /// </summary>
    class StaticObjectDetector 
    {
        /// <summary>
        /// Leverage contour information to detect objects
        /// </summary>
    public:
        StaticObjectDetector(const cv::Mat &image, int algoFlag=0);
        /// <summary>
        /// Select the algorithms to use.
        /// </summary>
        /// <param name="flag">The flag mask (see ALGO_XXX) </param>
        void SetAlgoFlag(int flag);

        /// <summary>
        /// Calculate blobs for CCL
        /// </summary>
        void calculateBlobs(Connectivity connectivity = Connectivity::CONNECTIVITY_8, double connectThreshold = 3);

        /// <summary>
        /// Get blob set
        /// </summary>
        /// <returns></returns>
        boost::shared_ptr<unordered_set<boost::shared_ptr<BlobExtractor::Blob> > > getBlobSet() const;

        /// <summary>
        /// Get the rectangle that bounds the object at "where".
        /// </summary>
        /// <param name="where">Where the point locates</param>
        /// <returns>The bounding rect of the object</returns>
        Rect GetBoundingRect( Point pt);
        Rect GetBoundingRectCCL(Point pt);

        /// <summary>
        /// Get bounding rectangle by CCL with blob as parameter
        /// </summary>
        /// <param name="blob"></param>
        /// <returns></returns>
        Rect GetBoundingRectCCLByBlob(const BlobExtractor::Blob &blob);

        const static int ALGO_CONTOUR = 1;
        /// <summary>
        /// Leverage connected component labeling to detect objects
        /// </summary>
        const  static int ALGO_CCL = 2;
        /// <summary>
        /// Leverage OCR to detect objects
        /// </summary>
        const static int ALGO_OCR = 4;
        /// <summary>
        /// Grab a SURF-able object
        /// </summary>
        const static int ALGO_SURF = 8;

    private:
        const static int maxImageSize = 512;
        const static  int defaultRadius = 40;

       boost::shared_ptr<unordered_set<boost::shared_ptr<BlobExtractor::Blob>>> blobs;
        boost::shared_array<boost::shared_array<boost::shared_ptr<BlobExtractor::Blob>>> blobLookupTable;
        vector<Rect> textLocations;

        std::vector<Circle> circles;
        FeatureMatcher surf;
        double scale;
        int algoFlag;
        bool contourComputed;
        bool cclComputed;
        bool ocrComputed;
        bool surfComputed;
        cv::Mat image;
        bool IsPointInCircle(Circle c, Point pt);

        Rect CircleToRect(Circle c);

        Rect GetBoundingRectContour(Point pt);

        //Seq<Point2f> GetConvexHullContour(Point pt);
        Rect GetBoundingRectOCR(Point pt);

        //Seq<Point2f> GetConvexHullOCR(Point pt);

        Rect GetBoundingRectSURF(Point pt);

        //Seq<Point2f> GetConvexHullSURF(Point pt);

        Rect GetPointRectangle(Point pt);

        bool IsPointRectangle(Rect rect);

        //bool IsPointConvexHull(const Seq<Point2f> &convexHull);

        Rect GetDefaultRectangle(Point pt);

        //Seq<Point2f> RectToConvexHull(Rect rect);

        //Seq<Point2f> ResizeConvexHull(const Seq<Point2f> &convexHull, double ratio);

        Rect ResizeRectangle(Rect rect, double ratio);
        /// <summary>
        /// Constructor that accepts an image to detect objects from. The algorithms
        /// are picked from the algoFlag.
        /// </summary>
        /// <param name="image">The image to detect object from.</param>
        /// <param name="algoFlag">
        /// The flag to indicate which algorithms to use for detection. When no algorithm is selected,
        /// a default rectangle will be picked.
        /// </param>
        /// <summary>
        /// Get bounding rectangle by CCL
        /// </summary>
        /// <param name="where"></param>
        /// <returns></returns>

    };
}


#endif	//#ifndef __STATICOBJECTDETECTER__
