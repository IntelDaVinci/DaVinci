#ifndef BINARIZATION_H
#define BINARIZATION_H

#include "opencv/cv.h"
#include "opencv2/core/core.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    enum class BinarizationType
    {
        KMEANS_COLOR,
        KMEANS_COLOR_SSP,  // the result is not good now, we may consider GMM
        OTSU,
        NIBLACK
    };

    class Binarization{
    public:
        /// two binary method can be chosen
        /// binary_method = 1, choose kmeans for binarization
        /// binary_mtehot = 2, choose otsu for binarization
        //int binary_method;
        //Binarization(int binary_option):binary_method(binary_option)
        //{};
        //~Binarization()
        //{};

        /// Text line binarization for text recognition
        /// <summary>
        /// <param name="color_img">[input]The source color image</param>
        /// <param name="rst_text_lines">[input]The output of RST</param>
        /// <param name="binary_text_lines">[output]The final binarization result, use refining method on kmeans binary result</param>
        /// <param name="BT">[input]choose different binarization method</param>
        /// </summary>
        void ImgBinarization(const Mat &color_img, const vector<vector<CvContour*>> &rst_text_lines, 
            vector<Mat> &binary_text_lines, BinarizationType BT);

        void operator()(const Mat &color_img, const vector<vector<CvContour*>> &rst_text_lines, 
            vector<Mat> &binary_text_lines, BinarizationType BT);
        //only support OTSU and Niblack
       static void ImgBinarization(const Mat& color_img, Mat& binary_img, BinarizationType BT);

    private:
        /// use multi times kmeans clustering and refining strategy for binarization
        /// <summary>
        /// <param name="color_roi">[input]The source color image</param>
        /// <param name="binary_roi">[input]The source binary image of RST</param>
        /// <param name="foreground_img">[output]The output binary image of kmeans</param>
        /// <param name="refine_img">[output]The output binary image of refining process</param>
        /// </summary>
        void KmeansClusterAndRefine(const Mat &color_roi, const Mat &binary_roi, Mat &binary_img);

        /// use multi times kmeans method for text region binarization
        /// <summary>
        /// <param name="img">[input]The source color image</param>
        /// <param name="binary_img">[input]The detected binary image of RST</param> 	kernel32.dll!761f338a()	Unknown

        /// <param name="foreground">[output]The output pixels of kmeans</param>
        /// <param name="foreground_img">[output]The output binary image of kmeans</param>
        /// </summary>
        int KmeansBinaryMulti(const Mat& img, const Mat& binary_img, 
            vector<Point>& foreground, Mat& foreground_img);

        /// use kmeans method and the SSP for text region binarization
        /// <summary>
        /// <param name="img">[input]The source color image</param>
        /// <param name="binary_img">[input]The detected binary image of scene text detection</param>
        /// <param name="text_pixels">[input]The output pixels of scene text detection</param>
        /// <param name="foreground">[output]The output pixels of kmeans</param>
        /// <param name="foreground_img">[output]The output binary image of kmeans</param>
        /// </summary>
        void KmeansBinarySSP(const Mat& img, const Mat& binary_img, const vector<Point>& text_pixels, Mat& foreground_img);

        /// use otsu method for text region binarization
        /// <summary>
        /// <param name="color_roi">[input]The color roi of a text line</param>
        /// <param name="binary_roi">[input]The binary roi of a text line</param>
        /// <param name="foreground">[output]The output pixels of otsu</param>
        /// <param name="textline_img">[output]The output binary image of otsu</param>
        /// </summary>
        void OTSUBinary(const Mat& color_roi, const Mat& binary_roi, Mat& textline_img);

        /// refined the obtained new CCs from kmeans clustering results
        /// <summary>
        /// <param name="color_roi">[input]The color roi of a text line</param>
        /// <param name="binary_roi">[input]The binary roi of a text line</param>
        /// <param name="foreground_img">[input]The binary image of kmeans result</param>
        /// <param name="refine_img">[output]The final binary image by performing refining strategy on kmeans result</param>
        /// </summary>
        void RefineNewCC(const Mat& color_roi, const Mat& binary_roi, 
            const Mat& foreground_img, Mat& refine_img);

        /// refined the obtained new CCs from otsu (without using color information)
        /// <summary>
        /// <param name="color_roi">[input]The color roi of a text line</param>
        /// <param name="binary_roi">[input]The binary roi of a text line</param>
        /// <param name="foreground_img">[input]The binary image of kmeans result</param>
        /// <param name="refine_img">[output]The final binary image by performing refining strategy on kmeans result</param>
        /// </summary>
        void RefineNewCCWithoutColor(const Mat& color_roi, const Mat& binary_roi, 
            const Mat& foreground_img, Mat& refine_img);

        /// refined the obtained new CCs from hole image (just removing noise and overlapped rects)
        /// <summary>
        /// <param name="color_roi">[input]The color roi of a text line</param>
        /// <param name="binary_roi">[input]The binary roi of a text line</param>
        /// <param name="foreground_img">[input]The binary image of kmeans result</param>
        /// <param name="refine_img">[output]The final binary image by performing refining strategy on kmeans result</param>
        /// <param name="small_area">[output]The smallest detected connected component</param>
        /// </summary>
        void RefineNewCCWithGeoFeature(const Mat& color_roi, const Mat& binary_roi, 
            const Mat& foreground_img, Mat& refine_img, int small_area);

        /// find SSP points of a binary_img
        /// <summary>
        /// <param name="color_img">[input]The source color image</param>
        /// <param name="binary_img">[input]The input binary image</param>
        /// <param name="cluster_pixel">[output]The output SSP and probable text pixels</param>
        /// </summary>
        void SSP(const Mat& color_img, const Mat& binary_img, vector<vector<Point>>& cluster_pixel);

        /// compute features (color and stroke) of a binary image
        /// <summary>
        /// <param name="color_img">[input]The source color image</param>
        /// <param name="binary_img">[input]The source binary image</param>
        /// <param name="stroke_mean">[output]The output mean stroke value of the binary image</param>
        /// <param name="color_mean">[output]The output mean color value of the binary image</param>
        /// </summary>
        void BinaryImgFeature(const Mat &color_img, const Mat &binary_img, Scalar &stroke_mean, Scalar &color_mean);

        /// extract connected components from a binary image and compute some CCs features
        /// <summary>
        /// <param name="binary_img">[input]The input of new binary image by kmeans</param>
        /// <param name="binary_img">[input]The input of original binary image by scene text detection</param>
        /// <param name="CCs">[output]The output extracted connected components</param>
        /// <param name="percent">[output]The overlap ratio between each CC and the original binary image, 
        ///                   the larger the value, the more probabe a true CC</param>
        /// <param name="textrects">[output]bounding rects of CCs</param>
        /// <param name="CC_img">[output]binary roi images of CCs</param>
        /// </summary>
        void CCExtraction(const Mat& binary_img, const Mat& origin_bimg, vector<vector<Point>>& CCs, 
            vector<float>& percent, vector<Rect>& textrects, vector<Mat>& CC_img);

        /// extract connected component pixels from a binary image, not used now
        /// <summary>
        /// <param name="binary_img">[input]The input of binary image</param>
        /// <param name="small_area">[input]The smallest area of a detected connected component</param>
        /// </summary>
        //void CCExtraction(const Mat& binary_img, vector<vector<Point>>& CCs, int small_area);

        /// extract stroke features of connected components
        /// <summary>
        /// <param name="CC_img">[input]binary image of a CC</param>
        /// <param name="stroke_mean">[output]mean stroke value of a CC</param>
        /// <param name="stroke_std">[output]std stroke value of a CC</param>
        /// </summary>
        void FeatureExtract(const Mat& CC_img, float& stroke_mean, float& stroke_std);

        /// compute hole image a binary image 
        /// <summary>
        /// <param name="binary_img">[input]binary image</param>
        /// <param name="hole_img">[output]corresponding hole image</param>
        /// </summary>
        bool GetHoleImg(const Mat &binary_img, Mat &hole_img);

        /// compute skeleton of a binary image 
        /// <summary>
        /// <param name="img">[input]binary image of a CC</param>
        /// <param name="skeleton">[output]skeleton image</param>
        /// </summary>
        bool GuoHallThinning(const Mat1b & img, Mat& skeleton);

        /// NiBlackBinarization http://stackoverflow.com/questions/9871084/niblack-thresholding
        /// <summary>
        /// <param name="src">[input]srcImg</param>
        /// <param name="binaryImg">[output]binayImg image</param>
        /// <param name="maxValue">thrershold value</param>
        /// <param name="type">[output]threshlod type</param>
        /// <param name="blockSize">local size</param>
        /// <param name="delta">it's k value</param>
        /// </summary>
        static void NiBlackBinarization(const Mat& src, Mat& binaryImg, double maxValue,
            int type, Size blockSize,double delta);
    };
}
#endif