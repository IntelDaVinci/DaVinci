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
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;


namespace DaVinci
{
    /// <summary>
    /// 
    /// </summary>
    partial class ImageViewer : Form
    {
        /// <summary>
        /// 
        /// </summary>
        public ImageViewer()
        {
            InitializeComponent();

            imageBox1.Width = base.Width;
            imageBox1.Height = base.Height;
        }

        /// <summary>
        /// 
        /// </summary>
        public ImageBox ImageBox
        {
            get
            {
                return imageBox1;
            }
        }


        public new int Height
        {
            get
            {
                return base.Height;
            }
            set
            {
                base.Height = value;
                imageBox1.Height = base.Height - 35;
            }
        }

        public new int Width
        {
            get
            {
                return base.Width;
            }
            set
            {
                base.Width = value;
                imageBox1.Width = base.Width - 18;
            }
        }

        private void ImageViewer_FormClosed(object sender, FormClosedEventArgs e)
        {
            DaVinciAPI.NativeMethods.StopTest();
        }
    }
}
