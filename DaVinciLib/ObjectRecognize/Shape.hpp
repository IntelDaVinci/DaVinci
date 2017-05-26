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

#ifndef __SHAPE__
#define __SHAPE__

#include <vector>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "opencv2/core/core.hpp"

namespace DaVinci
{
    using namespace cv;
    using namespace std;

    enum ShapeType
    {
        ContourShape,
        MSERShape,
        SceneTextShape,
        Unknown
    };

    enum ShapeAlignOrientation
    {
        XALIGN,
        YALIGN,
        UNKNOWN
    };

    // Attention: serialized struct, if member changed, should sync it in SerializationUtil.hpp
    struct Shape
    {
        Point center;
        double area;
        vector<Point> contours;
        Rect boundRect;
        ShapeType type;
    };

    class ShapeHash
    {
    public:
        size_t operator() (const boost::shared_ptr<Shape> &key) const
        {
            std::hash<int> int_hash;
            return int_hash(key->boundRect.x) ^ int_hash(key->boundRect.y)
                ^ int_hash(key->boundRect.width) ^ int_hash(key->boundRect.height);
        }
    };

    class ShapeEqual
    {
    public:
        bool operator() (const boost::shared_ptr<Shape> t1, const boost::shared_ptr<Shape> t2) const
        {
            if(t1->boundRect.x == t2->boundRect.x
                && t1->boundRect.y == t2->boundRect.y
                && t1->boundRect.width == t2->boundRect.width
                && t1->boundRect.height == t2->boundRect.height
                )
                return true;
            else
                return false;
        }
    };
}

#endif //#ifndef __SHAPE__
