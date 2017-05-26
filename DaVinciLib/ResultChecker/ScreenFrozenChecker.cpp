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

#include "ScreenFrozenChecker.hpp"
#include "DeviceManager.hpp"

#include "boost/tokenizer.hpp"

namespace DaVinci
{
    using namespace std;


    bool ScreenFrozenChecker::VerifyLastMotionEvent(const string& file, int distance)
    {
        // this function will search line which has "MotionEvent" at first, find lines contain ACTION_DOWN, MOVE, UP sequentially and save them to a string vector, if the serial events found again, the vector will be cleared and refilled
        // If the vector has elements, the next will check if y coordinate value increases
        ifstream istrm(file);
        string tgt = "MotionEvent";
        string tgtNxtBeg = "ACTION_DOWN";
        string tgtNxtMd = "ACTION_MOVE";
        string tgtNxtEnd = "ACTION_UP";
        enum pointerSate {UNKNOWN, DOWN, MOVE, UP};
        pointerSate lastState = UNKNOWN, curState = UNKNOWN;
        vector<string> evts; 

        while (istrm.good())
        {
            string line;
            getline(istrm, line);
            if (string::npos != line.find(tgt))
            {
                if (string::npos != line.find(tgtNxtBeg))
                {
                    curState = DOWN;

                    evts.clear();                       

                    evts.push_back(line);
                    lastState = curState;
                }
                else if (string::npos != line.find(tgtNxtMd))
                {
                    curState = MOVE;
                    if (lastState == UNKNOWN
                        || (lastState != DOWN
                            && lastState != MOVE))
                    {
                        evts.clear();
                    }
                    evts.push_back(line);                                                
                    lastState = curState;
                }
                else if (string::npos != line.find(tgtNxtEnd))
                {                    
                    curState = UP;
                    if (lastState == UNKNOWN
                        || lastState != MOVE)
                    {
                        evts.clear();
                    }                    
                    evts.push_back(line);
                    lastState = curState;
                }                    
            }
        }
        if (curState == UP && evts.size() >= 3)
        {
            float firstY = -1.0F;
            float curY = -1.0F;
            for ( auto& line : evts)
            {                    
                curY = -1.0;
                size_t pos = line.find("y[0]=");
                if (pos != string::npos)
                {
                    pos += 5;
                    size_t pos2 = line.find(",", pos);
                    if (pos2 != string::npos)
                    {
                        if (TryParse(line.substr(pos, pos2 - pos), curY))
                        {
                            if (FEquals(firstY, -1.0F))
                                firstY = curY;
                        }
                    }
                }
            }

            if (distance > 0 && abs(curY - firstY)  < distance)
                return false;
        }


        return true;
    }

    /// <summary>
    /// Sets the final result
    /// </summary>
    /// <param name="width">The width of DUT in default orientation mode</param>
    /// <param name="height">The height of DUT in default orientation mode</param>
    /// <param name="statusbarX">X size of status bar in default cordinate system</param>
    /// <param name="statusbarY">Y size of status bar in default cordinate system</param>
    /// <param name="hasStatusBar">If has status bar</param>
    /// <returns></returns>
    static void GetLayerInfo(const string& fileName, int& width, int& height,
                             int& statusbarX, int& statusbarY, bool& hasStatusBar)
    {
        ifstream istream = ifstream(fileName, ios::binary);
        string target = "Hardware Composer state";
        int numLayers = -1;
        hasStatusBar = false;
        while (istream.good())
        {
            string line;            
            getline(istream, line);
            if (numLayers != -1)
            {
                size_t idx = line.find('|');
                if (idx != string::npos)
                {
                    deque<string> fields;
                    boost::char_separator<char> sep("| \r");
                    auto token = boost::tokenizer<boost::char_separator<char>>(line, sep);
                    for (auto& elm : token)
                    {
                        fields.push_back(elm);
                    }

                    if (fields.back() == "StatusBar")
                    {
                        DAVINCI_LOG_DEBUG << "Found StatusBar";
                        hasStatusBar = true;
                        string field1, field2, field3, field4;
                        int pos = (int)(fields.size());
                        if (pos >= 5)
                        {
                            field1 = fields[pos-2];
                            field2 = fields[pos-3];
                            field3 = fields[pos-4];
                            field4 = fields[pos-5];
                            int bottom(0), right(0), top(0), left(0);
                            if (TryParse(ispunct(field1.at(field1.size()-1))?field1.substr(0, field1.size()-1):field1, bottom)                                
                                && TryParse(ispunct(field2.at(field2.size()-1))?field2.substr(0, field2.size()-1):field2, right)
                                && TryParse(ispunct(field3.at(field3.size()-1))?field3.substr(0, field3.size()-1):field3, top)
                                && TryParse(ispunct(field4.at(field4.size()-1))?field4.substr(0, field4.size()-1):field4, left))
                            {
                                statusbarX = right - left;
                                statusbarY = bottom - top;
                            }
                        }
                    }
                    else if (fields.back() == "HWC_FRAMEBUFFER_TARGET")
                    {
                        DAVINCI_LOG_DEBUG << "Found HWC_FRAMEBUFFER_TARGET";
                        string field1, field2;
                        int pos = (int)(fields.size());
                        if (pos >= 3)
                        {
                            field1 = fields[pos-2];     
                            field2 = fields[pos-3];
                            int w(0), h(0);
                            if (TryParse(ispunct(field1.at(field1.size()-1))?field1.substr(0, field1.size()-1):field1, h)
                                && TryParse(ispunct(field2.at(field2.size()-1))?field2.substr(0, field2.size()-1):field2, w))
                            {
                                height = h;
                                width = w;
                            }
                        }
                    }

                    numLayers --;
                    if (0 == numLayers)
                        return;
                }         
            }
            else
            {
                size_t idx = line.find(target);
            
                if (idx != string::npos)
                {
                    if (target == "Hardware Composer state")
                    {
                        DAVINCI_LOG_DEBUG << "Found layer table";
                        target = "numHwLayers=";
                    }
                    else if (target == "numHwLayers=")
                    {
                        size_t idxComma = line.find(',');
                        if (idxComma != string::npos && idxComma > idx)
                        {
                            numLayers = boost::lexical_cast<int>(line.substr(idx + target.size(),
                                idxComma - idx - target.size()));
                            // incuding table header
                            numLayers ++;
                        }
                    }
                }
            }            
        }
    }

    static inline bool SwipeDownAndParse(boost::shared_ptr<AndroidTargetDevice>& androidDut,
         int swipeXPos, int swipeDistance, const string& sfDumpFile, int commandTimeout, int timeWaitingSwipe, 
         bool permitClearLog, bool useInputSwipe = false)
    {
        string adbDir = androidDut->GetAdbPath();
        string dutID = androidDut->GetDeviceName();
        string dumpSf(adbDir + " -s " + dutID + " shell dumpsys SurfaceFlinger > " + sfDumpFile);
        string clrLog(adbDir + " -s " + dutID + " logcat -c");
        bool hasStatusBar;
        int dutWidth(0), dutHeight(0), statusbarSizeX(0), statusbarSizeY(0); 
        
        if (boost::filesystem::exists(sfDumpFile))
            boost::filesystem::remove(sfDumpFile);
        
        RunShellCommand(dumpSf, nullptr, "", commandTimeout);

        GetLayerInfo(sfDumpFile, dutWidth, dutHeight, statusbarSizeX, statusbarSizeY, hasStatusBar);

        RunShellCommand(clrLog, nullptr, "", commandTimeout);

        if (dutWidth == statusbarSizeX
            && dutHeight == statusbarSizeY)
        {
            // statusbar is fullscreen on the beginning
            androidDut->Drag(swipeXPos, swipeDistance, swipeXPos, 0, androidDut->GetCurrentOrientation(), useInputSwipe);
        }
        else
        {
            androidDut->Drag(swipeXPos, 0, swipeXPos, swipeDistance, androidDut->GetCurrentOrientation(), useInputSwipe);
        }

        ThreadSleep(timeWaitingSwipe);

        RunShellCommand(dumpSf, nullptr, "", commandTimeout);

        int statusbarSizeXTwice(0), statusbarSizeYTwice(0);        
        GetLayerInfo(sfDumpFile, dutWidth, dutHeight, statusbarSizeXTwice, statusbarSizeYTwice, hasStatusBar);

        if (statusbarSizeX != statusbarSizeXTwice
            || statusbarSizeY != statusbarSizeYTwice)
        {
            DAVINCI_LOG_INFO << "Screen not frozen.";            

            if (dutWidth == statusbarSizeXTwice
                && dutHeight == statusbarSizeYTwice)
            {
                // hide pull down if visible
                androidDut->Drag(swipeXPos, swipeDistance, swipeXPos, 0, androidDut->GetCurrentOrientation(), useInputSwipe);
            }
            return true;
        }

        return false;
    }

    ScreenFrozenChecker::ScreenFrozenChecker()
    {
        SetCheckerName("ScreenFrozenChecker");
    }

    CheckerResult ScreenFrozenChecker::Check(bool permitClearLog)
    {
        finalResult = NotRun;
        boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(DeviceManager::Instance().GetCurrentTargetDevice());

        assert (androidDut != nullptr);

        string adbDir = androidDut->GetAdbPath();
        string dutID = androidDut->GetDeviceName();
        auto tempFile = boost::filesystem::temp_directory_path();
        auto tempName = boost::filesystem::unique_path();
        tempFile /= tempName;
        string sfDumpFile(tempFile.string());
        string dumpLog(adbDir + " -s " + dutID + " logcat -s -d Input > " + sfDumpFile);
        DAVINCI_LOG_INFO << "ScreenFrozenChecker - dump file name is: " << sfDumpFile;
        int swipeXPos = int(AndroidTargetDevice::Q_COORDINATE_X * 0.5);
        int swipeDistance = int(AndroidTargetDevice::Q_COORDINATE_Y * 0.5);

        androidDut->PressHome();
        androidDut->PressHome();
        ThreadSleep(timeWaitingPressHome);
        
        // Current screen frozen checker don't support ARC++ device. It don't have notification bar.
        if (androidDut->IsARCdevice()) 
        {
            finalResult = Pass;
            return finalResult;
        }

        int count = 0;
        for( ; count < 5; ++ count)
        {
            int timeWaiting = timeWaitingSwipe*(count+1);
            if (!SwipeDownAndParse(androidDut, swipeXPos, swipeDistance, sfDumpFile, commandTimeout, timeWaiting, permitClearLog, false))
            {
                DAVINCI_LOG_DEBUG << "Screen could be frozen."; 
                if (SwipeDownAndParse(androidDut, swipeXPos, swipeDistance, sfDumpFile, commandTimeout, timeWaiting, permitClearLog, true))
                {
                    finalResult = Pass;
                    break;
                }
            }
            else
            {
                finalResult = Pass;
                break;
            }
        } 

        if (count == 5)
        {
            RunShellCommand(dumpLog, nullptr, "", commandTimeout);
            int distance = 0;
            Orientation ort = androidDut->GetCurrentOrientation(true);
            int width = androidDut->GetDeviceWidth();
            int height = androidDut->GetDeviceHeight();
            switch (ort)
            {
            case Orientation::Portrait:
            case Orientation::ReversePortrait:
                distance = max(width, height);
                break;
            case Orientation::Landscape:
            case Orientation::ReverseLandscape:
                distance = min(width, height);
                break;
            }
            if (!ScreenFrozenChecker::VerifyLastMotionEvent(sfDumpFile, distance /2 - 1))
            {
                DAVINCI_LOG_WARNING << "Swipe down event is not completed as expected.";
                return finalResult;
            }             
            DAVINCI_LOG_INFO << "Screen frozen.";
            finalResult = Fail;
        }

        return finalResult;
    }
}