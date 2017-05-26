#include "StaticObjectDetector.hpp"
#include "TextRecognize.hpp"

namespace DaVinci
{
    using namespace cv;
    using namespace std;

    StaticObjectDetector::StaticObjectDetector(const cv::Mat &imageP, int algoFlag)
    {
        algoFlag = 0;
        contourComputed = false;
        cclComputed = false;
        ocrComputed = false;
        surfComputed = false;    
        blobs=NULL;
        //blobLookupTable=NULL;

        textLocations =vector<Rect>();
        circles =vector<Circle>();

        if (!imageP .empty())
        {
            image = imageP.clone();
        }
        else
        {
            image = Mat();
        }
        SetAlgoFlag(algoFlag);
    }


    void StaticObjectDetector::SetAlgoFlag(int flag)
    {
        algoFlag = flag;
        if ((this->algoFlag & ALGO_CONTOUR) != 0 && !contourComputed)
        {
            cv::Mat gray;
            cvtColor(image,gray,CV_BGR2GRAY);
            Mat graycopy=gray.clone(); 
            GaussianBlur(graycopy,gray,Size(5,5),0);

            // detect circles with 10 steps to improve performance, otherwise HoughCircles is very slow.
            int minRadius = 11;
            int diffRadius = min(gray.cols, gray.rows) / 2 - minRadius;
            int steps = 10;
            int radiusStep = diffRadius / steps;
            for (int i = 0; i < steps; i++)
            {
                int min = minRadius + i * radiusStep;
                int max = min + radiusStep;
                vector<Circle> c;
                HoughCircles(gray,c,CV_HOUGH_GRADIENT,1,min-1,50,40,min,max);
                circles.insert(circles.end(),c.begin(),c.end());
            }
            contourComputed = true;
        }
        if ((this->algoFlag & ALGO_CCL) != 0 && !cclComputed)
        {
            calculateBlobs();
            cclComputed = true;
        }
        if ((this->algoFlag & ALGO_OCR) != 0 && !ocrComputed)
        {
            TextRecognize ocr = TextRecognize();

            TextRecognize::TextRecognizeResult ocrResult = ocr.DetectDetail(image, TextRecognize::PARAM_CAMERA_IMAGE);
            if (ocrResult.GetRecognizedText()!="")
            {
                textLocations = ocrResult.GetBoundingRects("(\\b\\S+\\b)");
                ocrComputed = true;
            }
        }
        if ((this->algoFlag & ALGO_SURF) != 0 && !surfComputed)
        {
            surf.SetModelImage(image);
            surfComputed = true;
        }
    }

    void StaticObjectDetector::calculateBlobs(Connectivity connectivity, double connectThreshold )
    {
        // scale the image to get better performance from CCL
        double scaleHeight = 1.0;
        if (image.rows > maxImageSize)
        {
            scaleHeight = (double)maxImageSize / image.rows;
        }
        double scaleWidth = 1.0;
        if (image.cols > maxImageSize)
        {
            scaleWidth = (double)maxImageSize / image.cols;
        }
        this->scale = scaleHeight > scaleWidth ? scaleWidth : scaleHeight;
        // for colorful image, L*a*b can better differentiate colors using Euclidean distance
        Mat labImageTemp;
        cvtColor(image,labImageTemp,CV_BGR2Lab);
        Mat labImage;
        resize(labImageTemp,labImage,Size(),scale,scale,CV_INTER_CUBIC);
        BlobExtractor blobExtractor;

        blobs = blobExtractor.Extract(labImage, connectivity, connectThreshold);    
        boost::shared_array<boost::shared_array<boost::shared_ptr<BlobExtractor::Blob> > > tableTemp(new boost::shared_array<boost::shared_ptr<BlobExtractor::Blob>>[labImage.cols]);

        // blobLookupTable(new boost::shared_array<BlobExtractor::Blob>[labImage.cols] );
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
        for(int i=0;i<labImage.cols;++i)
        {
            tableTemp[i].reset(new boost::shared_ptr<BlobExtractor::Blob>[labImage.rows]);
        }
        blobLookupTable = tableTemp;
        // unordered_set<boost::shared_ptr<BlobExtractor::Blob>> blobs;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
        for( unordered_set<boost::shared_ptr<BlobExtractor::Blob>>::iterator it = (blobs)->begin();it!=(blobs)->end();++it)   
        {
            // vector<Point2i>pts = (*it)->GetPoints();
            boost::shared_ptr<vector<Point2i>> innPts = (*it)->innerPoints;
            if (innPts != nullptr)
            {
                int size =innPts->size();
                for(int i=0;i<size;++i)
                {
                    blobLookupTable[(*innPts)[i].x][(*innPts)[i].y] = *it;
                }
            }

            boost::shared_ptr<vector<Point2i>> edgPts = (*it)->edgePoints;
            if (edgPts != nullptr)
            {
                int size = edgPts->size();
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
                for(int i=0;i<size;++i)
                {
                    blobLookupTable[(*edgPts)[i].x][(*edgPts)[i].y] = *it;
                }
            }
        }

    }

    boost::shared_ptr<unordered_set<boost::shared_ptr<BlobExtractor::Blob>>> StaticObjectDetector::getBlobSet() const
    {
        if(blobs->size()==0)
        {
            boost::shared_ptr<unordered_set<boost::shared_ptr<BlobExtractor::Blob>>> res(new  unordered_set<boost::shared_ptr<BlobExtractor::Blob>>());
            return res;
        }
        else
        {
            return blobs;
        }
    }

    Rect StaticObjectDetector::GetBoundingRect(Point pt)
    {
        Rect result;
        if ((this->algoFlag & ALGO_CONTOUR) != 0)
        {
            result = GetBoundingRectContour(pt);
        }
        if (!IsPointRectangle(result))
        {
            return result;
        }

        if ((this->algoFlag & ALGO_CCL) != 0)
        {
            result = GetBoundingRectCCL(pt);
        }
        if (!IsPointRectangle(result))
        {
            return result;
        }

        if ((this->algoFlag & ALGO_OCR) != 0)
        {
            result = GetBoundingRectOCR(pt);
        }
        if (!IsPointRectangle(result))
        {
            return result;
        }

        if ((this->algoFlag & ALGO_SURF) != 0)
        {
            result = GetBoundingRectSURF(pt);
        }
        if (!IsPointRectangle(result))
        {
            return result;
        }

        return GetDefaultRectangle(pt);
    }

    bool StaticObjectDetector::IsPointInCircle(Circle c, Point pt)
    {
        //circles ?Output vector of found circles. Each vector is encoded as a 3-element floatingpoint vector (x; y; radius) .
        double diffx = c[0] - pt.x;
        double diffy = c[1] - pt.y;
        return sqrt(diffx * diffx + diffy * diffy) < c[2];
    }

    Rect StaticObjectDetector::CircleToRect(Circle c)
    {
        double x = c[0] - c[2];
        double y = c[1] - c[2];
        double bottomX = c[0]+ c[2];
        double bottomY = c[1] + c[2];
        if (x < 0)
        {
            x = 0;
        }
        if (y < 0)
        {
            y = 0;
        }
        if (bottomX > image.cols)
        {
            bottomX =image.cols;
        }
        if (bottomY > image.rows)
        {
            bottomY = image.rows;
        }

        return Rect(static_cast<int>(x), static_cast<int>(y), static_cast<int>(bottomX - x), static_cast<int>(bottomY - y));

    }

    Rect StaticObjectDetector::GetBoundingRectContour(Point pt)
    {
        int size = circles.size();
        for(int i=0;i<size;++i)
        {
            if(IsPointInCircle(circles[i],pt))
            {
                return CircleToRect(circles[i]);
            }
        }
        return GetPointRectangle(pt);
    }

    //Seq<Point2f> StaticObjectDetector::GetConvexHullContour(Point pt)
    //{
    //    // TODO: compute convex hull instead of rectangle
    //    return RectToConvexHull(GetBoundingRectContour(pt));
    //}

    Rect StaticObjectDetector::GetBoundingRectCCL(Point pt)
    {
        boost::shared_ptr<BlobExtractor::Blob> whereBlob = (blobLookupTable[static_cast<int>(pt.x * scale)][static_cast<int>(pt.y * scale)]);
        // FIXME: whereBlob should always be non-null
        if (whereBlob->edgePoints->size()!=0)
        {
            return GetBoundingRectCCLByBlob( *whereBlob);
        }
        else
        {
            return GetPointRectangle(pt);
        }
    }

    Rect StaticObjectDetector::GetBoundingRectCCLByBlob(const BlobExtractor::Blob &blob)
    {
        return ResizeRectangle(blob.boundingRectCache, 1.0 / scale);
    }

    Rect StaticObjectDetector::GetBoundingRectOCR(Point pt)
    {
        int size = textLocations.size();
        for(int i=0;i<size;++i)
        {
            if(textLocations[i].contains(pt))
            {
                return textLocations[i];
            }
        }
        return GetPointRectangle(pt);
    }

    //Seq<Point2f> StaticObjectDetector::GetConvexHullOCR(Point pt)
    //{
    //    return RectToConvexHull(GetBoundingRect(pt));
    //}

    // this is a functor
    struct KeyPointCompare {
        KeyPointCompare(Point x) : pt(x) {}
        int operator()(KeyPoint p1, KeyPoint p2) { return (int)((p1.pt.x-pt.x)*(p1.pt.x-pt.x)+(p1.pt.y-pt.y)*((p1.pt.y-pt.y))-(p2.pt.x-pt.x)*(p2.pt.x-pt.x)+(p2.pt.y-pt.y)*((p2.pt.y-pt.y))); }

    private:
        Point pt;
    };

    Rect StaticObjectDetector::GetBoundingRectSURF(Point pt)
    {
        int numRequiredKeyPoints = 150;
        int keyPointsStep = 50;
        vector<KeyPoint> keypoints= surf.GetKeyPoints();
        if (keypoints.size()<=0)
        {
            return GetPointRectangle(pt);
        }
        Rect roi ;
        int size=keypoints.size();
        while(numRequiredKeyPoints < size)
        {
            vector<KeyPoint> keypointsTemp = keypoints;
            sort(keypointsTemp.begin(),keypointsTemp.end(),KeyPointCompare(pt));
            std::vector<Point2f> points = std::vector<Point2f>(numRequiredKeyPoints);
            for (int i = 0; i < numRequiredKeyPoints; i++)
            {
                points[i] = keypointsTemp[i].pt;
            }
            // verify the match
            roi = boundingRect(points);

            Mat imageCopy = image.clone();
            cv::Mat model =imageCopy(roi);

            FeatureMatcher modelSurf;
            modelSurf.SetModelImage(model);
            if (!modelSurf.Match(surf).empty())
            {
                Rect matchedROI = modelSurf.GetBoundingRect();
                if (abs(matchedROI.x - roi.x) < 3 && abs(matchedROI.y - roi.y) < 3 && abs(matchedROI.width - roi.width) < 3 && abs(matchedROI.height - roi.height) < 3)
                {
                    return roi;
                }
            }
            numRequiredKeyPoints += keyPointsStep;
        }
        return roi;
    }

    //Seq<Point2f> StaticObjectDetector::GetConvexHullSURF(Point pt)
    //{
    //    return RectToConvexHull(GetBoundingRectSURF(pt));
    //}

    Rect StaticObjectDetector::GetPointRectangle(Point pt)
    {
        return Rect(pt, Size(0, 0));
    }

    bool StaticObjectDetector::IsPointRectangle(Rect rect)
    {
        return rect.width == 0 && rect.height == 0;
    }

    //bool StaticObjectDetector::IsPointConvexHull(const Seq<Point2f> &convexHull)
    //{
    //    return convexHull.size() == 1;
    //}

    Rect StaticObjectDetector::GetDefaultRectangle(Point pt)
    {
        return CircleToRect(Circle((float)pt.x,(float)pt.y, (float)defaultRadius));
    }

    //Seq<Point2f> StaticObjectDetector::RectToConvexHull(Rect rect)
    //{
    //    Seq<Point2f> result;

    //    result.push_back(Point2f((float)rect.x, (float)rect.y));
    //    result.push_back(Point2f((float)rect.x,(float)(rect.y + rect.height)));
    //    result.push_back(Point2f((float)(rect.x + rect.width), (float)(rect.y + rect.height)));
    //    result.push_back(Point2f((float)(rect.x + rect.width), (float)rect.y));
    //    return result;
    //}

    //Seq<Point2f> StaticObjectDetector::ResizeConvexHull(const Seq<Point2f> &convex, double ratio)//need to check
    //{
    //    vector<Point2f> vectorTemp(convex.size());
    //    int vectorTempSize=vectorTemp.size();
    //    for (int i = 0; i < vectorTempSize; i++)
    //    {
    //        vectorTemp[i].x = static_cast<float>( convex[i].x * ratio);
    //        vectorTemp[i].y = static_cast<float>( convex[i].y * ratio);
    //    }
    //    vector<Point2f> hullTemp;
    //    convexHull(vectorTemp,hullTemp,false);
    //    Seq<Point2f> result;
    //    int hullTempSize = hullTemp.size();
    //    for(int i=0;i<hullTempSize;++i)
    //    {
    //        result.push_back(hullTemp[i]);
    //    }
    //    return result;
    //}

    Rect StaticObjectDetector::ResizeRectangle(Rect rect, double ratio)
    {
        Rect result;
        result.x = static_cast<int>(rect.x * ratio);
        result.y = static_cast<int>(rect.y * ratio);
        result.width = static_cast<int>(rect.width * ratio);
        result.height = static_cast<int>(rect.height * ratio);
        return result;
    }
}
