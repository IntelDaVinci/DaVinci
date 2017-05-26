/*
----------------------------------------------------------------------------------------

"Copyright 2014-2015 Intel Corporation.

The source code, information and material ("Material") contained herein is owned by Intel Corporation
or its suppliers or licensors, and title to such Material remains with Intel Corporation or its suppliers
or licensors. The Material contains proprietary information of Intel or its suppliers and licensors.
The Material is protected by worldwide copyright laws and treaty provisions. No part of the Material may
be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed or disclosed
in any way without Intel's prior express written permission. No license under any patent, copyright or
other intellectual property rights in the Material is granted to or conferred upon you, either expressly,
by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights
must be express and approved by Intel in writing.

Unless otherwise agreed by Intel in writing, you may not remove or alter this notice or any other notice
embedded in Materials by Intel or Intel's suppliers or licensors in any way."
-----------------------------------------------------------------------------------------
*/

#ifndef __ROBUST_SCENE_TEXT_HPP__
#define __ROBUST_SCENE_TEXT_HPP__
#include "ObjectRecognizeCaffe.h"
#include "MserTree.hpp"
#include "opencv2/core/core.hpp"
#include <vector>
#include <deque>
#include <string>
#include <memory>
#include "Binarization.hpp"
#define DaVinciRST
//#define FORMEDICA

#ifdef DaVinciRST
#include "boost/smart_ptr/shared_ptr.hpp"
#include "boost/thread/mutex.hpp"
#endif

//#define SAVEIMAGE

namespace DaVinci
{
#ifdef DaVinciRST
    using boost::shared_ptr;
#endif
    //Paper:Robust Text Detection in Natural Scene Images
    struct MSERFeature
    {
        Rect  rect;//location
        double strokeMean;  //mean stroke width approximation of the whole region
        double strokeStd;  // stroke standard deviation of the whole region
        Scalar colorMean;//mean color
        shared_ptr<vector<Point>> pPts;
    };

    enum  class RSTLANGUAGE
    {
        CHIENG = 0,
        ENG = 1,
        CHI = 2
    };


    class RSTParameter
    {
    public:
        RSTParameter(RSTLANGUAGE lang = RSTLANGUAGE::CHIENG);
        RSTLANGUAGE lang;
        bool clusterUseColor;
        bool clusterUseStroke;
        int minArea;
        int maxArea;
        double maxVariation;
        double minDiversity;
        int delta;
        double scale;//scale for speed, will lose recall if resize to small, keep 1.0 in DaVinci
        bool recoverLostChar;
        int preFilterMSERMaxHeight;
        int preFilterMSERMinHeight;
        int preFilterMSERMaxWidth;
        int preFilterMSERMinWidth;
        int preFilterMSERMaxArea;
        float postFilterTextLineAspectRatio;
        int charHighProb;//the full score is 100, if the classifier gives higher score than this, it's will be a real char 
        int charLowProb;//if the classifier gives higher scroe than this but lower than charHighProb, it's will be a protential char, need to  check whether if it's in a textline enlarge rect
        bool useGradientChannel;
        BinarizationType binaryType;

    };

    //Paper:Robust Text Detection in Natural Scene Images
    /*Note:
    contour reserved[0] saves the variance of the mser *10000, after regularization, this value will multiplied by -1 if it's belonged to MSER-
    contour reserved[1] saves mser's children number
    contour reserved[2] saves probability to be a char * 100
    */
    class RobustSceneText
    {
    public:
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="para">parameters</param>
        RobustSceneText(const RSTParameter& para = RSTParameter());

        /// <summary>
        /// Detect text rects in acolor or gray image,note that black rects and white rects may have union part
        /// </summary>
        /// <param name="img">The source image</param>
        /// <param name="textRects">[output] text rects</param>
        void DetectText(const Mat& img, vector<Rect>&textRects);

        /// <summary>
        /// Detect text rects in a color or gray image and return binary image for ocr recognizer
        /// use binarization method by HuiWu (adaptive kmeans).
        /// </summary>
        /// <param name="img">[input]The source image</param>
        /// <param name="textRects">[output]boundingrect of detected textlines</param>
        /// <param name="binaryMat">[output]binary text lines for recognition</param>
        //void DetectTextWithDetail(const Mat& img, vector<Mat>& blackBinaryImages, vector<Mat>& whiteBinaryImages, vector<Rect>& blackRects, vector<Rect>& whiteRects);


        /// <summary>
        /// Debug function of "DetectTextWithDetail" and output results of each procedure
        /// use binarization method by HuiWu (adaptive kmeans).
        /// </summary>
        /// <param name="img">[input]The source image</param>
        /// <param name="textRects">[output]boundingrect of detected textlines from RST</param>
        /// <param name="binary_line">[output]put binary text lines in one whole image</param>
        void DetectTextWithDetail(const Mat& img, vector<Rect>& textRects, vector<Mat>& binary_line);

        /// <summary>
        ///  Get RST parameters
        /// </summary>
        /// <returns>parameters</returns>
        RSTParameter& GetParams();

    private:
        /// <summary>
        /// Detect text  in an image
        /// </summary>
        /// <param name="img">The source image</param>
        /// <param name="textRects">[output] text rects</param>
        /// <param name="textlines">[output]  text lines</param>
        void DetectText(const Mat& img, vector<Rect>& textRects, vector<vector<CvContour*>>& textlines);

        /// <summary>
        /// adjust rects' position based on scale and padding
        /// </summary>
        /// <param name="textRects">[input & output]text rects</param>
        /// <param name="rows"> image rows </param>
        /// <param name="whiteRects">image cols</param>
        void PostProcess(vector<Rect>& textRects, int rows, int cols);

        /// <summary>
        ///Convert RBG text image to binary image
        /// we have character info,  then we can use the color infomation to detect each pixel is belong to character
        /// </summary>
        /// <param name="colorImage">The source image</param>
        /// <param name="textRects">text line rectangles</param>
        /// <param name="pts">charater points</param>
        /// <returns>binary images corresponding to text lines</returns>
        //vector<Mat > ConvertRGBTextToBinary(const Mat& colorImage, const vector<Rect>& textRects, const vector<vector<Point2i>>& pts);

        /// <summary>
        /// Detect character in an image
        /// </summary>
        /// <param name="img">The source image</param>
        /// <param name="charCandidates"> char candidates</param>
        /// <param name="realChars">real chars aftring filtering cha candidates</param>
        void DetectCharacter(const Mat& img, vector<CvContour*>& charCandidates, vector<CvContour*>& realChars);

        /// <summary>
        /// propose character candidate by MSER tree
        /// </summary>
        /// <param name="src">The source image</param>
        /// <param name="charCandidates"> text proposals</param>
        void ProposeTextCandidate(const Mat& src, vector<CvContour*>& charCandidates);

        /// <summary>
        /// eliminate false positive character candidate by CNN
        /// </summary>
        /// <param name="gray">The source image</param>
        /// <param name="rects">text proposal regions</param>
        /// <param name="resizeScale">the CNN input size</param>
        /// <returns>the resut correspondes to rects</returns>
        vector<float> EliminateFalseCharacter(const Mat& gray, const vector<Rect>& rects, Size resizeScale = Size(32, 32));

        /// <summary>
        /// extract mser features, such as mean stroke, mean color
        /// </summary>
        /// <param name="image">The source image</param>
        /// <param name="regions">the mesr(text) regions</param>
        /// <param name="erFeatures">[output]the output mser features</param>
        void ExtractFeatures(const Mat& image, const vector<CvContour*>& regions, vector<MSERFeature>& erFeatures);

        /// <summary>
        /// extract points from contours
        /// </summary>
        /// <param name="regions">The source image</param>
        /// <param name="MSERFeatures">[output]only copy points here</param>
        void ExtractPoints(const vector<CvContour*>& regions, vector<MSERFeature>& MSERFeatures);

        /// <summary>
        /// Single linkage cluster, merge character to text line
        /// </summary>
        /// <param name="erfeatures">The source mser features</param>
        /// <param name="texts">the orginal mser</param>
        /// <param name="clusters">clusters[n] includes all the msers belong to class n</param>
        /// <param name="clusterLabels">clusterLabels[n] includes all the MSERFeature index belong to class n</param>
        /// <returns>text lines rect</returns>
        vector<Rect> SingleLinkageCluster(vector<MSERFeature>& erfeatures, vector<CvContour*>&texts, vector<vector<CvContour*>>& clusters, vector<vector<int>>& clusterLabels);

        /// <summary>
        /// recover some lost characters
        /// due to character classifier's issue, some i or j at the beginning or end  are lost.
        /// </summary>
        /// <param name="img">the source image</param>
        /// <param name="allProposals">all text proposals</param>
        /// <param name="mserFeautures">mser featuers have been extracted in SLC, the size does not equal to size of all proposals </param>
        /// <param name="clusterLabels">clusterlabes[n] represlent a text line cluster, the value represenst the index in mserFeatuers</param>
        /// <param name="rects">the output rects</param>
        /// <param name="root">mser tree root node</param>
        /// <param name="root">mser tree root node</param>
        /// <returns>mser tree root node</returns>
        void RecoverLostCharacter(const Mat&img, const vector<CvContour*>& allProposals, const  vector<MSERFeature>& mserFeautures, const vector<vector<int>>& clusterLabels, vector<Rect>& rects, vector<vector<CvContour*>>&clusters);

        /// <summary>
        ///union ratio of two rects,union(large rect,small rect)/samll rect
        /// </summary>
        /// <param name="largeRect">large rect</param>
        /// <param name="smallRect">small rect</param>
        /// <returns>union ratio</returns>
        double TextInterUnio(const Rect& largeRect, const Rect& smallRect);

        /// <summary>
        ///prefilter mser by some conditions
        /// </summary>
        /// <param name="rect">mser rect info</param>
        /// <param name="maxHeight">maxHeight</param>
        /// <param name="minHeight">minHeight</param>
        /// <param name="maxWidth">maxWidth</param>
        /// <param name="minWidth">minWidth</param>
        /// <param name="maxArea">maxArea</param>
        /// <returns>union ratio</returns>
        bool PreFilterMSER(const Rect& rect, int maxHeight, int minHeight, int maxWidth, int minWidth, int maxArea);

        /// <summary>
        ///remove useless chars by intersection region
        /// </summary>
        /// <param name="charCandidates">char candidates</param>
        void RemoveUselessChars(vector<CvContour*>& charCandidates);

        /// <summary>
        ///remove useless text lines by intersection region
        /// </summary>
        /// <param name="textRects">text line rects</param>
        /// <param name="textClusters">cluster of chars</param>
        void RemoveUselessRect(vector<Rect>&textRects, vector<vector<CvContour*>>& textClusters);

        /// <summary>
        ///remove useless text lines by Beyesian method
        /// </summary>
        /// <param name="charCandidates">text line contours</param>
        /// <param name="textRects">text line rects</param>
        /// <param name="textlines">the output refined text lines</param>
        //void FilterTextLineByBeyesian(const Mat& srcColorImage, const vector<CvContour*>& charCandidates,vector<Rect>& textRects, vector<vector<CvContour*>>& textlines);

        //void FilterTextLineByBeyesian(const Mat& srcColorImage, const vector<CvContour*>& charCandidates,vector<Rect>& textRects, vector<vector<CvContour*>>& textlines);

        /// <summary>
        ///train a classifier for character verification and used by Beyesian function
        /// </summary>
        /// <param name="charCandidates">text line contours</param>
        /// <param name="textRects">text line rects</param>
        /// <param name="textlines">the output refined text lines</param>
        //void ClassifierForChars();


        Rect PadRect(const Rect& rect, int rows, int cols);

        /// <summary>
        /// Get char candidates by mser tree prunning
        /// </summary>
        /// <param name="msers">all mesers</param>
        /// <param name="charCandidates">char candidates</param>
        /// <param name="isBlack">mser+ or mser-</param>
        void PruneTree(vector<CvContour*>&  msers, vector<CvContour*>& charCandidates, bool isBlack);

        /// <summary>
        ///Get the root node of every mser tree
        /// </summary>
        /// <param name="msers">msers</param>
        /// <param name="roots">[output] mser tree roots</param>
        void GetRootNode(const vector<CvContour*>& msers, vector<CvContour*>& roots);

        /// <summary>
        ///Extracted msers
        /// </summary>
        /// <param name="img">msers</param>
        /// <param name="blackMsers">[output] msers</param>
        /// <param name="whiteMsers">[output] msers</param>
        void GetMSER(const Mat& img, vector<CvContour*>& blackMsers, vector<CvContour*>& whiteMsers);

        /// <summary>
        /// Regualr mser's variation based on its apect ratio ,chapter 3.1.2
        /// </summary>
        /// <param name="msers">msers</param>
        /// <param name="isBlack">if isBlack is false, multipy the variation by minus one for later usage</param>
        void RegVar(vector<CvContour*>& msers, bool isBlack);

        /// <summary>
        /// calculate mser's child number,the info will save in contour
        /// </summary>
        /// <param name="msers">[in][output]msers</param>
        void CalcChildNum(vector<CvContour*>&  msers);

        /// <summary>
        /// Linear recution mser tree, to make every node not have exact one child,recursive, chapter 3.1.3
        /// </summary>
        /// <param name="root">mser tree root node</param>
        /// <returns>mser tree root node</returns>
        CvContour* LinearReduction(CvContour* root);

        /// <summary>
        /// Based on character can be included in another character, delete higher vadiation nodes in parent&child pair, chapter 3.1.3
        /// </summary>
        /// <param name="root">mser tree root node</param>
        /// <returns>disconnected mser tree node</returns>
        vector<CvContour*> TreeAccumulation(CvContour* root);

        class MSERFeatureFunc
        {
        public:
            bool operator()(const MSERFeature& f1, const MSERFeature& f2);
        };

        class MSERFeatureFuncRecover
        {
        public:
            bool operator()(const MSERFeature& f1, const MSERFeature& f2);
        };


#ifdef DaVinciRST
        static boost::mutex classifierLock;
#endif
#ifdef DAVINCI_LIB_STATIC
    public:
#endif
        void SortRects(vector<Rect>& rects, vector<vector<CvContour*>>& textlines);
    private:
        /// charater/noncharacter Classifier
        shared_ptr<caffe::CaffeNet> characterClassifier;
        MSERTree msertree;
        RSTParameter param;//prameters
        RSTParameter tempParam;
    };
}
#endif