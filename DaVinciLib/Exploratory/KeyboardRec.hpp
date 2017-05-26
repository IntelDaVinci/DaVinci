#ifndef _KeyboardRecaaa_HPP
#define _KeyboardRecaaa_HPP
#define WIN32_LEAN_AND_MEAN  

#include "TextRecognize.hpp"
#include "BaseObjectCategory.hpp"
#include"opencv2/highgui/highgui.hpp"
//#include"opencv/highgui.h"

#include<iostream>
#include<string.h>
#include <vector>

#include "ObjectUtil.hpp"

#include "ObjectCommon.hpp"
#include "boost/algorithm/string.hpp"
#include "AndroidBarRecognizer.hpp"
#include <regex>

namespace DaVinci
{
    using namespace std;
    using namespace cv;
    //using namespace DaVinci;

    struct rowInfo  //用于记录每个矩形的信息
    {
        Rect rowRect; //矩形
        int num; //在这一行的编号，比如‘a’是0，‘s’是1，‘d’是2，‘f’是3；  在九宫格中'   '是0（因为第一个键没有内容）‘abc’是1，‘def’是2
        string Character; //对于26个字母形式的键盘，进行单个字母的识别，这里可以是'a','s','d'等；对于九宫格，进行多字母识别比如'abc','def'
        rowInfo()
        {
            rowRect = Rect(0,0,0,0);
            num = 0;
            Character = "";
        }
        rowInfo(Rect rowrect, int Num, string &Ch)
        {
            rowRect = rowrect;
            num = Num;
            Character = Ch;
        }
        rowInfo & operator = (const rowInfo &a) 
        {
            if(this != &a)
            {
                this ->rowRect = a.rowRect;
                this ->num = a.num;
                this ->Character =a.Character;
            }
            return *this;
        }
        bool operator<(const rowInfo &a) const
        {
            return (rowRect.y< a.rowRect.y);
        }
        bool operator>=(const rowInfo &a) const
        {
            return (rowRect.y>= a.rowRect.y);
        }
    };

    struct widthInfo  //在计算key的宽度的时候，采取的办法是同一行两个矩形相减并除以之间的间隔， 比如‘d’的roInfo减去‘a’的rowInfo在除以（2-0）
    {
        int width;  //记录宽度值（）
        rowInfo rowLeft; //同时记录这个宽度值是由哪两个rowInfo算出来的，这样的话如果宽度值正确，也表示rowInfo框对了
        rowInfo rowRight;
        widthInfo()
        {
        }  

        widthInfo(int w, rowInfo L, rowInfo R)
        {
            width = w;
            rowLeft = L;
            rowRight = R;
        }

        bool operator < (const widthInfo &a) const
        {
            return (width < a.width);
        }

        bool operator >= (const widthInfo &a) const
        {
            return(width >= a.width);
        }

    };

    class KeyboardRec
    {
    public:

        KeyboardRec();
        ~KeyboardRec();

        std::unordered_map<char, Rect> RecForTwentieSix(Mat &src, std::vector<Rect> row1,std::vector<Rect> row2,std::vector<Rect> row3,std::vector<Rect> rectList); 
        void quickSort(rowInfo s[], int l, int r);   
        void quickSort(widthInfo s[], int l, int r);
        vector<rowInfo> vote(const vector<rowInfo> &src ,int distance);
        vector<widthInfo> vote(const vector<widthInfo> &src ,int distance);
        vector<rowInfo> KeyboardRec :: getRow(std::vector<Rect> row1); 
        vector<vector<Rect>> KeyboardRec ::getAllRow(Mat &src, std::vector<Rect> rectList, bool isSymbol);
        vector<rowInfo> KeyboardRec :: getSecondRow(const cv::Mat image, std::vector<Rect> row1, int type,int interval);
        vector<rowInfo> KeyboardRec :: getThirdRow(const cv::Mat image, std::vector<Rect> row2,int type,int interval);
        vector<rowInfo> KeyboardRec :: getFourthRow(const cv::Mat image, std::vector<Rect> row3,int type,int interval);
        vector<widthInfo> KeyboardRec :: getWidthInfo(const vector<rowInfo> &src);
        int KeyboardRec :: getY(const vector<widthInfo> &src); 
        vector<rowInfo> KeyboardRec :: localization(rowInfo mark, int rowNum, int width, int height,int Y, int type , int Boundary);

    private:

    };
}

#endif