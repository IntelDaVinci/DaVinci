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
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
//using System.Windows.Controls;
using System.Windows.Forms;
//using System.Windows.Documents;
using System.Runtime.InteropServices; // GCHandle
using System.Threading;
using System.Xml;
using System.Timers;
using System.Diagnostics;
using System.Reflection;
using System.Web;
using System.Collections.Specialized;

using System.Management;

namespace DaVinci
{
    /// <summary>
    /// Provide test utility functions
    /// </summary>
    public class TestUtil
    {
        /// <summary>
        /// 
        /// </summary>
        public static string[] targetDevices = null;
        /// <summary>
        /// 
        /// </summary>
        public static string[] fFRDDevices = null;
        /// <summary>
        /// 
        /// </summary>
        public static string[] captureDevices = null;

        /// <summary> The audio record devices. </summary>
        public static string[] audioRecordDevices = null;
        /// <summary> The audio play devices. </summary>
        public static string[] audioPlayDevices = null;

        private static String DaVinciDir = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);

        /// <summary>
        /// Enable or disable the children of a control.
        /// </summary>
        /// <param name="p">The parent control</param>
        /// <param name="enable">To enable or disable</param>
        /// <param name="list">The list to disable or enable</param>
        public static void EnableDisableControls(Control p, bool enable, List<String> list = null)
        {
            foreach (Control c in p.Controls.Cast<Control>())
            {
                if (list != null && list.Contains(c.Text))
                    c.Enabled = !enable;
                else
                {
                    EnableDisableControls(c, enable);
                    c.Enabled = enable;
                }
                if (c is ToolStrip)
                {
                    ToolStrip ts = (ToolStrip)c;
                    foreach (ToolStripItem item in ts.Items.Cast<ToolStripItem>())
                    {
                        item.Enabled = enable;
                    }
                }
            }
        }

        /// <summary> The maximum length of native string. </summary>
        public const int maxLen = 1024;

        private delegate int GetStringFunc(
            [In, Out, MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)] string[] array, int length, int arraySize);

        private static void GetStringArrayFromNative(out string[] stringArray, GetStringFunc callback)
        {
            int arraySize = 0;
            arraySize = callback(null, maxLen, arraySize);
            if (arraySize > 0)
            {
                string[] devices = new string[arraySize];
                for (int i = 0; i < arraySize; i++)
                {
                    devices[i] = new StringBuilder().Append('c', maxLen).ToString();
                }
                callback(devices, maxLen, arraySize);
                stringArray = new string[arraySize];
                for (int i = 0; i < arraySize; i++)
                {
                    stringArray[i] = devices[i].ToString();
                }
            }
            else
            {
                stringArray = new string[0];
            }
        }

        /// <summary>
        /// 
        /// </summary>
        public static void RefreshTestDevices()
        {
            // Target devices are refreshed by devMonitor. No need to refresh here.
            try
            {
                DaVinciAPI.NativeMethods.RefreshTargetDevice();
            }
            catch (Exception)
            {
                Console.WriteLine("Exception when refreshing target device!");
            }

            GetStringArrayFromNative(out captureDevices, DaVinciAPI.NativeMethods.GetCaptureDeviceNames);
            GetStringArrayFromNative(out fFRDDevices, DaVinciAPI.NativeMethods.GetHWAccessoryControllerNames);
            GetStringArrayFromNative(out targetDevices, DaVinciAPI.NativeMethods.GetTargetDeviceNames);
            GetStringArrayFromNative(out audioRecordDevices, DaVinciAPI.NativeMethods.GetAudioRecordDeviceNames);
            GetStringArrayFromNative(out audioPlayDevices, DaVinciAPI.NativeMethods.GetAudioPlayDeviceNames);
        }

        /// <summary>
        /// Represent an image specified in IF_MATCH opcodes.
        /// </summary>
        public class ImageObject
        {
            /// <summary>
            /// The path to the image
            /// </summary>
            public string ImageName;
            /// <summary>
            /// Valid value ranges from 0 to 359. Ignored if not valid.
            /// If valid, the angle is relative to the landscape frame clockwise.
            /// </summary>
            public int Angle = -1;
            /// <summary>
            /// If Angle is valid, it specifies the error allowed to match
            /// the angle.
            /// </summary>
            public int AngleError = 10;
            /// <summary>
            /// Valid value ranges from 0 to 1.0. Ignored if not valid.
            /// If valid, the ratio means percentage matched feature points between reference and observation.
            /// </summary>
            public float RatioReferenceMatch = -1.0f;

            /// <summary>
            /// Parse the operand string provided to IF_MATCH opcode to get
            /// image object components.
            /// </summary>
            /// <param name="operand">A string separated by "|"</param>
            /// <returns></returns>
            public static ImageObject Parse(string operand)
            {
                ImageObject theObject = new ImageObject();
                if (operand.IndexOf('|') > 0)
                {
                    string[] components = operand.Split(new char[] { '|' });
                    theObject.ImageName = components[0];
                    if (components.Length > 1)
                    {
                        string[] angleAndError = components[1].Split(new char[] { ',' });
                        int.TryParse(angleAndError[0], out theObject.Angle);
                        if (angleAndError.Length > 1)
                        {
                            int.TryParse(angleAndError[1], out theObject.AngleError);
                        }
                    }
                }
                else
                {
                    var baseUri = new Uri("file://");
                    var uri = new Uri(baseUri, operand);
                    if (uri.Segments.Count() > 1)
                    {
                        theObject.ImageName = uri.Segments[1];
                        NameValueCollection query = HttpUtility.ParseQueryString(uri.Query);
                        if (query["ratioReferenceMatch"] != null)
                        {
                            var val = Convert.ToDouble(query["ratioReferenceMatch"]);
                            theObject.RatioReferenceMatch = (float)val;
                        }
                        if (query["angle"] != null)
                        {
                            Int32.TryParse(query["angle"], out theObject.Angle);
                        }
                        if (query["angleError"] != null)
                        {
                            Int32.TryParse(query["angleError"], out theObject.Angle);
                        }
                    }
                }
                return theObject;
            }

            /// <summary>
            /// Get the required string format in IF_MATCH opcodes
            /// </summary>
            /// <returns></returns>
            public override string ToString()
            {
                string ret = ImageName;
                bool appendRatio = false;
                if (RatioReferenceMatch > .0f &&
                    RatioReferenceMatch <= 1.0f)
                {
                    appendRatio = true;
                }
                if (Angle >= 0)
                {
                    ret += "?angle=" + Angle;
                    if (AngleError != 10)
                    {
                        ret += "&angleError=" + AngleError;
                    }
                    if (appendRatio)
                    {
                        ret += "&ratioReferenceMatch=" + RatioReferenceMatch;
                    }
                }
                else if (appendRatio)
                {
                    ret += "?ratioReferenceMatch=" + RatioReferenceMatch;
                }
                return ret;
            }
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="node"></param>
        /// <returns></returns>
        public static string getNodeFullPath(TreeNode node)
        {
            if (node == null)
                return null;
            if (node.Level > 0)
            {
                return (Path.Combine(getNodeFullPath(node.Parent), node.Text));
            }
            else
            {
                return MainWindow_V.currentTestProjectConfig.BaseFolder;
            }
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="node"></param>
        /// <returns></returns>
        public static bool isQSTreeNode(TreeNode node)
        {
            if (node != null)
                return (node.Text.EndsWith(".qs")||node.Text.EndsWith(".xml"));
            return false;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="node"></param>
        /// <returns></returns>
        public static string getNodeFullDirectoryPath(TreeNode node)
        {
            string path = getNodeFullPath(node);
            if (isQSTreeNode(node))
            {
                return Path.GetDirectoryName(path);
            }
            else
            {
                return path;
            }
        }

        delegate bool AddControlValueUntilCallback(Control oControl, string propName, int max);
        /// <summary>
        /// 
        /// </summary>
        /// <param name="oControl"></param>
        /// <param name="propName"></param>
        /// <param name="max"></param>
        /// <returns></returns>
        public static bool AddControlPropertyValueUntil(Control oControl, string propName, int max)
        {
            if (oControl.InvokeRequired)
            {
                AddControlValueUntilCallback d = new AddControlValueUntilCallback(AddControlPropertyValueUntil);
                try
                {
                    return (bool)(oControl.Invoke(d, new object[] { oControl, propName, max }));
                }
                catch
                {
                    return false;
                }
            }
            else
            {
                Type t = oControl.GetType();
                PropertyInfo[] props = t.GetProperties();
                PropertyInfo valueP = null;
                foreach (PropertyInfo p in props)
                {
                    if (p.Name.ToUpper() == propName.ToUpper())
                    {
                        valueP = p;
                    }
                }
                if (valueP != null)
                {
                    int oldValue = (int)valueP.GetValue(oControl, null);
                    if (oldValue < max)
                    {
                        valueP.SetValue(oControl, oldValue + 1, null);
                        return true;
                    }
                }
                return false;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        public static bool isValidFileName(string name)
        {
            char[] invalidChars = Path.GetInvalidFileNameChars();
            bool isValid = true;
            foreach (char ch in invalidChars)
            {
                if (name.Contains(ch))
                {
                    isValid = false;
                    MessageBox.Show("Folder or file name should not contain invalid characters!", 
                        "Warning", 
                        MessageBoxButtons.OK, 
                        MessageBoxIcon.Exclamation);
                    break;
                }
            }
            return isValid;
        }
    }
}
