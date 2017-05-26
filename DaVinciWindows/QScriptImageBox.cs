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
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Drawing;
using System.Runtime.InteropServices;
using System.IO;
using System.Windows.Forms;

namespace DaVinci
{
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    internal unsafe struct IplImage
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

    partial class QScriptImageBox : ImageBox
    {
        Bitmap bitmapToDraw;
        bool isEditingPicture = false;

        public Bitmap BitmapToDraw
        {
            set
            {
                if (bitmapToDraw != null) bitmapToDraw.Dispose();
                
                bitmapToDraw = value;
                Invalidate(false);

            }
        }

        public bool IsEditingPicture
        {
            get
            {
                return isEditingPicture;
            }
            set
            {
                isEditingPicture = value;

            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="pe"></param>
        protected override void OnPaint(PaintEventArgs pe)
        {
            if (isEditingPicture && bitmapToDraw != null)
            {
                IntPtr hbmp = bitmapToDraw.GetHbitmap();
                IntPtr pTarget = pe.Graphics.GetHdc();
                IntPtr pSource = CreateCompatibleDC(pTarget);
                IntPtr pOrig = SelectObject(pSource, hbmp);

                SetStretchBltMode(pTarget, PaintStretchMode.STRETCH_HALFTONE);
                StretchBlt(pTarget, 0, 0, this.ClientSize.Width, this.ClientSize.Height, pSource,
                    0, 0, bitmapToDraw.Width, bitmapToDraw.Height, PaintRasterOperations.SRCCOPY);

                IntPtr pNew = SelectObject(pSource, pOrig);
                DeleteObject(pNew);
                DeleteDC(pSource);
                pe.Graphics.ReleaseHdc(pTarget);
            }else{
                base.OnPaint(pe);
            }
            

        }

        public QScriptImageBox()
        {
            InitializeComponent();
        }

        public QScriptImageBox(IContainer container)
        {
            container.Add(this);

            InitializeComponent();
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
