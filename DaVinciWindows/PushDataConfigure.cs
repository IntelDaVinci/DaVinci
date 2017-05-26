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
using System.IO;

namespace DaVinci
{
    /// <summary>
    /// 
    /// </summary>
    public partial class PushDataConfigure : Form
    {
        /// <summary>
        /// 
        /// </summary>
        public PushDataConfigure()
        {
            InitializeComponent();
        }

        /// <summary>
        /// 
        /// </summary>
        public string textSourceContext = "";
        /// <summary>
        /// 
        /// </summary>
        public string textTargetContext = "";

        private void buttonOK_Click(object sender, EventArgs e)
        {
            char[] invalidCharFileName = Path.GetInvalidPathChars();
            if (textBoxSource.Text.Trim() == "" || textBoxTarget.Text.Trim() == "")
            {
                MessageBox.Show("Source or Target path cannot be empty",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
                return;
            }
            else
            {
                foreach (char ch in invalidCharFileName)
                {
                    if (textBoxSource.Text.Trim().Contains(ch) || textBoxTarget.Text.Trim().Contains(ch))
                    {
                        MessageBox.Show("Source or Target path should not contain invalid characters!",
                            "Warning",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Exclamation);
                        return;
                    }
                }
            }
            textSourceContext = textBoxSource.Text;
            textTargetContext = textBoxTarget.Text;
            this.Close();
        }

        private void buttonBrowser_Click(object sender, EventArgs e)
        {
            using (OpenFileDialog dialog = new OpenFileDialog())
            {
                if (dialog.ShowDialog(this) == DialogResult.OK)
                {
                    textBoxSource.Text = dialog.FileName;
                }
            }
        }

        private void buttonClose_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void buttonReset_Click(object sender, EventArgs e)
        {
            textBoxSource.Text = "";
            textBoxTarget.Text = "";
        }
    }
}
