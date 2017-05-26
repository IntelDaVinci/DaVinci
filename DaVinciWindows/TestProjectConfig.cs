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

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Serialization;

namespace DaVinci
{
    /// <summary>
    /// The configuration for a test project.
    /// </summary>
    [XmlRoot("DaVinciTestConfig")]
    public class TestProjectConfig
    {
        /// <summary>
        /// The type of OS the project is testing on.
        /// </summary>
        public enum OperatingSystemType
        {
            /// <summary>
            /// Android OS
            /// </summary>
            [XmlEnum("android")]
            Android,
            /// <summary>
            /// Windows OS
            /// </summary>
            [XmlEnum("windows")]
            Windows,
            /// <summary>
            /// Windows Mobile OS
            /// </summary>
            [XmlEnum("WindowsMobile")]
            WindowsMobile,
            /// <summary>
            /// Chromium OS
            /// </summary>
            [XmlEnum("chome")]
            ChromeOS,
            /// <summary>
            /// iOS
            /// </summary>
            [XmlEnum("iOS")]
            iOS
        }

        /// <summary>
        /// The type of OS the project is testing on.
        /// </summary>
        [XmlAttribute("os")]
        [DefaultValue(OperatingSystemType.Android)]
        public OperatingSystemType OSType;

        /// <summary>
        /// Name of the project.
        /// </summary>
        [XmlElement("name")]
        public string Name;

        /// <summary>
        /// For Android, it's the name of the APK file name being tested.
        /// The file exists in the same folder as the test project config file.
        /// 
        /// For Windows, it's the full path to the device side app being tested.
        /// 
        /// For ChromeOS, it's the ID of the app being tested.
        /// </summary>
        [XmlElement("application")]
        public string Application;

        /// <summary>
        /// For Android only, the package name of the app being tested.
        /// </summary>
        [XmlElement("package")]
        public string PackageName;

        /// <summary>
        /// For Android only, When not null, the default activity name of the app.
        /// </summary>
        [XmlElement("activity")]
        public string ActivityName;

        /// <summary>
        /// The source path for pushing data.
        /// </summary>
        [XmlIgnore]
        public string PushDataSource;

        /// <summary>
        /// The target path for pushing data.
        /// </summary>
        [XmlIgnore]
        public string PushDataTarget;

        /// <summary>
        /// The base folder where the test configuration is loaded.
        /// </summary>
        [XmlIgnore]
        public string BaseFolder;

        /// <summary>
        /// Load a test project configuration from a stream.
        /// </summary>
        /// <param name="configStream"></param>
        /// <returns></returns>
        public static TestProjectConfig Load(Stream configStream)
        {
            XmlSerializer serializer = new XmlSerializer(typeof(TestProjectConfig));
            return (TestProjectConfig)serializer.Deserialize(configStream);
        }

        /// <summary>
        /// Save test project configuration to a file
        /// </summary>
        /// <param name="configFile"></param>
        public void Save(string configFile)
        {
            using (FileStream configFileStream = new FileStream(configFile, FileMode.Create, FileAccess.Write))
            {
                this.Save(configFileStream);
            }
        }

        /// <summary>
        /// Save test project configuration to a stream
        /// </summary>
        /// <param name="configStream"></param>
        public void Save(Stream configStream)
        {
            XmlSerializerNamespaces ns = new XmlSerializerNamespaces();
            ns.Add("", "");
            XmlSerializer serializer = new XmlSerializer(typeof(TestProjectConfig));
            serializer.Serialize(configStream, this,ns);
        }

    }
}
