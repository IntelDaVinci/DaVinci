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
using System.Diagnostics;
using System.Threading;

namespace DaVinci
{
    /// <summary>
    /// 
    /// </summary>
    public partial class TestProjectWizard : Form
    {
        private TestProjectConfig config = new TestProjectConfig();
        /// <summary>
        /// The test project configuration if successfully created
        /// </summary>
        public TestProjectConfig Config
        {
            get
            {
                return config;
            }
        }
        /// <summary>
        /// 
        /// </summary>
        public TestProjectWizard()
        {
            InitializeComponent();
        }

#if DEBUG
        /// <summary>
        /// 
        /// </summary>
        public static bool _debug = true;
#else
        /// <summary>
        /// switch for debugging
        /// </summary>
        public static bool _debug = false;
#endif
        /// <summary>
        /// 
        /// </summary>
        /// <param name="cond"></param>
        /// <param name="text"></param>
        public static void UpdateDebugMessage(bool cond, String text)
        {
            if (_debug && cond)
            {
                Console.WriteLine(text);
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="text"></param>
        public static void UpdateDebugMessage(String text)
        {
            UpdateDebugMessage(true, text);
        }

        private void UpdateLocationText(String path)
        {
            if (textBoxProjectLocation != null)
            {
                if (textBoxProjectLocation.InvokeRequired && !textBoxProjectLocation.IsDisposed)
                {
                    textBoxProjectLocation.BeginInvoke((Action<String>)UpdateLocationText, path);
                }
                else
                {
                    textBoxProjectLocation.Text = path;
                }
            }
        }

        private void UpdateProjectName(String name)
        {
            if (textBoxProjectName != null)
            {
                if (textBoxProjectName.InvokeRequired && !textBoxProjectName.IsDisposed)
                {
                    textBoxProjectName.BeginInvoke((Action<String>)UpdateProjectName, name);
                }
                else
                {
                    if (textBoxProjectName.Text == null || textBoxProjectName.Text.Trim() == "")
                    {
                        try
                        {
                            textBoxProjectName.Text = Path.GetFileName(name);
                            UpdateDebugMessage("Project path set to: " + name + "!");
                        }
                        catch
                        {
                            UpdateDebugMessage("Cannot set path: " + name + "!");
                        }
                    }
                }
            }
        }

        private void buttonProjectLocation_Click(object sender, EventArgs e)
        {
            using (FolderBrowserDialog dialog = new FolderBrowserDialog())
            {
                if (dialog.ShowDialog(this) == DialogResult.OK)
                {
                    String path = dialog.SelectedPath;
                    UpdateLocationText(path);
                    UpdateProjectName(path);
                }
            }
        }


        private static String DaVinciDir = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);

        private void buttonCreate_Click(object sender, EventArgs e)
        {
            char[] invalidCharFileName = Path.GetInvalidFileNameChars();
            if (textBoxProjectName.Text.Trim() == "")
            {
                MessageBox.Show("Project name cannot be empty",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
                return;
            }
            else
            {
                foreach (char ch in invalidCharFileName)
                {
                    if (textBoxProjectName.Text.Trim().Contains(ch))
                    {
                        MessageBox.Show("Project name should not contain invalid characters!",
                            "Warning",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Exclamation);
                        return;
                    }
                }
            }
            config.Name = textBoxProjectName.Text.Trim();

            if (textBoxProjectLocation.Text.Trim() == "")
            {
                MessageBox.Show("Project location cannot be empty",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
                return;
            }

            config.BaseFolder = textBoxProjectLocation.Text.Trim();
            if (!Directory.Exists(config.BaseFolder))
            {
                MessageBox.Show("Project location does not exist",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
                return;
            }
            config.BaseFolder = Path.GetFullPath(config.BaseFolder);

            try
            {
                // TODO: check whether the project file already exists.
                config.Save(config.BaseFolder + Path.DirectorySeparatorChar + config.Name + ".qproj");
            }
            catch (Exception)
            {
                MessageBox.Show("Fail to create test project configuration file!",
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return;
            }
            DialogResult = DialogResult.OK;
            this.Close();
        }

        private void buttonCancel_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
            this.Close();
        }
    }
}
