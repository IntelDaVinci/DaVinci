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

#ifndef __DAVINCI_TESTGROUP_H__
#define __DAVINCI_TESTGROUP_H__


#include <string>
#include <vector>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "boost/smart_ptr/enable_shared_from_this.hpp"

#include "boost/thread/recursive_mutex.hpp"
#include "TestInterface.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    class TestInterface;

    class TestGroup : public boost::enable_shared_from_this<TestGroup> 
    {
    private:
        vector<boost::shared_ptr<TestInterface>> testLane;
        boost::recursive_mutex mutexTestLane;

        TestGroup(const TestGroup&);

    public:
        /// <summary> Default constructor. </summary>
        TestGroup();

        /// <summary>
        /// Add a test into current test group
        /// </summary>
        /// <param name="test"> add a test into different lanes according to different types: observer, action and the rest.</param>
        void AddTest(const boost::shared_ptr<TestInterface> test, bool foreground=false);

        /// <summary>
        /// Precondition: Init() has been called.
        /// </summary>
        /// <param name="test"></param>
        bool StopTest(const boost::shared_ptr<TestInterface> test);

        /// <summary>
        /// Stops all tests.
        /// Precondition: Init() has been called.
        /// </summary>
        bool StopTests();

        /// <summary>
        /// Init this test group by init each test in this test group
        /// </summary>
        /// <returns>whether the init succeeded</returns>
        bool Init();

        /// <summary>
        /// Get captured frame sent from TestManager, send frame to each test in current test group in a specific order
        /// </summary>
        /// <param name="frame"> The frame captured</param>
        /// <param name="timeStamp"> The timestamp of the frame captured</param>
        /// <returns> The processed frame</returns>
        Mat ProcessFrame(const Mat& frame, double timeStamp, int frameIndex);

        /// <summary>
        /// Pass the audio event to the underlying tests.
        /// </summary>
        void ProcessWave(const boost::shared_ptr<vector<short>> &samples);

        /// <summary>
        /// Set each test in this test group to start state
        /// </summary>
        void Start();

        /// <summary>
        /// Check whether all tests in current test group finished.  The test group finishes if any one of the tests finished
        /// TestManager is free to destroy a test group if it is finished.
        /// </summary>
        /// <returns>the finish state of the test group</returns>
        bool IsFinished();

        /// <summary>
        /// 
        /// </summary>
        void SetFinished();
        /// <summary>
        /// 
        /// </summary>
        /// <param name="test"></param>
        void SetTestAsForeground(const boost::shared_ptr<TestInterface> test);
    };
}

#endif
