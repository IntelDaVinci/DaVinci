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
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Drawing.Imaging;

namespace DaVinci
{
    class DaVinciCommon
    {
        [DllImport("kernel32.dll", EntryPoint = "CopyMemory")]
        public static extern void CopyMemory(IntPtr dest, IntPtr src, uint count);

        /// <summary>
        /// 
        /// </summary>
        /// <param name="src"></param>
        /// <returns></returns>
        public static Bitmap ConvertToBitmap(IntPtr src)
        {
            WIplImage image = (WIplImage)Marshal.PtrToStructure(src, typeof(WIplImage));
            unsafe
            {
                PixelFormat format = PixelFormat.Format24bppRgb;
                if (image.nChannels == 1)
                {
                    format = PixelFormat.Format8bppIndexed;
                }

                try
                {
                    Bitmap dst = new Bitmap(image.width, image.height, image.widthStep,
                        format, (System.IntPtr)image.imageData);
                    return dst;
                }
                catch
                {
                    Bitmap dst = new Bitmap(image.width, image.height, format);
                    try
                    {
                        BitmapData data = dst.LockBits(new Rectangle(0, 0, dst.Width, dst.Height), ImageLockMode.WriteOnly, format);
                        for (int i = 0; i < image.height; i++)
                        {
                            CopyMemory(data.Scan0 + (i * data.Stride), ((System.IntPtr)image.imageData) + (i * image.widthStep), (uint)(image.width * image.nChannels));
                        }
                        dst.UnlockBits(data);
                        return dst;
                    }
                    catch (Exception e)
                    {
                        dst.Dispose();
                        throw e;
                    }
                }
            }
        }

        public static Color IntelGreen = Color.FromArgb(255, 166, 206, 57);
        public static Color IntelRed = Color.FromArgb(255, 237, 28, 36);
        public static Color IntelYellow = Color.FromArgb(255, 255, 218, 0);
        public static Color IntelWhite = Color.FromArgb(255, 255, 255, 255);
        public static Color IntelBlue = Color.FromArgb(255, 0, 113, 197);
        public static Color IntelLightGrey = Color.FromArgb(255, 177, 186, 191);
        public static Color IntelBlack = Color.FromArgb(255, 0, 0, 0);
    }
}
