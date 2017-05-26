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
using System.Reflection;

namespace DaVinci
{
    /// <summary>
    /// 
    /// </summary>
    public partial class SplashScreen : Form
    {
        /// <summary>
        /// 
        /// </summary>
        public SplashScreen()
        {
            InitializeComponent();
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="ver"></param>
        public SplashScreen(string ver)
        {
            InitializeComponent();
            ShowVersion(ver);
            backgroundWorker.WorkerReportsProgress = true;
            if (!backgroundWorker.IsBusy)
            {
                backgroundWorker.RunWorkerAsync();
            }
        }

        /// <summary>
        /// 
        /// </summary>
        public void SetInitEnd()
        {
            if (backgroundWorker.IsBusy)
            {
                backgroundWorker.CancelAsync();
            }
            UpdateProgress(ProgressBar.Maximum);
        }

        private void ShowVersion(string version)
        {
            this.labelShowVer.Text = "v" + version;
        }

        private void backgroundWorker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            UpdateProgress(e.ProgressPercentage);
        }

        private void UpdateProgress(int value)
        {
            if (this.ProgressBar != null && !this.ProgressBar.IsDisposed)
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke((Action<int>)UpdateProgress, value);
                }
                else
                {
                    this.ProgressBar.Value = value;
                }
            }
        }

        private void backgroundWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            for (int i = ProgressBar.Minimum; i <= ProgressBar.Maximum / 10 * 9; i++)
            {
                // Set the ceiling value to 90 in order to process following case:
                // If the Init process is very slow while the value of the progress bar
                // has alread update to 90, then automatic report progress should halt.
                System.Threading.Thread.Sleep(300);
                backgroundWorker.ReportProgress(i);
            }
        }
    }
}
