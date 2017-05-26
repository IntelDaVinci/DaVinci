#ifndef __ROBUST_SCENE_TEXT_HPP__
#define __ROBUST_SCENE_TEXT_HPP__

#include "ErTree.hpp"
#include "opencv2/core/core.hpp"
#include <vector>
#include <deque>
#include <string>

//Paper:Robust Text Detection in Natural Scene Images
//Current object: modified er to mser, return a stuctured region(have parent,chidren), opencv mser only return points.

//To do
//single linkage clustering
//Bayesian

//need to confirm
//MSER has some difference with opencv MSER, need to root cause
//why widthDiff's param is minus?
namespace DaVinci
{
    struct ERFeatures
    {
        Rect  rect;//location
        double meanStroke;  //mean stroke width approximation of the whole region
        double stroke_std;  // stroke standard deviation of the whole region
        Scalar meanColor;//mean color
        //average difference of adjacent boundary pixels' gradient directions
    };

    class ClusterFeature
    {
    public:
        ClusterFeature(int erIndex1_, int erIndex2_,double spatialDistance_,double widthDiff_,double heightDiff_,double topAlignments_,double bottomAlignments_,\
            double colorDifference_, double strokeWidthDiff_,double distance_);
        ClusterFeature();

        int erIndex1;
        int erIndex2;
        double spatialDistance;
        double widthDiff;
        double heightDiff;
        double topAlignments;
        double bottomAlignments;
        double colorDifference;
        double strokeWidthDiff;
        //after calc
        double distance;

    };

    //Paper:Robust Text Detection in Natural Scene Images
    class RobustSceneText
    {
    public:
        RobustSceneText(const string& clusterParaPath="");
        void DetectText(InputArray colorImage,vector<Rect>&blackRects, vector<Rect>& whiteRects);
        bool static GuoHallThinning(const Mat1b & img, Mat& skeleton);
    private:
        //chapter 3.1
        void ProposeTextCandidate(InputArray image,vector<ErTree>& disconnectRegion,int thresholdDelta=1,int delta = 5,int minArea = 60,int maxArea=14400,float maxVariation = 0.25,float minDiversity =0.2);

        //need double check
        void ExtractFeatures(InputArray image, const vector<ErTree>& regions,vector<ERFeatures>& erFeatures);

        void ConstructClusterFeature(InputArray image,const vector<ERFeatures>& erfeatures, vector<ClusterFeature>& clusterFeatures);

        //erfeatures has been modified
        vector<Rect> SingleLinkageCluster( vector<ERFeatures>& erfeatures,vector<int>& labels);

        //chapter 3.1.2
        void RegularVariation(ErTree* root);

        //chapter 3.1.3,recursive
        ErTree*  LinearReduction(ErTree* root);

        //chapter 3.14
        void TreeAccumulation(ErTree* root, vector<ErTree*>& disconnectedSet);

        void DrawERRecursiveHelper(const Mat& image,const string& saveName,const vector<ErTree*>& subroot);

        void DrawERHelper(const Mat& img, const string& saveName,const vector<ErTree*>& ers);

        void GetDisconnectMSER(InputArray image,vector<ErTree>& regions,int thresholdDelta=1,int delta = 5,int minArea = 60,int maxArea=14400,float maxVariation = 0.25,float minDiversity =0.2);

        //currently is assigned directly;
        void GetClusterParams(const string& path);
        //void* linkChildren(ErTree* T, ErTree* c,bool linkChild);
        double spatialDistanceParam;
        double widthDiffParam;
        double heightDiffParam;
        double topAlignmentsParam;
        double bottomAlignmentsParam;
        double colorDifferenceParam;
        double strokeWidthDiffParam;
        double deltaParam;
        ER er;
        typedef int(*CompareFunc)(const void*, const void*, void*);
        static int ErFeatureCompFunc(const void* pCF1,const void* pCF2,void* data);

        class ClusterFeatureFunctor
        {
        public:
            bool operator()(const ClusterFeature& li,const ClusterFeature& lj)
            {
                if(li.distance<lj.distance)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
        };

        typedef struct TreeNode
        {
            struct TreeNode* parent;
            void* element;
            int rank;
        } ;

        template<typename T>
        int UnionFind(const vector<T>& srcData,vector<int>& label,CompareFunc compFun, void* dataForCompare);
    };
}
#endif