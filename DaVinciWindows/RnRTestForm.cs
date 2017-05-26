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
    public partial class RnRTestForm : Form
    {
        /// <summary>
        /// 
        /// </summary>
        public RnRTestForm()
        {
            InitializeComponent();
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="defaultName"></param>
        /// <param name="ext"></param>
        public RnRTestForm(string defaultName, string ext = ".qs")
        {
            InitializeComponent();
            if (defaultName != null)
            {
                this.textBoxQSFileName.Text = defaultName;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        public string ScriptName;
        private void buttonRecord_Click(object sender, EventArgs e)
        {
            CreateTestSelect.rnrForm.Close();
            if (ScriptName != null)
            {
                MainWindow_V.form.IsRecordingScript = true;
                DaVinciAPI.NativeMethods.RecordQScript(ScriptName);
            }
            else
            {
                MessageBox.Show("Script name cannot be empty!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }

            if (MainWindow_V.currentApkLocation == "")
            {
                MessageBox.Show("Will set Default APK as Current APK!\n Default APK is: " + (
                    MainWindow_V.defaultApkLocation == "" ? "None" : Path.GetFileName(MainWindow_V.defaultApkLocation)),
                    "Information",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                if (File.Exists(MainWindow_V.defaultApkLocation))
                {
                    MainWindow_V.currentApkLocation = MainWindow_V.defaultApkLocation;
                }
                else
                {
                    MainWindow_V.currentApkLocation = "";
                }
            }
        }

        private void buttonSave_Click(object sender, EventArgs e)
        {
            string fillScriptName;
            if (textBoxQSFileName.Text.Trim() != "")
            {
                fillScriptName = textBoxQSFileName.Text.Trim() + ".qs";
                if (!TestUtil.isValidFileName(fillScriptName))
                {
                    return;
                }
                if (MainWindow_V.dirName == null)
                {
                    MessageBox.Show("Need to new or open one project first!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    return;
                }
                else
                {
                    if (MainWindow_V.curNode != null)
                    {
                        string tempPath = TestUtil.getNodeFullDirectoryPath(MainWindow_V.curNode);
                        ScriptName = Path.Combine(tempPath, fillScriptName);
                    }
                    else
                    {
                        ScriptName = Path.Combine(MainWindow_V.dirName, fillScriptName);
                    }
                    if (File.Exists(ScriptName))
                    {
                        MessageBox.Show("Already has the qs file with the same name! Please use another name!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                        return;
                    }
                    else
                    {
                        MessageBox.Show("Save successfully!", "Info", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    }
                }
            }
            else
            {
                MessageBox.Show("Script name cannot be empty!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
        }

        private void checkBox1_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox1.CheckState == CheckState.Checked)
            {
                DaVinciAPI.NativeMethods.SetRecordWithID(1);
            }
            else
            {
                DaVinciAPI.NativeMethods.SetRecordWithID(0);
            }
        }
    }
}
