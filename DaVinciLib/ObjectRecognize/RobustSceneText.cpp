#include "RobustSceneText.hpp"
#include <queue>
#include <stack>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv/cv.h"
using namespace cv;
using namespace std;

namespace DaVinci
{
    void RobustSceneText::ProposeTextCandidate(InputArray image,vector<ErTree>& disconnectRegion,int thresholdDelta,int delta ,int minArea,int maxArea,float maxVariation,float minDiversity)
    {
        disconnectRegion.clear();
        vector<ErTree> blackDConnectRegion;
        Mat gray;
        if(image.channels()==3)
        {
            cvtColor(image,gray,CV_BGR2GRAY);
        }
        else
        {
            gray = image.getMat();
        }
        GetDisconnectMSER(gray,blackDConnectRegion,thresholdDelta,delta,minArea,maxArea,maxVariation,minDiversity);

        vector<ErTree> whiteDConnectRegion;
        disconnectRegion = blackDConnectRegion;
        //Mat whiteGray = 255-gray;
        //getDisconnectMSER(whiteGray,whiteDConnectRegion,thresholdDelta,delta,minArea,maxArea,maxVariation,minDiversity);

        //disconnectRegion.insert(disconnectRegion.end(),whiteDConnectRegion.begin(),whiteDConnectRegion.end());
#ifdef _DEBUG
        vector<ErTree*> wholeDConnectRegion(blackDConnectRegion.size()+whiteDConnectRegion.size());
        for(size_t i=0;i<blackDConnectRegion.size();++i)
        {
            wholeDConnectRegion[i]= &blackDConnectRegion[i];
        }
        int tempn = blackDConnectRegion.size();
        for(size_t i=0;i<whiteDConnectRegion.size();++i)
        {
            wholeDConnectRegion[tempn++]=& whiteDConnectRegion[i];
        }
        DrawERHelper(image.getMat(),"DebugWholeDisconnect.jpg",wholeDConnectRegion);
#endif

    }


    void RobustSceneText::GetDisconnectMSER(InputArray image,vector<ErTree>& regions,int thresholdDelta,int delta,int minArea,int maxArea,float maxVariation,float minDiversity)
    {
        assert(image.channels()==1);
        Mat gray = image.getMat();
        er.Extract(gray,regions,thresholdDelta);
        er.MarkStable(&regions[0],delta,minArea,maxArea,maxVariation,minDiversity);
        vector<ErTree*> subroot;
        er.ConstructMsErTree(&regions[0],subroot);
        for(size_t i=0;i<subroot.size();++i)
        {
            RegularVariation(subroot[i]);
        }

        vector<ErTree*> linearSubRoot;

        for(size_t i=0;i<subroot.size();++i)
        {
            ErTree* rootTemp = LinearReduction(subroot[i]);
            linearSubRoot.push_back(rootTemp);

        }

        vector<ErTree*> disconnectSet;
        for(size_t i=0;i<linearSubRoot.size();++i)
        {
            vector<ErTree*>  temp;
            TreeAccumulation(linearSubRoot[i],temp);
            disconnectSet.insert(disconnectSet.end(),temp.begin(),temp.end());
        }
        //erased aspect ratio is too large and area is also large one
        vector<bool> isErased(disconnectSet.size());
        vector<ErTree*> disconnectSetTemp;
        for(size_t i=0; i<disconnectSet.size();++i)//trick one,also we can add stroke variance or something like that
        {
            double aspectRatio = (double)disconnectSet[i]->rect.width/disconnectSet[i]->rect.height;
            int area = disconnectSet[i]->rect.area();
            if(area>10000||aspectRatio>10||aspectRatio<0.143||(area>6000&&(aspectRatio>=5||aspectRatio<=0.2)))
            {
            }
            else
            {
                disconnectSetTemp.push_back(disconnectSet[i]);
            }
        }
        disconnectSet = disconnectSetTemp;

#ifdef _DEBUG

        Mat img;
        cvtColor(gray,img,CV_GRAY2BGR);
        //subroot.insert(subroot.end(),subrootTemp.begin(),subrootTemp.end());
        //linearSubRoot.insert(linearSubRoot.end(),linearSubRootTemp.begin(),linearSubRootTemp.end());
        //disconnectSet.insert(disconnectSet.end(),disconnectSetTemp.begin(),disconnectSetTemp.end());
        DrawERRecursiveHelper(img,"debugMser.jpg",subroot);
        DrawERRecursiveHelper(img,"debugLinear.jpg",linearSubRoot);
        DrawERHelper(img,"debugDisconnect.jpg",disconnectSet);
        DrawERHelper(img,"debugSubRoot.jpg",subroot);
        MSER mser(delta,minArea,maxArea,maxVariation,minDiversity);
        vector<vector<Point2i>> ptsVec;
        Mat drawImage2 = img.clone();
        mser(gray,ptsVec);
        for(size_t i=0;i<ptsVec.size();++i)
        {
            Rect rect = boundingRect(ptsVec[i]);
            rectangle(drawImage2,rect,Scalar(0,0,255),2);
        }
        imwrite("debugOpencvMSER.jpg",drawImage2);
#endif
        vector<ErTree> result(disconnectSet.size());
        for(size_t i=0;i<disconnectSet.size();++i)
        {
            result[i] = *(disconnectSet[i]);
        }
        regions=result;
    }

    //need double check
    void RobustSceneText::ExtractFeatures(InputArray image,const vector<ErTree>& regions,vector<ERFeatures>& erFeatures)
    {
        erFeatures.clear();
        assert(3==image.channels());
        assert(!regions.empty());
        assert(erFeatures.empty());
        erFeatures.reserve(regions.size());
        Mat img = image.getMat();
        Mat gray;
        cvtColor(img,gray,CV_BGR2GRAY);
        for(size_t i = 0;i<regions.size();++i)
        {
            const ErTree *stat = &(regions[i]);
            Rect imgRec = Rect(Point(stat->rect.x,stat->rect.y),Point(stat->rect.br().x+2,stat->rect.br().y+2));
            Mat region(imgRec.height,imgRec.width,CV_8UC1);//as opencv said, must be 2 pixels wider and 2 pixels taller.
            region = Scalar(0);
            int newMaskVal = 255;
            int flags = 4 + (newMaskVal << 8) + FLOODFILL_FIXED_RANGE + FLOODFILL_MASK_ONLY;
            Rect rect;

            floodFill( gray(Rect(Point(stat->rect.x,stat->rect.y),Point(stat->rect.br().x,stat->rect.br().y))),
                region, Point(stat->pixel%gray.cols - stat->rect.x, stat->pixel/gray.cols - stat->rect.y),
                Scalar(255), &rect, Scalar(stat->level), Scalar(0), flags );

            rect.width += 2;
            rect.height += 2;
            Mat rect_mask = region(Rect(1,1,stat->rect.width,stat->rect.height));
            Scalar colorMean,colorStd;
            meanStdDev(img(stat->rect), colorMean, colorStd, rect_mask);//get one feature

            Mat bw =rect_mask.clone();
            copyMakeBorder(bw, bw, 5, 5, 5, 5, BORDER_CONSTANT, Scalar(0));//first make border,in case the border are all 255s
            Mat tmp;
            distanceTransform(bw, tmp, CV_DIST_L1,3); //L1 gives distance in round integers while L2 floats

            Mat skeleton = Mat::zeros(bw.size(),CV_8UC1);
            GuoHallThinning(bw,skeleton);
            Mat mask;
            skeleton(Rect(5,5,bw.cols-10,bw.rows-10)).copyTo(mask);
            bw(Rect(5,5,bw.cols-10,bw.rows-10)).copyTo(bw);
            tmp(Rect(5,5,tmp.cols-10,tmp.rows-10)).copyTo(tmp);
            Scalar strokeMean,storkeStd;
            meanStdDev(tmp,strokeMean,storkeStd,mask);

            ERFeatures feat;
            feat.meanColor = colorMean;
            feat.meanStroke = strokeMean[0];
            feat.stroke_std = storkeStd[0];
            feat.rect = stat->rect;
            erFeatures.push_back(feat);
        }
    }

    vector<Rect> RobustSceneText::SingleLinkageCluster( vector<ERFeatures>& erfeatures,vector<int>& labels)
    {
        labels.clear();
        double strokeWidthVarThreshold = 10.0;//tricky one
        vector<ERFeatures> temp;
        for(size_t i=0;i<erfeatures.size();++i)
        {
            if(erfeatures[i].stroke_std<strokeWidthVarThreshold)
                temp.push_back(erfeatures[i]);
        }
        erfeatures = temp;
        int n=UnionFind(temp,labels,ErFeatureCompFunc,NULL);
        vector<Rect> rects(n);
        vector<bool> bInit(n);
        for(int i=0;i<n;++i)
        {
            bInit[i] = false;
        }

        for(size_t i=0;i<temp.size();++i)
        {
            int label = labels[i];
            if(bInit[label]==false)
            {
                rects[label] = temp[i].rect;
                bInit[label] = true;
            }
            else
            {
                int xTL = std::min(rects[label].x,temp[i].rect.x);
                int yTL = std::min(rects[label].y,temp[i].rect.y);
                int xBR = std::max(rects[label].x+rects[label].width,temp[i].rect.x+temp[i].rect.width);
                int yBR = std::max(rects[label].y+rects[label].height,temp[i].rect.y+temp[i].rect.height);
                rects[label] = Rect(Point(xTL,yTL),Point(xBR,yBR));
            }
        }
        return rects;
    }

    void RobustSceneText::RegularVariation(ErTree* root)
    {
        if(root==NULL)
            return;
        stack<ErTree*> stackNodes;
        stackNodes.push(root);
        float aspectRatio = 0.0;
        while(stackNodes.size()>0)
        {
            ErTree* currentNode = stackNodes.top();
            stackNodes.pop();
            if(currentNode->rect.height<=0)
                currentNode->variation=-100;
            else
            {
                aspectRatio = ((float)currentNode->rect.width)/currentNode->rect.height;
                //formula 2 in paper
                if(aspectRatio>1.2)
                {
                    currentNode->variation += (float)(0.01*(aspectRatio-1.2));
                }
                else if(aspectRatio<0.3)
                {
                    currentNode->variation +=(float)( 0.35*(0.7-aspectRatio));
                }
            }
            assert(currentNode->variation>-10000);
            for(ErTree* child = currentNode->child;child!=NULL;child=child->next)
            {
                stackNodes.push(child);
            }
        }
    }

    //void* RobustSceneText::linkChildren(ErTree* T, ErTree* c,bool linkChild)
    //{
    //    if(linkChild)
    //    {
    //        ErTree* Tchild = T->child;
    //        for(ErTree*cChild=cChild->child;cChild!=NULL;cChild=cChild->next)
    //        {
    //            Tchild = cChild;
    //            Tchild = Tchild->next;
    //        }
    //    }
    //    else
    //    {
    //        ErTree* lastNoempty
    //    }
    //}

    ErTree*  RobustSceneText::LinearReduction(ErTree* root)//need double check
    {
        //clac number of children
        int childNum = 0;
        for(ErTree* child = root->child;child!=NULL;child=child->next)
        {
            ++childNum;
        }
        if(0==childNum)
        {
            return root;
        }
        else if(1==childNum)
        {
            ErTree* c = LinearReduction(root->child);
            if(root->variation<=c->variation)
            {
                root->child = c->child;//set root's child
                for(ErTree*cChild=c->child;cChild!=NULL;cChild=cChild->next)//link all the c's children to root,reset their root
                {
                    cChild->parent = root;
                }
                //root->child=c;
                return root;
            }
            else
            {
                return c;
            }
        }
        else//childNum>=2
        {
            vector<ErTree*> vecErTreeP;
            for(ErTree* child = root->child;child!=NULL;child=child->next)
            {
                ErTree* c = LinearReduction(child);
                vecErTreeP.push_back(c);
            }
            //link vecErTreeP to root;
            ErTree* rootChild = root->child;
            rootChild = vecErTreeP[0];
            for(size_t i=0;i<vecErTreeP.size()-2;++i)//reset their pre and next;
            {
                vecErTreeP[i]->next= vecErTreeP[i+1];
                vecErTreeP[i+1]->prev = vecErTreeP[i];
            }
            for(size_t i=0;i<vecErTreeP.size()-1;++i)//reset their parents
            {
                vecErTreeP[i]->parent=root;
            }
            return root;
        }
    }

    void ER::MarkStable(ErTree* root,int delta,int minArea,int maxArea,float maxVariation ,float minDiversity )
    {
        DecideStable(root,delta,minArea, maxArea, maxVariation);
        DecideStableByDiversity(root,minDiversity);
    }
    ErTree* ER::GetRoot()
    {
        assert(regions->size()>0);
        return &((*regions)[0]);
    }
    void ER::ConstructMsErTree(ErTree* root, vector<ErTree*>& subroot)
    {
        assert(root!=NULL);
        //for keyboard
        //stack<ErTree*> ErPStack;
        //ErPStack.push(root);
        //while(!ErPStack.empty())
        //{
        //    ErTree* top = ErPStack.top();
        //    ErPStack.pop();
        //    if(root->isStable&&root->rect.area()>6000)
        //    {
        //        root->isStable = false;
        //    }
        //    for(ErTree* c = top->child;c!=NULL;c=c->next)
        //    {
        //        ErPStack.push(c);
        //    }
        //}
        int nchild = 0;
        for(ErTree* c = root->child;c!=NULL;c=c->next)
        {
            ++nchild;
        }
        if(0==nchild)
        {
            if(root->isStable)
            {
                subroot.push_back(root);
            }
            return;
        }
        //child num>0
        for(ErTree* c = root->child;c!=NULL;c=c->next)
        {
            vector<ErTree*> temp;
            ConstructMsErTree(c,temp);
            if(temp.size()>0)
            {
                subroot.insert(subroot.end(),temp.begin(),temp.end());
            }
        }
        if(root->isStable&&subroot.size()>0)//link to root
        {
            root->child=subroot[0];
            subroot[0]->parent = root;
            subroot[0]->next = NULL;
            subroot[0]->prev = NULL;
            subroot[subroot.size()-1]->next = NULL;
            for(size_t i=1;i<subroot.size();++i)
            {
                subroot[i]->parent = root;
                subroot[i-1]->next=subroot[i];
                subroot[i]->prev = subroot[i-1];
            }
            subroot.clear();
            subroot.push_back(root);
            return;
        }
        else if(root->isStable&&subroot.size()==0)
        {
            subroot.push_back(root);
            return;
        }
        else 
        {
            return;
        }

    }

    void RobustSceneText::TreeAccumulation(ErTree* root, vector<ErTree*>& disconnectedSet)
    {
        //clac children;
        int nChildren = 0;
        for(ErTree*c=root->child;c!=NULL;c=c->next)
        {
            ++nChildren;
        }
        if(nChildren>=2)
        {
            vector<ErTree*> children;
            for(ErTree*c=root->child;c!=NULL;c=c->next)
            {
                vector<ErTree*> temp;
                TreeAccumulation(c,temp);
                children.insert(children.end(),temp.begin(),temp.end());
            }
            float minVar = children[0]->variation;
            for(size_t i=0;i<children.size();++i)
            {
                if(children[i]->variation<minVar)
                {
                    minVar = children[i]->variation;
                }
            }
            if(root->variation <= minVar)
            {
                //discard T's children
                disconnectedSet.push_back(root);
                return;
            }
            else
            {
                disconnectedSet = children;
                return;
            }
        }
        else
        {
            disconnectedSet.push_back(root);
            return;
        }
    }

    void RobustSceneText::DrawERRecursiveHelper(const Mat& image,const string& saveName,const vector<ErTree*>& subroot)
    {
        Mat drawImage = image.clone();
        stack<ErTree*> stackER;
        vector<ErTree*> allER;
        for(size_t i=0;i<subroot.size();++i)
        {
            stackER.push(subroot[i]);
            while(stackER.size()!=0)
            {
                ErTree *top = stackER.top();
                stackER.pop();
                allER.push_back(top);
                for(ErTree* c=top->child;c!=NULL;c=c->next)
                {
                    stackER.push(c);
                }
            }
        }
        for(size_t i=0;i<allER.size();++i)
        {
            rectangle(drawImage,allER[i]->rect,Scalar(51,249,40),2);
        }
        imwrite(saveName,drawImage);
    }

    void RobustSceneText::DrawERHelper(const Mat& img, const string& saveName,const vector<ErTree*>& ers)
    {
        Mat drawImage = img.clone();
        for(size_t i=0;i<ers.size();++i)
        {
            rectangle(drawImage,ers[i]->rect,Scalar(51,249,40),2);
        }
        imwrite(saveName,drawImage);
    }

    ClusterFeature::ClusterFeature(int erIndex1_, int erIndex2_,double spatialDistance_,double widthDiff_,double heightDiff_,double topAlignments_,double bottomAlignments_,\
        double colorDifference_, double strokeWidthDiff_,double distance_)
    {
        erIndex1 = erIndex1_;
        erIndex2 = erIndex2_;
        spatialDistance = spatialDistance_;
        widthDiff = widthDiff_;
        heightDiff = heightDiff_;
        topAlignments = topAlignments_;
        bottomAlignments = bottomAlignments_;
        colorDifference = colorDifference_;
        strokeWidthDiff = strokeWidthDiff_;
        distance = distance_;
    }

    RobustSceneText::RobustSceneText(const string& clusterParaPath)
    {
        GetClusterParams(clusterParaPath);
    }

    void RobustSceneText::GetClusterParams(const string& path)
    {
        spatialDistanceParam = 6.6395;
        widthDiffParam = -0.756415;
        heightDiffParam = 1.520538;
        topAlignmentsParam =4.4788746 ;
        bottomAlignmentsParam = 4.850933;
        colorDifferenceParam=0.04525285;
        strokeWidthDiffParam=0.6514;
        deltaParam=9.207139;
    }


    template<typename T>
    int RobustSceneText::UnionFind(const vector<T>& srcData,vector<int>& label,CompareFunc compFun, void* dataForCompare)
    {
        int srcDataSize= srcData.size();
        if(srcDataSize<=0)
            return 0;
        TreeNode* node=new TreeNode[srcDataSize];
        for (int i = 0; i < srcDataSize; ++i)
        {
            node[i].parent = 0;
            node[i].element = const_cast<T*>(&srcData[i]);
            node[i].rank = 0;
        }
        for (int i = 0; i <srcDataSize; ++i)
        {
            if (!node[i].element)
                continue;
            TreeNode* root = node + i;
            while (root->parent)
                root = root->parent;

            for (int j = 0; j < srcDataSize; ++j)
            {
                if( i != j && node[j].element && compFun(node[i].element, node[j].element, dataForCompare))
                {
                    TreeNode* root2 = node + j;

                    while(root2->parent)
                        root2 = root2->parent;

                    if(root2 != root)
                    {
                        if(root->rank > root2->rank)
                            root2->parent = root;
                        else
                        {
                            root->parent = root2;
                            root2->rank += root->rank == root2->rank;//wenhua not clear, why add 1
                            root = root2;
                        }

                        /* compress path from node2 to the root: */
                        TreeNode* node2 = node + j;
                        while(node2->parent)
                        {
                            TreeNode* temp = node2;
                            node2 = node2->parent;
                            temp->parent = root;
                        }

                        /* compress path from node to the root: */
                        node2 = node + i;
                        while(node2->parent)
                        {
                            TreeNode* temp = node2;
                            node2 = node2->parent;
                            temp->parent = root;
                        }
                    }
                }
            }
        }
        label.clear();

        int class_idx = 0;
        for(int i = 0; i <srcDataSize; ++i)
        {
            int j = -1;
            TreeNode* node1 = node + i;
            if(node1->element)
            {
                while(node1->parent)
                    node1 = node1->parent;
                if(node1->rank >= 0)
                    node1->rank = ~class_idx++;//node1->rank will be negative after this operation
                j = ~node1->rank;
            }
            label.push_back(j);
        }
        delete[] node;
        return class_idx;
    }

    int RobustSceneText::ErFeatureCompFunc(const void* pCF1,const void* pCF2,void* data)
    {
        const ERFeatures &li = *((ERFeatures*)pCF1);
        const ERFeatures & lj = *((ERFeatures*)pCF2);
        using std::abs;
        double spatialDistance=0;
        if(li.rect.x<lj.rect.x)
        {
            spatialDistance =   double(abs(lj.rect.x-li.rect.x-li.rect.width))/max(li.rect.width,lj.rect.width);
        }
        else
        {
            spatialDistance =   double(abs(li.rect.x-lj.rect.x-lj.rect.width))/max(li.rect.width,lj.rect.width);
        }
        double widthDiff=(double)(abs(li.rect.width-lj.rect.width))/max(li.rect.width,lj.rect.width);
        double heightDiff=(double)(abs(li.rect.height-lj.rect.height))/max(li.rect.height,lj.rect.height);
        double topAlignments=(double)(atan2(abs(li.rect.y-lj.rect.y),abs(li.rect.x+li.rect.width/2-lj.rect.x-lj.rect.width/2)));
        double bottomeAlignments=(double)(atan2(abs(li.rect.y+li.rect.height-lj.rect.y-lj.rect.height),abs(li.rect.x+li.rect.width/2-lj.rect.x-lj.rect.width/2)));
        double colorDifference=sqrt((li.meanColor[0]-lj.meanColor[0])*(li.meanColor[0]-lj.meanColor[0])+ (li.meanColor[1]-lj.meanColor[1])*(li.meanColor[1]-lj.meanColor[1])\
            +(li.meanColor[2]-lj.meanColor[2])*(li.meanColor[2]-lj.meanColor[2]));
        colorDifference/=255.0;
        double strokeWidthDiff=(double)(abs(li.meanStroke-lj.meanStroke))/max(li.meanStroke,lj.meanStroke);
        /*  double distance = spatialDistance*6.6395 + widthDiff*(-0.756415) +heightDiff*(1.520538) + topAlignments*(4.4788746)+\
        bottomeAlignments*4.850933 + colorDifference*0.04525285 + strokeWidthDiff*0.6514 - 9.207139;*/

        double spatialDistanceParamHandTune = 2.8;//trick one by wenhua
        double distance = spatialDistance*3.78*spatialDistanceParamHandTune + widthDiff*(9.84) +heightDiff*(2.12) + topAlignments*(1.25)+\
            bottomeAlignments*21.71 + colorDifference*(1.22) + strokeWidthDiff*21.57 - 21.52;
        if(distance<0)
            return 1;
        else
            return 0;
    }

    bool RobustSceneText::GuoHallThinning(const Mat1b & img, Mat& skeleton)
    {

        uchar* img_ptr = img.data;
        uchar* skel_ptr = skeleton.data;

        for (int row = 0; row < img.rows; ++row)
        {
            for (int col = 0; col < img.cols; ++col)
            {
                if (*img_ptr)
                {
                    int key = row * img.cols + col;
                    if ((col > 0 && *img_ptr != img.data[key - 1]) ||
                        (col < img.cols-1 && *img_ptr != img.data[key + 1]) ||
                        (row > 0 && *img_ptr != img.data[key - img.cols]) ||
                        (row < img.rows-1 && *img_ptr != img.data[key + img.cols]))
                    {
                        *skel_ptr = 255;
                    }
                    else
                    {
                        *skel_ptr = 128;
                    }
                }
                img_ptr++;
                skel_ptr++;
            }
        }

        int max_iters = 10000;
        int niters = 0;
        bool changed = false;

        /// list of keys to set to 0 at the end of the iteration
        deque<int> cols_to_set;
        deque<int> rows_to_set;

        while (changed && niters < max_iters)
        {
            changed = false;
            for (unsigned short iter = 0; iter < 2; ++iter)
            {
                uchar *skeleton_ptr = skeleton.data;
                rows_to_set.clear();
                cols_to_set.clear();
                // for each point in skeleton, check if it needs to be changed
                for (int row = 0; row < skeleton.rows; ++row)
                {
                    for (int col = 0; col < skeleton.cols; ++col)
                    {
                        if (*skeleton_ptr++ == 255)
                        {
                            bool p2, p3, p4, p5, p6, p7, p8, p9;
                            p2 = (skeleton.data[(row-1) * skeleton.cols + col]) > 0;
                            p3 = (skeleton.data[(row-1) * skeleton.cols + col+1]) > 0;
                            p4 = (skeleton.data[row     * skeleton.cols + col+1]) > 0;
                            p5 = (skeleton.data[(row+1) * skeleton.cols + col+1]) > 0;
                            p6 = (skeleton.data[(row+1) * skeleton.cols + col]) > 0;
                            p7 = (skeleton.data[(row+1) * skeleton.cols + col-1]) > 0;
                            p8 = (skeleton.data[row     * skeleton.cols + col-1]) > 0;
                            p9 = (skeleton.data[(row-1) * skeleton.cols + col-1]) > 0;

                            int C  = (!p2 & (p3 | p4)) + (!p4 & (p5 | p6)) +
                                (!p6 & (p7 | p8)) + (!p8 & (p9 | p2));
                            int N1 = (p9 | p2) + (p3 | p4) + (p5 | p6) + (p7 | p8);
                            int N2 = (p2 | p3) + (p4 | p5) + (p6 | p7) + (p8 | p9);
                            int N  = N1 < N2 ? N1 : N2;
                            int m  = iter == 0 ? ((p6 | p7 | !p9) & p8) : ((p2 | p3 | !p5) & p4);

                            if ((C == 1) && (N >= 2) && (N <= 3) && (m == 0))
                            {
                                cols_to_set.push_back(col);
                                rows_to_set.push_back(row);
                            }
                        }
                    }
                }

                // set all points in rows_to_set (of skel)
                unsigned int rows_to_set_size = (unsigned int)rows_to_set.size();
                for (unsigned int pt_idx = 0; pt_idx < rows_to_set_size; ++pt_idx)
                {
                    if (!changed)
                        changed = (skeleton.data[rows_to_set[pt_idx] * skeleton.cols + cols_to_set[pt_idx]]) > 0;

                    int key = rows_to_set[pt_idx] * skeleton.cols + cols_to_set[pt_idx];
                    skeleton.data[key] = 0;
                    if (cols_to_set[pt_idx] > 0 && skeleton.data[key - 1] == 128) // left
                        skeleton.data[key - 1] = 255;
                    if (cols_to_set[pt_idx] < skeleton.cols-1 && skeleton.data[key + 1] == 128) // right
                        skeleton.data[key + 1] = 255;
                    if (rows_to_set[pt_idx] > 0 && skeleton.data[key - skeleton.cols] == 128) // up
                        skeleton.data[key - skeleton.cols] = 255;
                    if (rows_to_set[pt_idx] < skeleton.rows-1 && skeleton.data[key + skeleton.cols] == 128) // down
                        skeleton.data[key + skeleton.cols] = 255;
                }

                if ((niters++) >= max_iters) // we have done!
                    break;
            }
        }

        skeleton = (skeleton != 0);
        return true;
    }

    void RobustSceneText::DetectText(InputArray colorImage,vector<Rect>&blackRects, vector<Rect>& whiteRects)
    {
        Mat image = colorImage.getMat();
        vector<ErTree> vecER;
        RobustSceneText rst;
        Mat gray;
        vector<ERFeatures> erFeatures;
        cvtColor(image,gray,CV_BGR2GRAY);
        rst.ProposeTextCandidate(gray,vecER);

        rst.ExtractFeatures(image,vecER,erFeatures);
        vector<int> labels;
        blackRects=rst.SingleLinkageCluster(erFeatures,labels);

        gray = 255-gray;
        rst.ProposeTextCandidate(gray,vecER);
        rst.ExtractFeatures(image,vecER,erFeatures);
        whiteRects=rst.SingleLinkageCluster(erFeatures,labels);
        /*  Mat drawImageWhite= image.clone();
        for(size_t i=0;i<whiteRects.size();++i)
        {
        rectangle(drawImageWhite,whiteRects[i],Scalar(51,249,40),2);
        }
        imwrite("testWhite.jpg",drawImageWhite);*/

    }
}//end of namespace cv
