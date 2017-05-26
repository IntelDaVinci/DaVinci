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
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace DaVinci
{
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    internal unsafe struct WIplImage
    {
        public int nSize;
        public int ID;
        public int nChannels;
        public int alphaChannel;
        public int depth;
        public fixed byte colorModel[4];
        public fixed byte channelSeq[4];
        public int dataOrder;
        public int origin;
        public int align;
        public int width;
        public int height;
        public void* roi;
        public void* maskROI;
        public void* imageId;
        public void* tileInfo;
        public int imageSize;
        public byte* imageData;
        public int widthStep;
        public fixed int BorderMode[4];
        public fixed int BorderConst[4];
        public byte* imageDataOrigin;
    }

    /// <summary>
    /// 
    /// </summary>
    partial class ImageBox : PictureBox
    {
        public enum PaintStretchMode
        {
            STRETCH_ANDSCANS = 1,
            STRETCH_ORSCANS = 2,
            STRETCH_DELETESCANS = 3,
            STRETCH_HALFTONE = 4,
        }

        public enum PaintRasterOperations
        {
            SRCCOPY = 0x00CC0020,
            SRCPAINT = 0x00EE0086,
            SRCAND = 0x008800C6,
            SRCINVERT = 0x00660046,
            SRCERASE = 0x00440328,
            NOTSRCCOPY = 0x00330008,
            NOTSRCERASE = 0x001100A6,
            MERGECOPY = 0x00C000CA,
            MERGEPAINT = 0x00BB0226,
            PATCOPY = 0x00F00021,
            PATPAINT = 0x00FB0A09,
            PATINVERT = 0x005A0049,
            DSTINVERT = 0x00550009,
            BLACKNESS = 0x00000042,
            WHITENESS = 0x00FF0062,
        };

        DaVinciAPI.ImageInfo imageToDraw;
        /// <summary>
        /// 
        /// </summary>
        public ImageBox()
        {
            InitializeComponent();

            SetStyle(ControlStyles.UserPaint, true);
        }
        /// <summary>
        /// The interval is used to control how long (in seconds) the frameCount is reset to zero.
        /// This acts as a moving average of the FPS to keep a steady refreshing rate.
        /// </summary>
        private const long frameCountResetInterval = 5;
        /// <summary> Number of frames being refreshed so far within frameCountResetInterval. </summary>
        private long frameCount = 0;
        /// <summary> The time when the first frame is refreshed within frameCountResetInterval. </summary>
        private DateTime frameCountingStart;
        /// <summary>
        /// 
        /// </summary>
        public new DaVinciAPI.ImageInfo Image
        {
            get
            {
                return imageToDraw;
            }
            set
            {
                DaVinciAPI.NativeMethods.ReleaseImage(ref imageToDraw);
                imageToDraw = value;
                DateTime now = DateTime.Now;
                if (frameCount == 0)
                {
                    frameCountingStart = now;
                }
                TimeSpan delta = now - frameCountingStart;
                // Ensure the frame rate is <= 24 which is enough for UX but
                // friendly to GUI responsiveness. Without this, GUI controls
                // may not be refreshed in time.
                if (delta.TotalMilliseconds == 0 || (frameCount + 1) * 1000 / delta.TotalMilliseconds <= 24)
                {
                    frameCount++;
                    Invalidate(false);
                }
                // reset per frameCountResetInterval seconds
                if (delta.TotalMilliseconds > frameCountResetInterval * 1000)
                {
                    frameCount = 0;
                }
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="pe"></param>
        protected override void OnPaint(PaintEventArgs pe)
        {
            if (imageToDraw.iplImage != IntPtr.Zero)
            {
                using (Bitmap bmp = DaVinciCommon.ConvertToBitmap(imageToDraw.iplImage))
                {
                    IntPtr hbmp = bmp.GetHbitmap();
                    IntPtr pTarget = pe.Graphics.GetHdc();
                    IntPtr pSource = CreateCompatibleDC(pTarget);
                    IntPtr pOrig = SelectObject(pSource, hbmp);

                    SetStretchBltMode(pTarget, PaintStretchMode.STRETCH_HALFTONE);
                    StretchBlt(pTarget, 0, 0, this.ClientSize.Width, this.ClientSize.Height, pSource,
                        0, 0, bmp.Width, bmp.Height, PaintRasterOperations.SRCCOPY);

                    IntPtr pNew = SelectObject(pSource, pOrig);
                    DeleteObject(pNew);
                    DeleteDC(pSource);
                    pe.Graphics.ReleaseHdc(pTarget);
                    DeleteObject(hbmp);
                    //pe.Graphics.DrawImage(bmp, 0, 0, this.Width, this.Height);
                }
            }
        }

        [DllImport("gdi32.dll", ExactSpelling = true, SetLastError = true)]
        static extern IntPtr CreateCompatibleDC(IntPtr hdc);

        [DllImport("gdi32.dll")]
        static extern bool SetStretchBltMode(IntPtr hdc, PaintStretchMode stretchMode);

        [DllImport("gdi32.dll")]
        static extern bool StretchBlt(IntPtr dest, int xOriginDest,
            int yOriginDest, int widthDest, int heightDest, IntPtr src,
            int xOriginSrc, int yOriginSrc, int widthSrc, int heightSrc,
            PaintRasterOperations ops);

        [DllImport("gdi32.dll", ExactSpelling = true, SetLastError = true)]
        static extern bool DeleteDC(IntPtr hdc);

        [DllImport("gdi32.dll", ExactSpelling = true, SetLastError = true)]
        static extern IntPtr SelectObject(IntPtr hdc, IntPtr hgdiobj);

        [DllImport("gdi32.dll", ExactSpelling = true, SetLastError = true)]
        static extern bool DeleteObject(IntPtr hObject);

    }
}
