#include "KeyboardRec.hpp"
#include "TextUtil.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;
    KeyboardRec :: KeyboardRec(){};
    KeyboardRec :: ~KeyboardRec(){};


    vector<rowInfo> KeyboardRec ::getRow(std::vector<Rect> row1)
    {
        vector<rowInfo> rowInfoVecter;
        int num = 0;
        for(auto row:row1)
        {
            string str = "NONE";
            rowInfoVecter.push_back(rowInfo(row, num ,str));
            num++;
        }
        return rowInfoVecter;
    }

    vector<rowInfo> KeyboardRec :: getSecondRow(const cv::Mat image, std::vector<Rect> row1, int type, int interval)
    {
        vector<rowInfo> rowInfoVecter;

        int capital = 0;
        if(type)
        {
            capital = 32;
        }
        for (auto rect : row1)
        {
            if (0.8 > (1.0* rect.x) / interval )
            {
                string str = "q";
                str[0] = (char)('q'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 0 ,str));
                continue;
            }
            else if (1.8 > (1.0* rect.x) / interval )
            {
                string str = "w";
                str[0] = (char)('w'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 1 ,str));
                continue;
            }
            else if (2.8 > (1.0* rect.x) / interval )
            {
                string str = "e";
                str[0] = (char)('e'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 2 ,str));
                continue;
            }
            else if (3.8 > (1.0* rect.x) / interval )
            {
                string str = "r";
                str[0] = (char)('r'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 3 ,str));
                continue;
            }
            else if (4.8 > (1.0* rect.x) / interval )
            {
                string str = "t";
                str[0] = (char)('t'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 4 ,str));
                continue;
            }
            else if (5.8 > (1.0* rect.x) / interval )
            {
                string str = "y";
                str[0] = (char)('y'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 5 ,str));
                continue;
            }
            else if (6.8 > (1.0* rect.x) / interval )
            {
                string str = "u";
                str[0] = (char)('u'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 6 ,str));
                continue;
            }
            else if (7.8 > (1.0* rect.x) / interval )
            {
                string str = "i";
                str[0] = (char)('i'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 7 ,str));
                continue;
            }
            else if (8.8 > (1.0* rect.x) / interval )
            {
                string str = "o";
                str[0] = (char)('o'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 8 ,str));
                continue;
            }
            else if (9.8 > (1.0* rect.x) / interval )
            {
                string str = "p";
                str[0] = (char)('p'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 9 ,str));
                continue;
            }
        }
        return rowInfoVecter;
    }

    vector<rowInfo> KeyboardRec :: getThirdRow(const cv::Mat image, std::vector<Rect> row2, int type, int interval)
    {
        vector<rowInfo> rowInfoVecter;

        int capital = 0;
        if(type)
        {
            capital = 32;
        }
        for (auto rect : row2)
        {
            if (0 == ((rect.x - (int)(0.7*interval))/interval))
            {
                string str = "a";
                str[0] = (char)('a'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 0 ,str));

            }
            else if (1 == ((rect.x - (int)(0.7*interval))/interval))
            {
                string str = "s";
                str[0] = (char)('s'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 1 ,str));
            }
            else if (2 == ((rect.x - (int)(0.7*interval))/interval))
            {
                string str = "d";
                str[0] = (char)('d'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 2 ,str));
            }
            else if (3 == ((rect.x - (int)(0.7*interval))/interval))
            {
                string str = "f";
                str[0] = (char)('f'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 3 ,str));
            }
            else if ((4 == ((rect.x - (int)(0.7*interval))/interval)))
            {
                string str = "g";
                str[0] = (char)('g'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 4 ,str));
            }
            else if ((5 == ((rect.x - (int)(0.7*interval))/interval)))
            {
                string str = "h";
                str[0] = (char)('h'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 5 ,str));
            }
            else if ((6 == ((rect.x - (int)(0.7*interval))/interval)))
            {
                string str = "j";
                str[0] = (char)('j'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 6 ,str));
            }
            else if ((7 == ((rect.x - (int)(0.7*interval))/interval)))
            {
                string str = "k";
                str[0] = (char)('k'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 7 ,str));
            }
            else if ((0 == ((rect.x - (int)(0.7*interval))/interval)))
            {
                string str = "l";
                str[0] = (char)('l'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 8 ,str));
            }
        }
        return rowInfoVecter;
    }

    vector<rowInfo> KeyboardRec :: getFourthRow(const cv::Mat image, std::vector<Rect> row3, int type, int interval)
    {
        vector<rowInfo> rowInfoVecter;

        int capital = 0;
        if(type)
        {
            capital = 32;
        }
        for (auto rect : row3)
        {
            if (1.8 > ((rect.x - 0.6*interval)/interval))
            {
                string str = "z";
                str[0] = (char)('z'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 0 ,str));
                continue;
            }
            else if (2.8 > ((rect.x - 0.6*interval)/interval))
            {
                string str = "x";
                str[0] = (char)('x'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 1 ,str));
                continue;
            }
            else if (3.8 > ((rect.x - 0.6*interval)/interval))
            {
                string str = "c";
                str[0] = (char)('c'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 2 ,str));
                continue;
            }
            else if (4.8 > ((rect.x - 0.6*interval)/interval))
            {
                string str = "v";
                str[0] = (char)('v'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 3 ,str));
                continue;
            }
            else if (5.8 > ((rect.x - 0.6*interval)/interval))
            {
                string str = "b";
                str[0] = (char)('b'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 4 ,str));
                continue;
            }
            else if (6.8 > ((rect.x - 0.6*interval)/interval))
            {
                string str = "n";
                str[0] = (char)('n'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 5 ,str));
                continue;
            }
            else if (7.8 > ((rect.x - 0.6*interval)/interval))
            {
                string str = "m";
                str[0] = (char)('m'-capital);
                rowInfoVecter.push_back(rowInfo(rect, 6 ,str));
                continue;
            }
        }
        return rowInfoVecter;
    }

    void KeyboardRec :: quickSort(rowInfo s[], int l, int r)
    {
        if (l< r)  
        {        
            int i = l, j = r;
            rowInfo x = s[l];  
            while (i < j)  
            {  
                while(i < j && s[j]>= x) // find the first number which smaller than x
                    j--;   
                if(i < j)  
                    s[i++] = s[j];  
                while(i < j && s[i]< x) // find the first number which bigger than x
                    i++;   
                if(i < j)  
                    s[j--] = s[i];
            }  
            s[i] = x;  
            quickSort(s, l, i - 1);
            quickSort(s, i + 1, r);
        }  
    }

    void KeyboardRec :: quickSort(widthInfo s[], int l, int r)
    {
        if (l< r)  
        {        
            int i = l, j = r;
            widthInfo x = s[l];  
            while (i < j)  
            {  
                while(i < j && s[j]>= x)
                    j--;   
                if(i < j)  
                    s[i++] = s[j];  
                while(i < j && s[i]< x)
                    i++;   
                if(i < j)  
                    s[j--] = s[i];  
            }  
            s[i] = x;  
            quickSort(s, l, i - 1);
            quickSort(s, i + 1, r);  
        }  
    }

    vector<rowInfo> KeyboardRec :: vote(const vector<rowInfo> &src ,int distance) 
    {
        rowInfo *srcArray = new rowInfo[src.size()];
        vector<rowInfo> dst;
        for(size_t i =0 ;i <src.size(); i++)
        {
            srcArray[i] = src[i];
        }

        quickSort(srcArray, 0, (int)src.size()-1); // quick sort using row info
        int   StartTmp, EndTmp,Length, Start, End;
        Length = StartTmp = Start = EndTmp = End = 0;

        rowInfo a,b;

        for (; EndTmp < (int)src.size()-1; EndTmp++)   //compare data in SrcArray, split to two groups, keep one group which have more data
        {
            a =  srcArray[EndTmp+1];
            b =  srcArray[EndTmp];

            if ( a.rowRect.y -b.rowRect.y > distance)
            {
                if ( (EndTmp - StartTmp +1) > Length)
                {
                    Length = EndTmp - StartTmp +1;
                    Start = StartTmp;
                    End = EndTmp;
                }
                StartTmp = EndTmp +1;
            }

            else if(EndTmp + 1 == (int)(src.size() - 1))
            {
                if ( (EndTmp - StartTmp + 2) > Length)
                {
                    Length = EndTmp - StartTmp + 2;
                    Start = StartTmp;
                    End = EndTmp + 1;
                }
            }
        }
        for(int j = 0; j < Length; j++)
        {
            dst.push_back(srcArray[Start +j]);
        }
        return dst;
    }

    vector<widthInfo> KeyboardRec :: vote(const vector<widthInfo> &src ,int distance) 
    {
        widthInfo *srcArray = new widthInfo[src.size()];

        vector<widthInfo> dst;
        for(size_t i =0 ;i <src.size(); i++)
        {
            srcArray[i] = src[i];
        }

        quickSort(srcArray, 0, (int)src.size()-1);
        int   StartTmp, EndTmp,Length, Start, End;
        Length = StartTmp = Start = EndTmp = End = 0;

        for (; EndTmp < (int)src.size()-1; EndTmp++)
        {
            if ( (srcArray[EndTmp+1].width-srcArray[EndTmp].width) > distance)
            {
                if ( (EndTmp - StartTmp +1) > Length)
                {
                    Length = EndTmp - StartTmp +1;
                    Start = StartTmp;
                    End = EndTmp;
                }
                StartTmp = EndTmp +1;
            }
            else if(EndTmp +1 == (int)(src.size() - 1))
            {
                if ( (EndTmp - StartTmp +2) > Length)
                {
                    Length = EndTmp - StartTmp +2;
                    Start = StartTmp;
                    End = EndTmp + 1;
                }
            }
        }
        for(int j = 0; j < Length; j++)
        {
            dst.push_back(srcArray[Start +j]);
        }
        return dst;
    }

    vector<widthInfo> KeyboardRec :: getWidthInfo(const vector<rowInfo> &src) 
    {
        vector<widthInfo> rowWidth;
        rowWidth.clear();
        for(size_t i = 0; i < src.size();i++)
        {
            for(size_t j = i+1; j < src.size();j++)
            {
                if(src[j].num == src[i].num)break;
                int width = (int)(src[j].rowRect.x - src[i].rowRect.x )/(src[j].num-src[i].num);
                widthInfo widthInfoElement (abs(width), src[i],src[j]);
                rowWidth.push_back(widthInfoElement);
            }
        }
        return rowWidth;
    }

    int KeyboardRec :: getY(const vector<widthInfo> & src) 
    {
        int cnt = 0;
        int Y = 0;
        for (size_t i = 0; i < src.size(); i++ )
        {
            cnt = cnt + 2;
            Y = Y + src[i].rowLeft.rowRect.y + src[i].rowLeft.rowRect.height/2 + src[i].rowRight.rowRect.y + src[i].rowRight.rowRect.width/2;
        }
        if (cnt != 0)
            Y = (int)(Y/cnt);
        return Y;
    }

    vector<rowInfo> KeyboardRec :: localization(rowInfo mark, int rowNum, int width, int height,int Y, int type , int Boundary)
    {
        int length = 0;; //number of keys in this row 
        string dictStr;
        int Capital = 0;
        if(type == 1)//Capitalization
        {
            Capital = 32;
        }

        switch (rowNum)
        {
        case 0: 
            length = 7;
            dictStr = "zxcvbnm";
            break;
        case 1:  
            length = 9;
            dictStr = "asdfghjkl";
            break;
        case 2:
            length = 10;
            dictStr = "qwertyuiop";
            break;
        default:
            break;
        }

        vector<rowInfo> rowKeyboard;
        rowKeyboard.clear();
        int startX = (int)(mark.rowRect.x + mark.rowRect.width *0.5 - (mark.num + 0.5)* width);
        const char *dictChar = dictStr.c_str();
        for(int i = 0; i < length; i++)
        {
            int X = startX + i * width;
            int widthReal = width;
            if((X + width) >= Boundary)
            {
                widthReal = Boundary - X;
            }
            Rect rect = Rect(X, Y, widthReal, height); 
            char ch = (char)(dictChar[i] + Capital);
            string s;
            s.insert( s.begin(), ch );
            rowInfo rowKey(rect, i, s);
            rowKeyboard.push_back(rowKey);
        }
        return rowKeyboard;
    }

    vector<vector<Rect>> KeyboardRec ::getAllRow(Mat &src, std::vector<Rect> rectList, bool isSymbol)
    {
        int errorY = (int)(0.005 *src.rows);
        std::unordered_map<int, int> dicY;

        for (auto rect : rectList)
        {
            if (dicY.find(rect.y) == dicY.end())
            {
                dicY.insert(make_pair(rect.y, 1));
            }
            else
            {
                ++dicY[rect.y];
            }
        }
        vector<pair<int, int>> dicYSort(dicY.begin(), dicY.end());
        sort( dicYSort.begin(),  dicYSort.end(), SortPairByIntKeySmaller);//from buttom to the top

        vector<pair<int, int>> dicYPairs;
        dicYPairs.clear();
        int cnt = 0;
        for(auto Ypairs:dicYSort)
        {
            if(Ypairs.second > 5 && cnt < 4)
            {
                dicYPairs.push_back(Ypairs); //from bottom to top
                cnt++;
            }
            if(cnt == 4)
            {
                break;
            }
        }
        //arrange three rows
        sort(dicYPairs.begin(), dicYPairs.end(), SortPairByIntKeySmaller);
        std::vector<int> rowYcoordinate;

        if(isSymbol)
        {
            for(auto item: dicYPairs)
            {
                rowYcoordinate.push_back(item.first);
            }
            rowYcoordinate.push_back(dicYPairs[dicYPairs.size()-1].first);
            cnt = 4;
        }
        else
        {
            for(auto item: dicYPairs)
            {
                rowYcoordinate.push_back(item.first);
            }
        }

        std::sort(rowYcoordinate.begin(), rowYcoordinate.end());

        // four row keyboard: the first row contains digit and letter
        vector<Rect> row0;
        vector<Rect> row1;
        vector<Rect> row2;
        vector<Rect> row3;

        // filter some rectangle which are not around the four row y coordinate
        if(cnt >= 4 && rowYcoordinate.size() >= 4)
        {
            for (auto rect : rectList)
            {
                if (abs(rect.y - rowYcoordinate[0]) < errorY )
                {
                    row0.push_back(rect);
                }
                else if (abs(rect.y - rowYcoordinate[1]) < errorY )
                {
                    row1.push_back(rect);
                }
                else if (abs(rect.y - rowYcoordinate[2]) < errorY )
                {
                    row2.push_back(rect);
                }
                else if (abs(rect.y - rowYcoordinate[3]) < errorY )
                {
                    row3.push_back(rect);
                }
            }
        }
        else if (rowYcoordinate.size() >= 1)
        {
            for (auto rect : rectList)
            {
                if (abs(rect.y - rowYcoordinate[0]) < errorY )
                {
                    row0.push_back(rect);
                }
            }
        }

        if(isSymbol)
        {
            row1 = row0;
        }

        vector<vector<Rect>> AllRow;
        AllRow.clear();
        AllRow.push_back(row0);
        AllRow.push_back(row1);
        AllRow.push_back(row2);
        AllRow.push_back(row3);

        return AllRow;
    }

    std::unordered_map<char, Rect> KeyboardRec :: RecForTwentieSix(Mat &src, std::vector<Rect> row1,std::vector<Rect> row2,std::vector<Rect> row3,std::vector<Rect> rectList) //用于26宫格，1表示大写，小写
    {
        std::unordered_map<char, Rect> dicLetter;
        // sort rectangle by X
        SortRects(row1, UTYPE_X, true);
        SortRects(row2, UTYPE_X, true);
        SortRects(row3, UTYPE_X, true);

        vector<rowInfo> keyboardInfo[3];
        vector<rowInfo> keyboardRenfineY[3];
        vector<widthInfo> keyboardWidth[3];
        vector<widthInfo> keyboardRefineWidth[3];
        int Y[3];
        int errorY = (int)(0.005 * src.rows);
        int errorWidth = (int)(0.003 * src.cols);

        //Step 1 getRow
        keyboardInfo[0] = getRow(row3);
        keyboardInfo[1] = getRow(row2);
        keyboardInfo[2] = getRow(row1);

        //Step 2 Refine
        for(int i = 0; i < 3; i++) 
        {
            keyboardRenfineY[i] = vote(keyboardInfo[i],errorY); 
            keyboardWidth[i] = getWidthInfo(keyboardRenfineY[i]);
            keyboardRefineWidth[i] = vote(keyboardWidth[i],errorWidth);
            Y[i] = getY(keyboardRefineWidth[i]);
        }

        // Step3 get height & width
        int height = 0;
        height = (int)abs((Y[1]-Y[0] +Y[2] -Y[0])/3);
        height = (int)(height * 0.95);

        int width = 0;
        int cntWidth = 0;
        for(int i = 0; i <3; i++)
        {
            for(size_t j =0 ;j < keyboardRefineWidth[i].size(); j++)
            {
                width = width + keyboardRefineWidth[i][j].width;
                cntWidth ++;
            }
        }

        if (cntWidth == 0)
            return dicLetter;

        width = (int)(width/cntWidth);
        int type = 0;
        int interval = width;

        //Step 4 Map
        vector<rowInfo> keyboardMap[3];
        vector<widthInfo> WidthMap[3];
        keyboardMap[0] = getFourthRow(src, row3, type,interval); 
        keyboardMap[1] = getThirdRow(src, row2, type,interval);
        keyboardMap[2] = getSecondRow(src, row1, type,interval);
        for(int i = 0; i<3; i++)
        {
            WidthMap[i] = getWidthInfo(keyboardMap[i]);
        }

        //Step 5 locolization
        vector<rowInfo> keyboardRec;
        keyboardRec.clear();
        vector<rowInfo> rowRec[3];
        for(int i = 0; i<3; i++)  
        {
            if (WidthMap[i].size() == 0)
                continue;
            int markIndex = (int)WidthMap[i].size()/2;
            int Ystart = Y[i]-(int)(height/2) + (int)(WidthMap[i][markIndex].rowLeft.rowRect.height *0.5);
            rowRec[i] = localization(WidthMap[i][markIndex].rowLeft, i ,width,height,Ystart,type,src.cols);
            keyboardRec.insert(keyboardRec.begin(),rowRec[i].begin(),rowRec[i].end());
        }

        //find delete & enter
        bool isDelInFirstRow = false;
        if (rowRec[0].size() < 6 || rowRec[1].size() < 8 || rowRec[2].size() < 9)
            return dicLetter;

        int intervalForDel = (src.cols - (rowRec[0][6].rowRect.x + width));
        if(intervalForDel > 2* width)
        {
            isDelInFirstRow = true;
        }
        if(isDelInFirstRow)
        {
            Rect rectDel = Rect(rowRec[2][9].rowRect.x + (int)(1.2*width), rowRec[2][9].rowRect.y,width, height);
            string strDel = "Del";
            rowInfo rowInfoDel(rectDel, -1, strDel);
            keyboardRec.push_back(rowInfoDel);

            int heightEnter = (int)(0.8*height);
            Rect rectEnter = Rect(rowRec[1][8].rowRect.x + (int)(1.2*width), rowRec[1][8].rowRect.y,(int)(1.3 * width), heightEnter);
            string strEnter = "Enter";
            rowInfo rowInfoEnter(rectEnter, -1, strEnter);
            keyboardRec.push_back(rowInfoEnter);
        }
        else
        {
            int XDel = src.cols - (int)(1.3*width);
            Rect rectDel = Rect(XDel, rowRec[0][6].rowRect.y,(int)(1.2 * width), height);
            string strDel = "Del";
            rowInfo rowInfoDel(rectDel, -1, strDel);
            keyboardRec.push_back(rowInfoDel);

            int heightEnter = (int)(0.8*height);
            if(src.rows == (rowRec[1][8].rowRect.y + heightEnter))
            {
                heightEnter = src.rows - rowRec[1][8].rowRect.y;
            }
            Rect rectEnter = Rect(XDel, rowRec[0][6].rowRect.y + height,(int)(1.2 * width), heightEnter);
            string strEnter = "Enter";
            rowInfo rowInfoEnter(rectEnter, -1, strEnter);
            keyboardRec.push_back(rowInfoEnter);
        }

        //find upper &switch
        int YUpper = rowRec[0][0].rowRect.y;
        int HeightUpper = height;
        Rect rectUpper = Rect(0, YUpper, (int)( width), HeightUpper);
        string strUpper = "Upper";
        rowInfo rowInfoUpper(rectUpper, -1, strUpper);
        keyboardRec.push_back(rowInfoUpper);

        if(isDelInFirstRow)
        {
            Rect rectUpperRight = Rect(src.cols - width, rowRec[0][0].rowRect.y, (int)( width), height);
            rowInfo rowInfoUpperRight(rectUpperRight, -1, strUpper);
            keyboardRec.push_back( rowInfoUpperRight);
        }

        Rect rectSwitch = Rect(0, rowRec[0][0].rowRect.y +(int)(1.1 * height), (int)(1.3* width), (int)(0.7*height));
        string strSwitch = "Switch";
        rowInfo rowInfoSwitch(rectSwitch, -1, strSwitch);
        keyboardRec.push_back(rowInfoSwitch);

        //find space
        int WidthSpace = 5 * width;
        int XSpace = (int)(0.5*src.cols) - (int)(2.5*width);
        for(auto rect: rectList)
        {
            if(rect.width > 4 * width)
            {
                WidthSpace = rect.width;
                XSpace = rect.x;
            }
        }
        int YSpace = rowRec[0][1].rowRect.y + (int)(1.1*height);
        int HeightSpace = (int)(0.8*height);
        if((YSpace + HeightSpace)> src.rows)
        {
            HeightSpace = src.rows - YSpace;
        }
        Rect rectSpace = Rect(XSpace, YSpace, WidthSpace, HeightSpace);
        string strSpace = "Space";
        rowInfo rowInfoSpace(rectSpace, -1, strSpace);
        keyboardRec.push_back(rowInfoSpace);

        //find Dot
        int XDot = src.cols - (int)(2.4*width);
        int YDot = YSpace;

        Rect rectDot = Rect(XDot, YDot, width, HeightSpace);
        string strDot = "Dot";
        rowInfo rowInfoDot(rectDot, -1 ,strDot);
        keyboardRec.push_back(rowInfoDot);


        for(auto key: keyboardRec)
        {
            if("Space" == key.Character)
            {
                dicLetter[' '] = key.rowRect;
            }
            else if("Switch" == key.Character)
            {
                dicLetter['#'] = key.rowRect;
            }
            else if("Upper" == key.Character)
            {
                dicLetter['>'] = key.rowRect;
            }
            else if("Dot" == key.Character)
            {
                dicLetter['.'] = key.rowRect;
            }
            else if("Del" == key.Character)
            {
                dicLetter['~'] = key.rowRect;
            }
            else if("Enter" == key.Character)
            {
                dicLetter['`'] = key.rowRect;
            }
            else
            {
                dicLetter[key.Character[0]] = key.rowRect;
            }
        }
        return dicLetter;
    }
}