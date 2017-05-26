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

/*
* Intel confidential -- do not distribute further
* This source code is distributed covered by "Internal License Agreement -- For Internal Open Source DaVinci Program"
* Please read and accept license.txt distributed with this package before using this source code
*/

#ifndef __DEVICE_CONTROL_COMMON_HPP__
#define __DEVICE_CONTROL_COMMON_HPP__

#include "boost/algorithm/string.hpp"
#include "DaVinciDefs.hpp"

namespace DaVinci
{
    /// <summary> Values that represent the connection status of the device. </summary>
    enum class DeviceConnectionStatus
    {
        /// <summary>
        /// connected
        /// </summary>
        Connected,
        /// <summary>
        /// Ready for connection or can be connected from current host now
        /// </summary>
        ConnectedByOthers,
        /// <summary>
        /// Ready for connection or can be connected from current host now
        /// </summary>
        ReadyForConnection,
        /// <summary>
        /// Offline
        /// </summary>
        Offline
    };

    enum class ScreenSource
    {
        /// <summary>
        /// undefined
        /// </summary>
        Undefined = -3,
        /// <summary>
        /// use blank screen
        /// </summary>
        Disabled = -2,
        /// <summary>
        /// Use hyper soft camera
        /// </summary>
        HyperSoftCam = -1,
        /// <summary>
        /// Camera Index 0
        /// </summary>
        CameraIndex0 = 0,
        /// <summary>
        /// Camera Index 1
        /// </summary>
        CameraIndex1 = 1,
        /// <summary>
        /// Camera Index 2
        /// </summary>
        CameraIndex2 = 2,
        /// <summary>
        /// Camera Index 3
        /// </summary>
        CameraIndex3 = 3,
        /// <summary>
        /// Camera Index 4
        /// </summary>
        CameraIndex4 = 4,
        /// <summary>
        /// Camera Index 5
        /// </summary>
        CameraIndex5 = 5,
        /// <summary>
        /// Camera Index 6
        /// </summary>
        CameraIndex6 = 6,
        /// <summary>
        /// Camera Index 7
        /// </summary>
        CameraIndex7 = 7
    };

    enum class Orientation
    {
        /// <summary>
        /// Portrait or Vertical screen
        /// </summary>
        Portrait = 0,
        /// <summary>
        /// Landscape or Horizontal screen
        /// </summary>
        Landscape = 1,
        /// <summary>
        /// Reverse of portrait
        /// </summary>
        ReversePortrait = 2,
        /// <summary>
        /// Reverse of landscape
        /// </summary>
        ReverseLandscape = 3,
        /// <summary>
        /// Cannot get the orientation of the device
        /// </summary>
        Unknown = -1
    };

    inline Orientation IntergerToOrientation(int i)
    {
        if (i == 0)
        {
            return Orientation::Portrait;
        }
        else if (i == 1)
        {
            return Orientation::Landscape;
        }
        else if (i == 2)
        {
            return Orientation::ReversePortrait;
        }
        else if (i == 3)
        {
            return Orientation::ReverseLandscape;
        }
        else
        {
            return Orientation::Unknown;
        }
    }

    inline string OrientationToString(Orientation orientation)
    {
        switch (orientation)
        {
        case Orientation::Landscape:
            return "landscape";
        case Orientation::Portrait:
            return "portrait";
        case Orientation::ReverseLandscape:
            return "reverselandscape";
        case Orientation::ReversePortrait:
            return "reverseportrait";
        default:
            return "";
        }
    }

    inline Orientation StringToOrientation(const string &s)
    {
        if (boost::to_lower_copy(s) == "landscape")
        {
            return Orientation::Landscape;
        }
        else if (boost::to_lower_copy(s) == "portrait")
        {
            return Orientation::Portrait;
        }
        else if (boost::to_lower_copy(s) == "reverselandscape")
        {
            return Orientation::ReverseLandscape;
        }
        else if (boost::to_lower_copy(s) == "reverseportrait")
        {
            return Orientation::ReversePortrait;
        }
        else
        {
            return Orientation::Unknown;
        }
    }

    inline string ScreenSourceToString(ScreenSource s)
    {
        switch (s)
        {
        case ScreenSource::Disabled:
            return "Disabled";
        case ScreenSource::CameraIndex0:
            return "CameraIndex0";
        case ScreenSource::CameraIndex1:
            return "CameraIndex1";
        case ScreenSource::CameraIndex2:
            return "CameraIndex2";
        case ScreenSource::CameraIndex3:
            return "CameraIndex3";
        case ScreenSource::CameraIndex4:
            return "CameraIndex4";
        case ScreenSource::CameraIndex5:
            return "CameraIndex5";
        case ScreenSource::CameraIndex6:
            return "CameraIndex6";
        case ScreenSource::CameraIndex7:
            return "CameraIndex7";
        case ScreenSource::HyperSoftCam:
            return "HyperSoftCam";
        default:
            return "Undefined";
        }
    }

    inline ScreenSource StringToScreenSource(const string &s)
    {
        if (boost::contains(boost::to_lower_copy(s), "disable"))
        {
            return ScreenSource::Disabled;
        }
        else if (boost::contains(boost::to_lower_copy(s), "hypersoftcam")
            || boost::contains(boost::to_lower_copy(s), "usescreencap"))
        {
            // backward compatibility: usescreencap
            return ScreenSource::HyperSoftCam;
        }
        else if (boost::contains(boost::to_lower_copy(s), "cameraindex0"))
        {
            return ScreenSource::CameraIndex0;
        }
        else if (boost::contains(boost::to_lower_copy(s), "cameraindex1"))
        {
            return ScreenSource::CameraIndex1;
        }
        else if (boost::contains(boost::to_lower_copy(s), "cameraindex2"))
        {
            return ScreenSource::CameraIndex2;
        }
        else if (boost::contains(boost::to_lower_copy(s), "cameraindex3"))
        {
            return ScreenSource::CameraIndex3;
        }
        else if (boost::contains(boost::to_lower_copy(s), "cameraindex4"))
        {
            return ScreenSource::CameraIndex4;
        }
        else if (boost::contains(boost::to_lower_copy(s), "cameraindex5"))
        {
            return ScreenSource::CameraIndex5;
        }
        else if (boost::contains(boost::to_lower_copy(s), "cameraindex6"))
        {
            return ScreenSource::CameraIndex6;
        }
        else if (boost::contains(boost::to_lower_copy(s), "cameraindex7"))
        {
            return ScreenSource::CameraIndex7;
        }
        else
        {
            if (boost::contains(boost::to_lower_copy(s), "undefined") == false)
                DAVINCI_LOG_WARNING << "Unknown capture source: " << s << ". Converted to undefined source!";
            return ScreenSource::Undefined;
        }
    }

    inline bool IsHorizontalOrientation(Orientation orientation)
    {
        if (orientation == Orientation::Landscape || orientation == Orientation::ReverseLandscape)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    inline bool IsReverseOrientation(Orientation orientation)
    {
        if (orientation == Orientation::ReverseLandscape || orientation == Orientation::ReversePortrait)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    inline int ScreenSourceToIndex(ScreenSource screenSource)
    {
        return static_cast<int>(screenSource);
    }

    inline ScreenSource IndexToScreenSource(int index)
    {
        if (-2 <= index && index <= -1)
        {
            return static_cast<ScreenSource>(index);
        }
        else if (0 <= index && index <= 7)
        {
            return static_cast<ScreenSource>(index);
        }
        else
        {
            return ScreenSource::Undefined;
        }
    }
}

#endif