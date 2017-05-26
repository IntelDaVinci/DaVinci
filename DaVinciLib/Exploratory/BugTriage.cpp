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

#include "BugTriage.hpp"
#include "TestManager.hpp"
#include "DaVinciCommon.hpp"
#include "unzipper.hpp"

using namespace ziputils;
using namespace std;

namespace DaVinci
{
    BugTriage::BugTriage()
    {
    }

    list<MiddlewareInfo> BugTriage::InitMiddlewareInfo()
    {
        std::unordered_map<string, string> dicLibInfo;
        dicLibInfo.insert(make_pair("Unity", "libmono.so&&libunity.so"));
        dicLibInfo.insert(make_pair("RenderScript", "librsjni.so&&libRSSupport.so"));
        dicLibInfo.insert(make_pair("Cocos2d-x", "libcocos2dcpp.so"));
        dicLibInfo.insert(make_pair("Google Play Games Services", "libgpg.so"));
        dicLibInfo.insert(make_pair("WebP", "libwebp.so"));
        dicLibInfo.insert(make_pair("LibGDX", "libgdx.so"));
        dicLibInfo.insert(make_pair("SoundTouch", "libsoundtouch001.so"));
        dicLibInfo.insert(make_pair("OpenCV", "libopencv_core.so&&libopencv_imgproc.so"));
        dicLibInfo.insert(make_pair("FMOD", "libfmodex.so"));
        dicLibInfo.insert(make_pair("ffmpeg", "libffmpeg.so"));
        dicLibInfo.insert(make_pair("EveryPlay", "libeveryplay.so"));
        dicLibInfo.insert(make_pair("card.io", "libcardioDecider.so&&libcardioRecognizer_tegra2.so&&libcardioRecognizer.so"));
        dicLibInfo.insert(make_pair("GPUImage", "libgpuimage-library.so"));
        dicLibInfo.insert(make_pair("Adobe Air", "libCore.so"));
        dicLibInfo.insert(make_pair("Android GifDrawable", "libgif.so"));
        dicLibInfo.insert(make_pair("OpenAL", "libopenal.so"));
        dicLibInfo.insert(make_pair("Nuance Mobile SDK", "libnmsp_speex.so"));
        dicLibInfo.insert(make_pair("MP3 LAME", "libmp3lame.so"));
        dicLibInfo.insert(make_pair("OpenSSL", "libcrypto.so"));
        dicLibInfo.insert(make_pair("Marmalade SDK", "libs3edialog_ext.so"));
        dicLibInfo.insert(make_pair("Aviary", "libaviary_moalite.so&&libaviary_native.so"));
        dicLibInfo.insert(make_pair("SQLite3", "libsqlite3.so"));
        dicLibInfo.insert(make_pair("Sonic", "libsonic.so"));
        dicLibInfo.insert(make_pair("Voxel SDK", "libvxlnative.so"));
        dicLibInfo.insert(make_pair("Conceal", "libconceal.so"));
        dicLibInfo.insert(make_pair("Kamcord", "libkamcord.so&&libkamcordcore.so"));
        dicLibInfo.insert(make_pair("Baidu GeoLocation SDK", "liblocSDK3.so"));
        dicLibInfo.insert(make_pair("SQLCipher", "libdatabase_sqlcipher.so&&libsqlcipher_android.so"));
        dicLibInfo.insert(make_pair("cURL", "libcurl.so"));
        dicLibInfo.insert(make_pair("Umeng SDK", "libbspatch.so"));
        dicLibInfo.insert(make_pair("Chipmunk", "libchipmunk.so"));
        dicLibInfo.insert(make_pair("BASS", "libbass.so"));
        dicLibInfo.insert(make_pair("Unreal Engine", "libUnrealEngine3.so"));
        dicLibInfo.insert(make_pair("RedLaser", "libredlaser.so"));
        dicLibInfo.insert(make_pair("AndEngine", "libandengine.so"));
        dicLibInfo.insert(make_pair("ZBar", "libiconv.so&&libzbarjni.so"));
        dicLibInfo.insert(make_pair("Monogame", "libmonodroid.so&&libmonosgen-2.0.so"));
        dicLibInfo.insert(make_pair("MobileAppTracking Unity Plugin", "libmobileapptracker.so"));
        dicLibInfo.insert(make_pair("JavaCV", "libjniARToolKitPlus.so&&libjniavcodec.so&&libjniavdevice.so&&libjniavfilter.so&&libjniavformat.so&&libjniavutil.so&&libjnicvkernels.so&&libjniopencv_core.so&&libjniopencv_imgproc.so&&libjniswresample.so&&libjniswscale.so"));
        dicLibInfo.insert(make_pair("Wi Engine", "libwiengine_binding.so&&libwiengine.so&&libwinetwork.so&&libwiskia.so&&libwisound.so"));
        dicLibInfo.insert(make_pair("Intel TBB", "libtbb.so"));
        dicLibInfo.insert(make_pair("T-Store IAB SDK", "libdodo.so&&libUSToolkit.so"));
        dicLibInfo.insert(make_pair("Apportable", "libCoreGraphics.so&&libdispatch.so&&libFoundation.so&&libobjc.so&&libopenal.so"));
        dicLibInfo.insert(make_pair("libxml2", "libxml2.so"));
        dicLibInfo.insert(make_pair("GameMaker: Studio", "libopenal.so&&libyoyo.so"));
        dicLibInfo.insert(make_pair("libaal", "libaal_honeycomb.so&&libaal_jellybean.so&&libaal.so"));
        dicLibInfo.insert(make_pair("Immersion Haptic SDK", "libImmEmulatorJ.so"));
        dicLibInfo.insert(make_pair("GL2-android", "libandroidgl20.so"));
        dicLibInfo.insert(make_pair("Xamarin", "libmonodroid.so"));
        dicLibInfo.insert(make_pair("MM ANE", "libcasdkjni.so&&libidentifyapp.so"));
        dicLibInfo.insert(make_pair("Box2D", "libbox2d.so"));
        dicLibInfo.insert(make_pair("Speex", "libspeex.so"));

        list<MiddlewareInfo> middlewares;

        for(auto pair : dicLibInfo)
        {
            MiddlewareInfo m;
            std::vector<string> libs;
            std::unordered_map<string, bool> libInfo;
            m.middlewareName = pair.first;
            boost::split(libs, pair.second, boost::is_any_of("&&"));

            for (string lib : libs)
            {
                if (!lib.empty())
                    libInfo.insert(make_pair(lib, false));    
            }
            m.libsInfo = libInfo;
            middlewares.push_back(m);
        }

        return middlewares;
    }

    std::unordered_map<string, bool> BugTriage::TriageMiddlewareInfo(const std::string &apkPath)
    {
        list<MiddlewareInfo> dicLibInfo = InitMiddlewareInfo();
        list<MiddlewareInfo> updateDicLibInfo;

        unzipper zipFile;
        zipFile.open(apkPath.c_str());
        auto filenames = zipFile.getFilenames();
        zipFile.close();

        for (MiddlewareInfo m : dicLibInfo)
        {
            for (auto it = filenames.begin(); it != filenames.end(); it++)
            {
                std::string entryName = (*it).c_str();
                std::unordered_map<string, bool>::iterator iterator = m.libsInfo.begin();
                while (iterator != m.libsInfo.end())
                {
                    if (boost::ends_with(entryName, (*iterator).first))
                        (*iterator).second = true;          // if file name ends with (xxx.so), mark it as true
                    ++iterator;
                }
            }
            updateDicLibInfo.push_back(m);
        }

        std::unordered_map<string, bool> dicTriageResult;

        for (MiddlewareInfo m : updateDicLibInfo)
        {
            bool result = true;
            for (auto libs : m.libsInfo)
            {
                result = result && libs.second;             // use && means all the so file should be found
            }
            dicTriageResult.insert(make_pair(m.middlewareName, result));
        }

        return dicTriageResult;
    }


    std::unordered_map<string, bool> BugTriage::TriageHoudiniBug(const std::string &apkPath, std::string &missingLibName)
    {
        std::unordered_map<string, bool> dicTriageResult;
        vector<string> x86Libs;
        vector<string> armV7aLibs;
        vector<string> armV5Libs;

        bool flag_QQProtection_main = false;
        bool flag_QQProtection_shell = false;

        dicTriageResult.insert(make_pair("BangBangProtection", false));
        dicTriageResult.insert(make_pair("WangQinShield", false));
        dicTriageResult.insert(make_pair("iJiaMi", false));
        dicTriageResult.insert(make_pair("360Protection", false));
        dicTriageResult.insert(make_pair("360ProtectionV2", false));
        dicTriageResult.insert(make_pair("BangBangProtectionPaid", false));
        dicTriageResult.insert(make_pair("NaGaProtection", false));
        dicTriageResult.insert(make_pair("CitrixXenMobile", false));
        dicTriageResult.insert(make_pair("DexMaker", false));
        dicTriageResult.insert(make_pair("CMCCBillingSDK", false));
        dicTriageResult.insert(make_pair("Payegis", false));
        dicTriageResult.insert(make_pair("QQProtection", false));
        dicTriageResult.insert(make_pair("BaiduProtection", false));
        dicTriageResult.insert(make_pair("VKey", false));
        dicTriageResult.insert(make_pair("Wellbia", false));
        dicTriageResult.insert(make_pair("AliProtection", false));
        dicTriageResult.insert(make_pair("HyperTechProtection", false));
        dicTriageResult.insert(make_pair("MissX86", false));

        unzipper zipFile;
        zipFile.open(apkPath.c_str());
        auto filenames = zipFile.getFilenames();
        zipFile.close();
        for ( auto it = filenames.begin(); it != filenames.end(); it++ )
        {
            std::string entryName = (*it).c_str();
            boost::filesystem::path p(entryName);
            std::string dirName = p.parent_path().string();
            std::string fileName = p.filename().string();
            boost::to_lower(dirName);

            // collect all the so under x86 folder, armeabi-v7a folder and armeabi folder
            if (dirName.find("x86")!= string::npos && entryName.find(".so")!= string::npos)
            {
                x86Libs.push_back(fileName);
            }
            else if (dirName.find("armeabi-v7a")!= string::npos && entryName.find(".so")!= string::npos)
            {
                armV7aLibs.push_back(fileName);
            }
            else if (dirName.find("armeabi")!= string::npos && entryName.find(".so")!= string::npos)
            {
                armV5Libs.push_back(fileName);
            }
            else
            {
                ;
            }

            // check all kinds of protection
            if (boost::ends_with(entryName, "libsecexe.so") || boost::ends_with(entryName, "libsecmain.so")  || boost::ends_with(entryName, "libsecexe.x86.so") || boost::ends_with(entryName, "libsecmain.x86.so")  || boost::ends_with(entryName, "libsecpreload.x86.so")  || boost::ends_with(entryName, "libsecpreload.so"))
            {
                dicTriageResult["BangBangProtection"] = true;
            }
            else if (boost::ends_with(entryName, "libnqshield.so") || boost::ends_with(entryName, "libnqshieldx86.so"))
            {
                dicTriageResult["WangQinShield"] = true;
            }
            else if (boost::ends_with(entryName, "libexec.so") || boost::ends_with(entryName, "libexecmain.so"))
            {
                dicTriageResult["iJiaMi"] = true;
            }
            else if (boost::ends_with(entryName, "libprotectClass.so") || boost::ends_with(entryName, "libprotectClass_x86.so"))
            {
                dicTriageResult["360Protection"] = true;
            }
            else if (boost::ends_with(entryName, "libjiagu_art.so") || boost::ends_with(entryName, "libjiagu.so"))
            {
                dicTriageResult["360ProtectionV2"] = true;
            }
            else if (boost::ends_with(entryName, "libDexHelper.so") || boost::ends_with(entryName, "libDexHelper-x86.so"))
            {
                dicTriageResult["BangBangProtectionPaid"] = true;
            }
            else if (boost::ends_with(entryName, "libchaosvmp.so"))
            {
                dicTriageResult["NaGaProtection"] = true;
            }
            else if (boost::ends_with(entryName, "libCtxTFE.so"))
            {
                dicTriageResult["CitrixXenMobile"] = true;
            }
            else if (boost::ends_with(entryName, "dexmaker.jar"))
            {
                dicTriageResult["DexMaker"] = true;
            }
            else if (boost::ends_with(entryName, "libmegjb.so"))
            {
                dicTriageResult["CMCCBillingSDK"] = true;
            }
            else if (boost::ends_with(entryName, "libegis-x86.so") || boost::ends_with(entryName, "libegis.so") || boost::ends_with(entryName, "libegis_security.so"))
            {
                dicTriageResult["Payegis"] = true;
            }
            else if (boost::ends_with(entryName, "libmain.so"))
            {
                flag_QQProtection_main = true;
            }
            else if (boost::ends_with(entryName, "libshell.so"))
            {
                flag_QQProtection_shell = true;
            }
            else if (boost::ends_with(entryName, "libbaiduprotect_x86.so") || boost::ends_with(entryName, "libbaiduprotect.so"))
            {
                dicTriageResult["BaiduProtection"] = true;
            }
            else if (boost::ends_with(entryName, "libVosWrapper.so"))
            {
                dicTriageResult["VKey"] = true;
            }
            else if (boost::ends_with(entryName, "libxigncode.so") || boost::ends_with(entryName, "libgabriel.so") || boost::ends_with(entryName, "libpickle.so"))
            {
                dicTriageResult["Wellbia"] = true;
            }
            else if (boost::ends_with(entryName, "libmobisecx.so") || boost::ends_with(entryName, "libmobisec.so")|| boost::ends_with(entryName, "libmobisecy1.so")|| boost::ends_with(entryName, "libmobisecz1.so"))
            {
                dicTriageResult["AliProtection"] = true;
            }
            else if (boost::starts_with(entryName, "lib__") && boost::ends_with(entryName, "__.so") )
            {
                dicTriageResult["HyperTechProtection"] = true;
            }
            else
            {
                ;
            }
        }

        if (flag_QQProtection_main && flag_QQProtection_shell)
        {
            dicTriageResult["QQProtection"] = true;
        }

        // check missing x86 lib or not
        if (JudgeMissingX86lib(x86Libs, armV7aLibs, armV5Libs, missingLibName) == true)
        {
            dicTriageResult["MissX86"] = true;
        }

        return dicTriageResult;
    }

    
    bool BugTriage::JudgeMissingX86lib(vector<string> x86Libs, vector<string> armV7aLibs, vector<string> armV5Libs, std::string &missingLibName)
    {
        bool missX86 = false;
        bool needToCheck = false;
        vector<string> armLibs;

        // if apk both contains x86 folder and arm folder, need to check x86missing, else needn't to check
        if (x86Libs.size() > 0 && armV7aLibs.size() > 0)
        {
            needToCheck = true;
            armLibs = armV7aLibs;       // if contains V7, check V7, else check V5
        }
        else if (x86Libs.size() > 0 && armV5Libs.size() > 0)
        {
            needToCheck = true;
            armLibs = armV5Libs;
        }
        else
        {
            needToCheck = false;
        }

        bool alreadyCheckUserLib = false;
        bool containsUserLib = false;
        if (needToCheck)
        {
            // get third party so in ThirdPartySO.txt
            string thirdPartySOFile = TestManager::Instance().GetDaVinciResourcePath("ThirdPartySO.txt");
            vector <string> thirdPartySOs;
            ReadAllLines(thirdPartySOFile, thirdPartySOs);

            // if v7 or v5 lib is 3rd lib, need a same name lib under x86
            // for the other user libs, need at least one lib under x86 
            for (unsigned int index = 0; index < armLibs.size(); index ++)
            {
                // 3rd lib
                if (ContainVectorElement(thirdPartySOs, armLibs[index]))
                {
                    if (!ContainVectorElement(x86Libs, armLibs[index]))
                    {
                        missX86 = true;
                        missingLibName = armLibs[index];
                        break;
                    }
                }
                else // user lib, need at least one lib under x86 
                {
                    alreadyCheckUserLib = true;
                    if (ContainVectorElement(x86Libs, armLibs[index]))
                    {
                        containsUserLib = true;
                    }
                    else
                    {
                        missingLibName = armLibs[index];
                    }
                }
            }

            if (alreadyCheckUserLib == true && containsUserLib == false)
            {
                missX86 = true;
            }
        }
        return missX86;
    }
}
