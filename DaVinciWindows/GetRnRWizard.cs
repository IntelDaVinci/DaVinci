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
using System.Xml;

namespace DaVinci
{
    /// <summary>
    /// 
    /// </summary>
    public partial class GetRnRWizard : Form
    {
        /// <summary>
        /// 
        /// </summary>
        public GetRnRWizard()
        {
            InitializeComponent();
            textBoxOptQSFileName.Enabled = false;
            buttonOptQsBrowse.Enabled = false;
        }
        private static string dvcBinHome = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
        private string orgTemplatePath = Path.Combine(dvcBinHome, @"Resources\bias_connect_qs.xml");
        private string qsFilePath = "";
        private string updateText = "";
        private string mainQs = "";
        private string GetProjPath()
        {
            return MainWindow_V.dirName;
        }
        private void buttonMainQsBrowse_Click(object sender, EventArgs e)
        {
            using (OpenFileDialog dialog = new OpenFileDialog())
            {
                dialog.Filter = "DaVinci qs file|*.qs";
                dialog.FilterIndex = 1;
                dialog.Title = "Open a DaVinci test qs file";
                dialog.RestoreDirectory = true;
                dialog.InitialDirectory = GetProjPath();
                if (dialog.ShowDialog() == DialogResult.OK && dialog.FileName != "")
                {
                    textBoxMainQSFileName.Text = Path.GetFileNameWithoutExtension(dialog.FileName);
                    qsFilePath = Path.GetDirectoryName(dialog.FileName);
                    updateText = "<Script name=\"" + textBoxMainQSFileName.Text.Trim() + ".qs\"" + "/>";
                    mainQs = updateText;
                }
            }
        }

        private void checkBoxOptQsFiles_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBoxOptQsFiles.Checked)
            {
                textBoxOptQSFileName.Enabled = true;
                buttonOptQsBrowse.Enabled = true;
            }
            else
            {
                updateText = mainQs;
                textBoxOptQSFileName.Enabled = false;
                buttonOptQsBrowse.Enabled = false;
            }
        }

        private void buttonOptQsBrowse_Click(object sender, EventArgs e)
        {
            using (OpenFileDialog dialog = new OpenFileDialog())
            {
                dialog.Filter = "DaVinci qs file(s)|*qs";
                dialog.FilterIndex = 1;
                dialog.Title = "Open DaVinci test qs file(s)";
                dialog.Multiselect = true;
                dialog.RestoreDirectory = true;
                dialog.InitialDirectory = GetProjPath();
                if (dialog.ShowDialog() == DialogResult.OK && dialog.FileNames != null)
                {
                    string showFileNames = "";
                    foreach (var fileName in dialog.FileNames)
                    {
                        string tmpNameString = Path.GetFileNameWithoutExtension(fileName) + ".qs";
                        tmpNameString += "\r\n";
                        showFileNames += tmpNameString;
                        updateText += "<Script name=\"" + tmpNameString.Trim() + "\"/>";
                    }
                    textBoxOptQSFileName.Text = showFileNames;
                }
            }
        }

        private void UpdateXMLFile(string xmlFilePath, string nodeName, string text)
        {
            XmlDocument xmlDoc = new XmlDocument();
            try
            {
                xmlDoc.Load(xmlFilePath);
                XmlNodeList nodesList = xmlDoc.GetElementsByTagName(nodeName);
                nodesList[0].InnerXml = text;
                xmlDoc.Save(xmlFilePath);
            }
            catch (Exception e)
            {
                MessageBox.Show("Cannot save XML file!\n" + e.Message,
                    "Error!",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
        }

        private void buttonSaveAs_Click(object sender, EventArgs e)
        {
            if (MainWindow_V.dirName == null)
            {
                MessageBox.Show("Need to new or open one project first!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return;
            }
            string mainQsFileName = textBoxMainQSFileName.Text.Trim();
            string xmlFileName = textBoxQSSaveName.Text.Trim();
            if (mainQsFileName != null && mainQsFileName != "")
            {
                if (xmlFileName != null && xmlFileName != "")
                {
                    xmlFileName += ".xml";
                    string xmlSavePath = Path.Combine(qsFilePath, xmlFileName);
                    try
                    {
                        File.Copy(orgTemplatePath, xmlSavePath);
                        if (File.Exists(xmlSavePath))
                        {
                            UpdateXMLFile(xmlSavePath, "Scripts", updateText);
                            MessageBox.Show("XML file has been saved!",
                                "Information",
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Information);
                            this.Close();
                        }
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show("Cannot save XML file!\n" + ex.Message,
                            "Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                    }
                }
                else
                {
                    MessageBox.Show("XML file cannot be empty!",
                        "Warning",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Exclamation);
                }
            }
            else
            {
                MessageBox.Show("Please select QS file first!",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
            }
        }
    }
}
