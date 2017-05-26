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

#include "MapPowerPusher.hpp"

namespace DaVinci
{
    //Ready
    MapPowerPusher::MapPowerPusher()
    {
        InitParameters();
    }

    //Ready
    MapPowerPusher::~MapPowerPusher()
    {

    }

    void MapPowerPusher::InitParameters()
    {
        map_process_result = true;      //initial the flag!
        pusher_num_before = 0;
		device_num_before = 0;
		pusher_num_after = 0;
		device_num_after = 0;
    }

    //Ready except for Lotus image match
    bool MapPowerPusher::Init()
    {
        if (TestManager::Instance().IsConsoleMode())
        {
            //TODO
            //TestManager.closeViewer();
            //CloseImageViewer();
        }

        SetName("Mapping_Power_Pusher_with_Mobile_Device_Test");
        DAVINCI_LOG_INFO << string("*************************************************************");
        DAVINCI_LOG_INFO << string("Command line to mapping power pusher with mobile device!");
        DAVINCI_LOG_INFO << string("*************************************************************");

#pragma region //chong: Initial log file.
        exe_direction = TestManager::Instance().GetDaVinciHome();
        if (exe_direction == "")
        {
            DAVINCI_LOG_ERROR << string("Error: DaVinci Directory must NOT be NULL!") << endl;
            return false;
        }
        else
        {
            DAVINCI_LOG_DEBUG << string("DaVinci Directory is: ") << exe_direction << endl;
        }

        boost::filesystem::path portable_LogFile(this->exe_direction + "/" + "MappingLogFile.txt");
        LogFile = portable_LogFile.generic_string();
        DAVINCI_LOG_DEBUG << string("The log is stored in: ") << LogFile << endl;
        if (fileExists(LogFile))
        {
            DAVINCI_LOG_DEBUG << string("This MappingLogFile.txt file already exists, I will delete it!");
            DeleteFile(LogFile.c_str());
        }
        DAVINCI_LOG_DEBUG << string("This MappingLogFile.txt file doesn't exist, I will create it!");
        ofstream file_out(LogFile);
        file_out.close();

        boost::filesystem::path portable_DetailedLogFile(this->exe_direction + "/" + "DetailedMappingLogFile.txt");
        DetailedLogFile = portable_DetailedLogFile.generic_string();
        DAVINCI_LOG_DEBUG << string("The detailed log is stored in: ") << DetailedLogFile << endl;
        if (fileExists(DetailedLogFile))
        {
            DAVINCI_LOG_DEBUG << string("This DetailedMappingLogFile.txt file already exists, I will delete it!");
            DeleteFile(DetailedLogFile.c_str());
        }
        DAVINCI_LOG_DEBUG << string("This DetailedMappingLogFile.txt file doesn't exist, I will create it!");
        ofstream detailed_file_out(DetailedLogFile);
        detailed_file_out.close();
    
        InitParameters();
#pragma endregion

#pragma region //chong: Initial all power pusher.
        FFRD_before = (DeviceManager::Instance()).GetHWAccessoryControllerNames();

        //pusher_num_before = FFRD_before.capacity;
        pusher_num_before = (int)FFRD_before.size();
        DAVINCI_LOG_INFO << string("-------------------------------------------------------------");
        if (pusher_num_before == 0)
        {
            DAVINCI_LOG_INFO << string("There is no power pusher connected with the host PC before mapping.");
        }
        if (pusher_num_before == 1)
        {
            DAVINCI_LOG_INFO << string("There is ") << pusher_num_before << string(" power pusher connected with the host PC before mapping.");
            DAVINCI_LOG_INFO << string("No.1 power pusher is connected on: ") << FFRD_before[0];
        }
        else
        {
            DAVINCI_LOG_INFO << string("There are ") << pusher_num_before << string(" power pushers connected with the host PC before mapping.");
            for (int i = 1; i <= pusher_num_before; i++)
            {
                DAVINCI_LOG_INFO << string("No.") << i << string(" power pusher is connected on: ") << FFRD_before[i - 1];
            }
        }
        DAVINCI_LOG_INFO << string("-------------------------------------------------------------");
#pragma endregion

#pragma region //chong: Initial all mobile device.
        AndroidTargetDevice::AdbDevices(mobile_device_before);
        //device_num_before = mobile_device_before.capacity;
        device_num_before = (int)mobile_device_before.size();
        DAVINCI_LOG_INFO << string("-------------------------------------------------------------");
        if (device_num_before == 0)
        {
            DAVINCI_LOG_INFO << string("There is no mobile device connected with the host PC before mapping.");
        }
        if (device_num_before == 1)
        {
            DAVINCI_LOG_INFO << string("There is ") << device_num_before << string(" mobile device connected with the host PC before mapping.");
            DAVINCI_LOG_INFO << string("No.1 mobile device's name is: ") << mobile_device_before[0];
        }
        else
        {
            DAVINCI_LOG_INFO << string("There are ") << device_num_before << string(" mobile devices connected with the host PC before mapping.");
            for (int i = 1; i <= device_num_before; i++)
            {
                DAVINCI_LOG_INFO << string("No.") << i << string(" mobile device's name is: ") << mobile_device_before[i - 1];
            }
        }
        DAVINCI_LOG_INFO << string("-------------------------------------------------------------");
#pragma endregion

#pragma region //chong: Mapping process
        file_out.open(LogFile, ios::app);
        file_out << string("[Mapping]:") << endl;
        file_out.flush();
        file_out.close();
        for (int i = 1; i <= pusher_num_before; i++)
        {
            DAVINCI_LOG_INFO << string("*************************************************************");
            DAVINCI_LOG_INFO << string("Try mapping No.") << i << string(" power pusher device!");

            detailed_file_out.open(DetailedLogFile, ios::app);
            DAVINCI_LOG_INFO << string("*************************************************************");
            detailed_file_out << string("Try mapping No.") << i << string(" power pusher device!") << endl;
            detailed_file_out.flush();
            detailed_file_out.close();

            MapPusher(i);
            ThreadSleep(mapping_interval);
            DAVINCI_LOG_INFO << string("*************************************************************");
        }
        ThreadSleep(mapping_interval);      //chong: make sure the last mapping process is finished!
#pragma endregion

#pragma region //chong: Save mapping results into log file
        ThreadSleep(overall_results_interval);      //chong: make sure all devices finished rebooting, and the overall results will be right!
        file_out.open(LogFile, ios::app);

        //Update FFRD and Mobile device status
        FFRD_after = (DeviceManager::Instance()).GetHWAccessoryControllerNames();
        AndroidTargetDevice::AdbDevices(mobile_device_after);
        //pusher_num_after = FFRD_after.capacity;
        pusher_num_after = (int)FFRD_after.size();
        //device_num_after = mobile_device_after.capacity;
        device_num_after = (int)mobile_device_after.size();

#pragma region //Mapping overall results
        file_out << string("[Result]:") << endl;
        if ((pusher_num_before == pusher_num_after) && (device_num_before == device_num_after) && (map_process_result == true))
        {
            DAVINCI_LOG_INFO << string("Mapping is successful!");
            file_out << string("Mapping is successful!") << endl;
        }
        else
        {
            DAVINCI_LOG_INFO << string("Mapping is failed!");
            file_out << string("Mapping is failed!") << endl;
            if (map_process_result == false)
            {
                DAVINCI_LOG_INFO << string("Mapping failed in the short press process. Please refer to DetailedMappingLogFile.txt!");
                file_out << string("Mapping failed in the short press process. Please refer to DetailedMappingLogFile.txt!") << endl;
            }
        }
#pragma endregion

        DAVINCI_LOG_INFO << string("*************************************************************");
        file_out << string("*************************************************************") << endl;
        file_out << string("[Detailed log]:") << endl;
        DAVINCI_LOG_INFO << string("Mapping Power Pusher with Mobile Device Test Result:");
        file_out << string("Mapping Power Pusher with Mobile Device Test Result:") << endl;

#pragma region //Power pusher connected with the host PC before mapping
        if (pusher_num_before == 0)
        {
            DAVINCI_LOG_INFO << string("There is no power pusher connected with the host PC before mapping.");
            file_out << string("There is no power pusher connected with the host PC before mapping.") << endl;
        }
        if (pusher_num_before == 1)
        {
            DAVINCI_LOG_INFO << string("There is ") << pusher_num_before << string(" power pusher connected with the host PC before mapping.");
            file_out << string("There is ") << pusher_num_before << string(" power pusher connected with the host PC before mapping.") << endl;
            file_out << string("        No.1 power pusher is connected on: ") << FFRD_before[0] << endl;
        }
        else
        {
            DAVINCI_LOG_INFO << string("There are ") << pusher_num_before << string(" power pushers connected with the host PC before mapping.");
            file_out << string("There are ") << pusher_num_before << string(" power pushers connected with the host PC before mapping.") << endl;
            for (int i = 1; i <= pusher_num_before; i++)
            {
                file_out << string("        No.") << i << string(" power pusher is connected on: ") << FFRD_before[i - 1] << endl;
            }
        }
#pragma endregion

#pragma region //Power pusher connected with the host PC after mapping
        if (pusher_num_after == 0)
        {
            DAVINCI_LOG_INFO << string("There is no power pusher connected with the host PC after mapping.");
            file_out << string("There is no power pusher connected with the host PC after mapping.") << endl;
        }
        if (pusher_num_after == 1)
        {
            DAVINCI_LOG_INFO << string("There is ") << pusher_num_after << string(" power pusher connected with the host PC after mapping.");
            file_out << string("There is ") << pusher_num_after << string(" power pusher connected with the host PC after mapping.") << endl;
            file_out << string("        No.1 power pusher is connected on: ") << FFRD_after[0] << endl;
        }
        else
        {
            DAVINCI_LOG_INFO << string("There are ") << pusher_num_after << string(" power pushers connected with the host PC after mapping.");
            file_out << string("There are ") << pusher_num_after << string(" power pushers connected with the host PC after mapping.") << endl;
            for (int i = 1; i <= pusher_num_after; i++)
            {
                file_out << string("        No.") << i << string(" power pusher is connected on: ") << FFRD_after[i - 1] << endl;
            }
        }
#pragma endregion

#pragma region //Mobile device connected with the host PC before mapping
        if (device_num_before == 0)
        {
            DAVINCI_LOG_INFO << string("There is no mobile device connected with the host PC before mapping.");
            file_out << string("There is no mobile device connected with the host PC before mapping.") << endl;
        }
        if (device_num_before == 1)
        {
            DAVINCI_LOG_INFO << string("There is ") << device_num_before << string(" mobile device connected with the host PC before mapping.");
            file_out << string("There is ") << device_num_before << string(" mobile device connected with the host PC before mapping.") << endl;
            file_out << string("        No.1 mobile device's name is: ") << mobile_device_before[0] << endl;
        }
        else
        {
            DAVINCI_LOG_INFO << string("There are ") << device_num_before << string(" mobile devices connected with the host PC before mapping.");
            file_out << string("There are ") << device_num_before << string(" mobile devices connected with the host PC before mapping.") << endl;
            for (int i = 1; i <= device_num_before; i++)
            {
                file_out << string("        No.") << i << string(" mobile device's name is: ") << mobile_device_before[i - 1] << endl;
            }
        }
#pragma endregion

#pragma region //Mobile device connected with the host PC after mapping
        if (device_num_after == 0)
        {
            DAVINCI_LOG_INFO << string("There is no mobile device connected with the host PC after mapping.");
            file_out << string("There is no mobile device connected with the host PC after mapping.") << endl;
        }
        if (device_num_after == 1)
        {
            DAVINCI_LOG_INFO << string("There is ") << device_num_after << string(" mobile device connected with the host PC after mapping.");
            file_out << string("There is ") << device_num_after << string(" mobile device connected with the host PC after mapping.") << endl;
            file_out << string("        No.1 mobile device's name is: ") << mobile_device_after[0] << endl;
        }
        else
        {
            DAVINCI_LOG_INFO << string("There are ") << device_num_after << string(" mobile devices connected with the host PC after mapping.");
            file_out << string("There are ") << device_num_after << string(" mobile devices connected with the host PC after mapping.") << endl;
            for (int i = 1; i <= device_num_after; i++)
            {
                file_out << string("        No.") << i << string(" mobile device's name is: ") << mobile_device_after[i - 1] << endl;
            }
        }
        DAVINCI_LOG_INFO << string("-------------------------------------------------------------");
#pragma endregion

        DAVINCI_LOG_INFO << string("Mapping Power Pusher with Mobile Device Test is finished! Exit!");
        file_out << string("Mapping Power Pusher with Mobile Device Test is finished! Exit!") << endl;
        DAVINCI_LOG_INFO << string("*************************************************************");
        file_out << string("*************************************************************") << endl;

        file_out.flush();
        file_out.close();
#pragma endregion

        StartGroup();
        SetFinished();


        return true;
    }

    //Ready
    void MapPowerPusher::MapPusher(int COM_num)
    {
        boost::shared_ptr<HWAccessoryController> mapping_FFRD = (DeviceManager::Instance()).GetHWAccessoryControllerDevice(FFRD_before[COM_num - 1]);
        mapping_FFRD->Connect();
        vector<int> device_screen_status_before_shortpress = vector<int>();
        vector<int> device_screen_status_after_shortpress = vector<int>();

#pragma region //chong: check screen status before short press
        for (int i = 1; i <= device_num_before; i++)
        {
            int screenStatus = getScreenOnStatus(mobile_device_before[i - 1]);
            if (screenStatus == 1)
            {
                device_screen_status_before_shortpress.insert(device_screen_status_before_shortpress.end(), 1);
                DAVINCI_LOG_INFO << string("*************************************************************");
                DAVINCI_LOG_INFO << string("Before short press, the screen mode of device: ") << mobile_device_before[i - 1] << string(" is On!");
                DAVINCI_LOG_INFO << string("*************************************************************");

                ofstream temp_file_out(DetailedLogFile, ios::app);
                temp_file_out << string("Before short press, the screen mode of device: ") << mobile_device_before[i - 1] << string(" is On!") << endl;
                temp_file_out.flush();
                temp_file_out.close();
            }
            else if (screenStatus == 0)
            {
                device_screen_status_before_shortpress.insert(device_screen_status_before_shortpress.end(), 0);
                DAVINCI_LOG_INFO << string("*************************************************************");
                DAVINCI_LOG_INFO << string("Before short press, the screen mode of device: ") << mobile_device_before[i - 1] << string(" is Off!");
                DAVINCI_LOG_INFO << string("*************************************************************");

                ofstream temp_file_out(DetailedLogFile, ios::app);
                temp_file_out << string("Before short press, the screen mode of device: ") << mobile_device_before[i - 1] << string(" is Off!") << endl;
                temp_file_out.flush();
                temp_file_out.close();
            }
            else
            {
                device_screen_status_before_shortpress.insert(device_screen_status_before_shortpress.end(), -1);
                DAVINCI_LOG_INFO << string("*************************************************************");
                DAVINCI_LOG_INFO << string("Before short press, the screen mode of device: ") << mobile_device_before[i - 1] << string(" is Unknown!");
                DAVINCI_LOG_INFO << string("*************************************************************");

                ofstream temp_file_out(DetailedLogFile, ios::app);
                temp_file_out << string("Before short press, the screen mode of device: ") << mobile_device_before[i - 1] << string(" is Unknown!") << endl;
                temp_file_out.flush();
                temp_file_out.close();
            }
        }
#pragma endregion

        DAVINCI_LOG_INFO << string("Short press to change the screen mode of the device.");

        //mapping_FFRD->PressPowerButton();
        //ThreadSleep(short_press_time);
        //mapping_FFRD->ReleasePowerButton();

        //Press+ThreadSleep+Release may cause long echo time!
        mapping_FFRD->ShortPressPowerButton(300);
        ThreadSleep(safe_interval);

#pragma region //chong: check screen status after short press
        for (int k = 1; k <= device_num_before; k++)
        {
            int screenStatus = getScreenOnStatus(mobile_device_before[k - 1]);
            if (screenStatus == 1)
            {
                device_screen_status_after_shortpress.insert(device_screen_status_after_shortpress.end(), 1);
                DAVINCI_LOG_INFO << string("*************************************************************");
                DAVINCI_LOG_INFO << string("After short press, the screen mode of device: ") << mobile_device_before[k - 1] << string(" is On!");
                DAVINCI_LOG_INFO << string("*************************************************************");

                ofstream temp_file_out(DetailedLogFile, ios::app);
                temp_file_out << string("After short press, the screen mode of device: ") << mobile_device_before[k - 1] << string(" is On!") << endl;
                temp_file_out.flush();
                temp_file_out.close();
            }
            else if (screenStatus == 0)
            {
                device_screen_status_after_shortpress.insert(device_screen_status_after_shortpress.end(), 0);
                DAVINCI_LOG_INFO << string("*************************************************************");
                DAVINCI_LOG_INFO << string("After short press, the screen mode of device: ") << mobile_device_before[k - 1] << string(" is Off!");
                DAVINCI_LOG_INFO << string("*************************************************************");

                ofstream temp_file_out(DetailedLogFile, ios::app);
                temp_file_out << string("After short press, the screen mode of device: ") << mobile_device_before[k - 1] << string(" is Off!") << endl;
                temp_file_out.flush();
                temp_file_out.close();
            }
            else
            {
                device_screen_status_after_shortpress.insert(device_screen_status_after_shortpress.end(), -1);
                DAVINCI_LOG_INFO << string("*************************************************************");
                DAVINCI_LOG_INFO << string("After short press, the screen mode of device: ") << mobile_device_before[k - 1] << string(" is Unknown!");
                DAVINCI_LOG_INFO << string("*************************************************************");

                ofstream temp_file_out(DetailedLogFile, ios::app);
                temp_file_out << string("After short press, the screen mode of device: ") << mobile_device_before[k - 1] << string(" is Unknown!") << endl;
                temp_file_out.flush();
                temp_file_out.close();
            }
        }
#pragma endregion

#pragma region //chong: use list to find the device with screen status changed
        if (device_screen_status_before_shortpress.size() != device_screen_status_after_shortpress.size())
        {
            DAVINCI_LOG_INFO << string("*************************************************************");
            DAVINCI_LOG_INFO << string("Before and after short press, the length of screen mode list changed, Warning!");
            DAVINCI_LOG_INFO << string("*************************************************************");
            map_process_result = false;
        }
        else
        {
            DAVINCI_LOG_INFO << string("*************************************************************");
            DAVINCI_LOG_INFO << string("Before and after short press, the length of screen mode list unchanged!");
            DAVINCI_LOG_INFO << string("*************************************************************");
            vector<string> mapped_mobile_device = vector<string>();
            for (unsigned int j = 1; j <= device_screen_status_before_shortpress.size(); j++)
            {
                if (device_screen_status_before_shortpress[j - 1] != device_screen_status_after_shortpress[j - 1])
                {
                    mapped_mobile_device.push_back(mobile_device_before[j - 1]);
                }
            }
            if (mapped_mobile_device.size() == 1)
            {
                DAVINCI_LOG_INFO << string("List compare function is successfull!");
                DAVINCI_LOG_INFO << string("The power pusher device connected with: ") << FFRD_before[COM_num - 1] << " is mapped with mobile device: " << mapped_mobile_device[0];

                ofstream file_out(LogFile, ios::app);
                //file_out << string("The power pusher device connected with: ") << FFRD_before[COM_num - 1] << " is mapped with mobile device: " << mapped_mobile_device[0] << endl;
                file_out << FFRD_before[COM_num - 1] << ":" << mapped_mobile_device[0] << endl;
                file_out.flush();
                file_out.close();
            }
            else
            {
                DAVINCI_LOG_INFO << string("List compare function is failed, Warning!");
                map_process_result = false;       //Chong: potential bug: will cause false alarm when two power pusher with only one mobile device! But it is the unusual use case.
            }
        }
#pragma endregion
    }

    //Ready
    cv::Mat MapPowerPusher::ProcessFrame(const cv::Mat &frame, const double timeStamp)
    {
        return frame;
    }

    //Ready
    void MapPowerPusher::Destroy()
    {
        DAVINCI_LOG_INFO << string("Mapping Power Pusher with Mobile Device Finish.");
    }

    //Ready
    CaptureDevice::Preset MapPowerPusher::CameraPreset()
    {
        return CaptureDevice::Preset::PresetDontCare;
    }

    //Ready
    int MapPowerPusher::fileExists(const string &fileName)
    {
        fstream _file;
        _file.open(fileName, ios::in);
        if(!_file)
        {
            //file not exists
            return 0;
        }
        else
        {
            //file exists
            return 1;
        }
    }

    //Ready (with the help from Jacky)
    int MapPowerPusher::getScreenOnStatus(const string &deviceName)
    {		
        int screenStatus = -1;
        string adbCommand = "  shell dumpsys display";
        DaVinciStatus status;

        auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
        status = AndroidTargetDevice::AdbCommandNoDevice("-s " + deviceName + adbCommand, outStr, 5000);
        if (!DaVinciSuccess(status))
        {
            screenStatus = -1;
            return screenStatus;
        }

        if (outStr != nullptr)
        {
            vector<string> dumpsysDisplayStr;
            ReadAllLinesFromStr(outStr->str(), dumpsysDisplayStr);
            if (!dumpsysDisplayStr.empty())
            {
                for(int index = 0; index < (int)(dumpsysDisplayStr.size()); index++)
                {
                    string line = dumpsysDisplayStr[index];
                    boost::trim(line);
                    if(boost::contains(line, "mScreenState=ON"))		//N5
                    {
                        screenStatus = 1;
                        break;
                    }
                    if (boost::contains(line, "mScreenState=OFF"))		//N5
                    {
                        screenStatus = 0;
                        break;
                    }
                    if (boost::contains(line, "mBlanked=false"))        //N7
                    {
                        screenStatus = 1;
                        break;
                    }
                    if (boost::contains(line, "mBlanked=true"))         //N7
                    {
                        screenStatus = 0;
                        break;
                    }
                }
            }
            else
            {
                DAVINCI_LOG_WARNING << string("Get no information when executing dumpsys display!");
                screenStatus = -1;
            }
        }
        else
        {
            DAVINCI_LOG_WARNING << string("Get no information when executing dumpsys display!");
            screenStatus = -1;
        }
        return screenStatus;
    }   

}