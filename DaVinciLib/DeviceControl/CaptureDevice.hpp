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

#ifndef __CAPTURE_DEVICE_HPP__
#define __CAPTURE_DEVICE_HPP__

#include <cstdint>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "boost/smart_ptr/enable_shared_from_this.hpp"
#include <vector>

#include "boost/thread/thread.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/function.hpp"
#include "boost/atomic.hpp"
#include "opencv2/opencv.hpp"

#include "DaVinciDefs.hpp"
#include "DaVinciCommon.hpp"
#include "DaVinciStatus.hpp"

namespace DaVinci
{
    using namespace std;

    class CaptureDevice : public boost::enable_shared_from_this<CaptureDevice>
    {
    public:
        typedef boost::function<void(boost::shared_ptr<CaptureDevice>)> FrameHandlerCallback;
        enum class Preset
        {
            PresetDontCare = -1,
            HighSpeed = 0,
            HighResolution = 1,
            AIResolution = 2,
            PresetNumber,
        };

        /// <summary> Default constructor. </summary>
        CaptureDevice() : currentPreset(Preset::PresetDontCare), shouldGrabThreadStop(true), isStarted(false)
        {
        }
        /// <summary> Destructor. </summary>
        virtual ~CaptureDevice()
        {
        }

        /// <summary> Gets camera name. </summary>
        ///
        /// <returns> The camera name. </returns>
        virtual string GetCameraName() = 0;

        /// <summary> Start the capture. </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        virtual DaVinciStatus Start();

        /// <summary> Stop the capture </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        virtual DaVinciStatus Stop();

        /// <summary> Restart the capture without stopping the capture thread </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        virtual DaVinciStatus Restart() = 0;

        /// <summary> Retrieve a frame from the capture </summary>
        ///
        /// <returns> The retrieved frame or nullptr if frame is not available yet </returns>
        virtual cv::Mat RetrieveFrame(int *frameIndex = NULL) = 0;

        /// <summary>
        /// Queries if a frame is available.
        /// The implementation may block until the frame available.
        /// </summary>
        ///
        /// <returns> true if a frame is available, false if not. </returns>
        virtual bool IsFrameAvailable() = 0;

        virtual bool InitCamera(CaptureDevice::Preset preset) = 0;

        /// <summary> Sets frame handler. </summary>
        ///
        /// <param name="callback"> The callback. </param>
        virtual void SetFrameHandler(FrameHandlerCallback &callback)
        {
            frameHandlerCallback = callback;
        }

        /// <summary> Gets the preset of the camera, indicating the resolution </summary>
        ///
        /// <returns> The preset. </returns>
        virtual Preset GetPreset()
        {
            return currentPreset;
        }

        virtual double GetCaptureProperty(int prop) = 0;

        /// <summary> Sets capture property. The property id is defined in opencv highgui </summary>
        ///
        /// <param name="prop">  The property. </param>
        /// <param name="value"> The value. </param>
        virtual void SetCaptureProperty(int prop, double value, bool shouldCheck = false) = 0;

        bool IsStarted()
        {
            return isStarted;
        }

    protected:
        Preset currentPreset;

        DaVinciStatus Join();
        DaVinciStatus SetStopFlag();


    private:
        FrameHandlerCallback frameHandlerCallback;
        boost::shared_ptr<boost::thread> grabThread;
        boost::atomic<bool> shouldGrabThreadStop;
        boost::mutex startStopMutex;
        boost::atomic<bool> isStarted;

        void GrabThreadEntry(void);
    };


    const string CapturePresetName[(int)CaptureDevice::Preset::PresetNumber + 1] = 
    {
        "Preset Don't care", 
        "High Speed",
        "High Resolution",
        "AI Resolution"
    };
}

#endif