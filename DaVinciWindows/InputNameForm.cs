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
    /// Form to get a Q script name to save
    /// </summary>
    public partial class InputNameForm : Form
    {
        /// <summary>
        /// The constructor
        /// </summary>
        public InputNameForm()
        {
            InitializeComponent();
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="defaultName"></param>
        /// <param name="ext"></param>
        public InputNameForm(string defaultName, string ext = ".qs")
        {
            InitializeComponent();
            if (defaultName != null)
                this.textBoxScriptName.Text = defaultName;
            if (extension != null)
            {
                extension = ext;
                this.label2.Text = ext;
            }
        }

        private void buttonOK_Click(object sender, EventArgs e)
        {
            if (textBoxScriptName.Text.Trim() != "")
            {
                ScriptName = textBoxScriptName.Text.Trim() + extension;
                DialogResult = DialogResult.OK;
                this.Close();
            }
            else
            {
                MessageBox.Show("Name cannot be empty!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return;
            }
        }

        /// <summary>
        /// The script name to save
        /// </summary>
        public string ScriptName;
        /// <summary>
        /// extension
        /// </summary>
        private string extension = "";
        private void buttonCancel_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
            this.Close();
        }
    }
}
