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
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Xml.Serialization;
using System.Windows.Forms;
using System.ComponentModel;

namespace DaVinci
{
    /// <summary>
    /// The configure of DaVinci
    /// </summary>
    [XmlRoot("DaVinciConfig")]
    public class DaVinciConfig
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
            /// Chromium OS
            /// </summary>
            [XmlEnum("chome")]
            ChromeOS
        };

        /// <summary>
        /// Orientation of target device
        /// </summary>
        public enum Orientation
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
            Unknown
        };

        /// <summary>
        /// Change interger to Orientation
        /// </summary>
        /// <param name="i"></param>
        /// <returns></returns>
        public static Orientation IntergerToOrientation(int i)
        {
            if (i == 0)
                return Orientation.Portrait;
            else if (i == 1)
                return Orientation.Landscape;
            else if (i == 2)
                return Orientation.ReversePortrait;
            else if (i == 3)
                return Orientation.ReverseLandscape;
            else
                return Orientation.Unknown;
        }

        /// <summary>
        /// Check if the given orientation is in horizontal (either reverse or not)
        /// </summary>
        /// <param name="orientation"></param>
        /// <returns></returns>
        public static bool IsHorizontalOrientation(Orientation orientation)
        {
            if (orientation == Orientation.Landscape ||
                orientation == Orientation.ReverseLandscape)
                return true;
            else
                return false;
        }

        /// <summary>
        /// Check if the given orientation is reverse or not.
        /// </summary>
        /// <param name="orientation"></param>
        /// <returns></returns>
        public static bool IsReverseOrientation(Orientation orientation)
        {
            if (orientation == Orientation.ReverseLandscape ||
                orientation == Orientation.ReversePortrait)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

    }
}
