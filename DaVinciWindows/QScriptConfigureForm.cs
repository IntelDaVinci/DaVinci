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
    public partial class QScriptConfigureForm : Form
    {
        /// <summary>
        /// 
        /// </summary>
        public QScriptConfigureForm()
        {
            InitializeComponent();
            this.setComoBox();
        }

        private void setComoBox()
        {
            this.configComboBox.Items.AddRange(new object[] {
            "CameraPresetting",
            "PackageName",
            "ActivityName",
            "IconName",
            "APKName",
            "PushData",
            "StartOrientation",
            "CameraType",
            "VideoRecording",
            "MultiLayerMode",
            "RecordedVideoType",
            "ReplayVideoType"});
        }

        private System.Collections.Generic.List<QScriptEditor.conftype> configList = QScriptEditor.configList;
        private void configComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (this.configComboBox.SelectedIndex == -1)
            {
                this.confValueBox.Text = "";
                return;
            }
            if (QScriptEditor.configList != null)
            {
                QScriptEditor.conftype conf = this.configList.Find(c => c.name == this.configComboBox.SelectedItem.ToString());
                if (conf != null)
                    this.confValueBox.Text = conf.value;
            }
        }

        private bool isConfigChanged = false;
        /// <summary>
        /// 
        /// </summary>
        public bool needSaveConfig = false;
        private void confValueBox_TextChanged(object sender, EventArgs e)
        {
            if (this.configComboBox.Text == "" || this.configComboBox.SelectedItem == null || this.configList == null)
                return;
            string configName = this.configComboBox.SelectedItem.ToString();
            if (configName != "")
            {
                int configIndex;
                for (configIndex = 0; configIndex < this.configList.Count; configIndex++)
                    if (this.configList[configIndex].name == configName) break;
                if (configIndex < this.configList.Count)
                {
                    if (this.configList[configIndex].value == this.confValueBox.Text) return;
                    this.configList[configIndex].value = this.confValueBox.Text;
                }
                else
                {
                    QScriptEditor.conftype config = new QScriptEditor.conftype(configName, this.confValueBox.Text);
                    this.configList.Add(config);
                }
                isConfigChanged = true;
            }
        }

        private void QScriptConfigureForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (isConfigChanged)
            {
                DialogResult saveConfigDlg = MessageBox.Show("Configure data has been updated. Save it?", "QScriptEditor", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);
                if (saveConfigDlg == DialogResult.Cancel)
                    e.Cancel = true;
                else if (saveConfigDlg == DialogResult.Yes)
                {
                    needSaveConfig = true;
                }
            }
        }
    }
}
