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

/* You can serialize a struct or a class as the following steps:
* First, include the hpp of the one you want to serialize
* Second, add template function serialize(Archive &ar, XXX &xxx, const unsigned int version) as shown.
*/

#ifndef _SERIALIZATIONUTIL_
#define _SERIALIZATIONUTIL_

#pragma warning(disable:4310)
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include "opencv2/core/core.hpp"

#include "DaVinciCommon.hpp"
#include "ScriptHelper.hpp"
#include "Shape.hpp"

namespace DaVinci
{
    class SerializationUtil : public SingletonBase<SerializationUtil>
    {
    public:
        template<typename Type>
        void Serialize(std::string fileName, Type object);

        template<typename Type>
        Type Deserialize(std::string fileName);
    };
}

namespace boost
{
    namespace serialization
    {
        template<class Archive>
        void serialize(Archive &ar, cv::Point &point, const unsigned int version)
        {
            ar & point.x;
            ar & point.y;
        }

        template<class Archive>
        void serialize(Archive &ar, cv::Rect &rect, const unsigned int version)
        {
            ar & rect.x;
            ar & rect.y;
            ar & rect.width;
            ar & rect.height;
        }

        template<class Archive>
        void serialize(Archive &ar, cv::Size &size, const unsigned int version)
        {
            ar & size.width;
            ar & size.height;
        }

        template<class Archive>
        void serialize(Archive &ar, cv::Mat &mat, const unsigned int version)
        {
            int cols;
            int rows;
            int type;
            bool isContinuous;

            if (Archive::is_saving::value)
            {
                cols = mat.cols;
                rows = mat.rows;
                type = mat.type();
                isContinuous = mat.isContinuous();
            }
            ar & cols;
            ar & rows;
            ar & type;
            ar & isContinuous;

            if (Archive::is_loading::value)
            {
                mat.create(rows, cols, type);
            }

            if (isContinuous)
            {
                const size_t dataSize = rows * cols * mat.elemSize();
                ar & boost::serialization::make_array(mat.ptr(), dataSize);
            }
            else
            {
                const size_t rowSize = cols * mat.elemSize();
                for (int i = 0; i < rows; i++)
                {
                    ar & boost::serialization::make_array(mat.ptr(i), rowSize);
                }
            }
        }

        template<class Archive>
        void serialize(Archive &ar, DaVinci::Shape &shape, const unsigned int version)
        {
            ar & shape.center;
            ar & shape.area;
            ar & shape.contours;
            ar & shape.boundRect;
            ar & shape.type;
        }

        template<class Archive>
        void serialize(Archive &ar, DaVinci::ScriptHelper::QSPreprocessInfo &info, const unsigned int version)
        {
            ar & info.frame;
            ar & info.mainROI;
            ar & info.endIp;
            ar & info.shapes;

            ar & info.alignShapes;
            ar & info.alignOrientation;
            ar & info.clickedAlignShapeIndex;

            ar & info.startLine;
            ar & info.endLine;

            ar & info.framePoint;
            ar & info.devicePoint;

            ar & info.isTouchMove;
            ar & info.isTouchAction;
            ar & info.clickedShape;
            ar & info.clickedTextRect;
            ar & info.clickedEnText;
            ar & info.clickedZhText;

            ar & info.isVirtualKeyboard;
            ar & info.keyboardInput;

            ar & info.isVirtualNavigation;
            ar & info.hasVirtualNavigation;
            ar & info.navigationBarRect;
            ar & info.barLocation;
            ar & info.navigationEvent;

            ar & info.isAdvertisement;

            ar & info.isPopUpWindow;
            ar & info.windows;
            ar & info.focusWindow;
        }
    }
}

#include "SerializationUtil.cpp"
#endif