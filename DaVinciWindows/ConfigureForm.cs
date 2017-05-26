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
using System.Text.RegularExpressions;
using System.Xml.Serialization;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace DaVinci
{
    /// <summary>
    /// 
    /// </summary>
    public partial class ConfigureForm : Form
    {
        // Main window
        MainWindow_V mainWindow;
        /// <summary>
        /// Initialize components not included in INDE release.
        /// </summary>
        private void InitializeComponentNonINDE()
        {
#if !INDE
            #region windows
            System.Windows.Forms.TreeNode treeNode6 = new System.Windows.Forms.TreeNode("Windows");
            treeNode6.Name = "NodeWindows";
            treeNode6.Text = "Windows";
            this.treeViewDevices.Nodes.Add(treeNode6);
            #endregion
            #region chrome
            System.Windows.Forms.TreeNode node = new System.Windows.Forms.TreeNode("Chrome OS");
            node.Name = "NodeChrome";
            node.Text = "Chrome OS";
            this.treeViewDevices.Nodes.Add(node);
            #endregion
#else
            this.labelDevicePlayAudio.Hide();
            this.comboBoxDevicePlayAudio.Hide();
            this.labelDeviceRecordAudio.Hide();
            this.comboBoxDeviceRecordAudio.Hide();
            this.labelPlayToUser.Hide();
            this.comboBoxPlayToUser.Hide();
            this.labelFromUser.Hide();
            this.comboBoxRecordFromUser.Hide();
            this.comboBoxDeviceFfrdController.Hide();
            this.labelFfrdController.Hide();
#endif
        }

        /// <summary>
        /// The type of screen source
        /// </summary>
        public enum ScreenSource
        {
#if !INDE
            /// <summary>
            /// Camera Index 0
            /// </summary>
            [XmlEnum("Camera 0")]
            CameraIndex0,
            /// <summary>
            /// Camera Index 1
            /// </summary>
            [XmlEnum("Camera 1")]
            CameraIndex1,
            /// <summary>
            /// Camera Index 2
            /// </summary>
            [XmlEnum("Camera 2")]
            CameraIndex2,
            /// <summary>
            /// Camera Index 3
            /// </summary>
            [XmlEnum("Camera 3")]
            CameraIndex3,
            /// <summary>
            /// Camera Index 4
            /// </summary>
            [XmlEnum("Camera 4")]
            CameraIndex4,
            /// <summary>
            /// Camera Index 5
            /// </summary>
            [XmlEnum("Camera 5")]
            CameraIndex5,
            /// <summary>
            /// Camera Index 6
            /// </summary>
            [XmlEnum("Camera 6")]
            CameraIndex6,
            /// <summary>
            /// Camera Index 7
            /// </summary>
            [XmlEnum("Camera 7")]
            CameraIndex7,
#endif
            /// <summary>
            /// Use screen cap
            /// </summary>
            [XmlEnum("useScreenCap")]
            ScreenCap,
            /// <summary>
            /// undefined
            /// </summary>
            [XmlEnum("undefined")]
            Undefined,
            /// <summary>
            /// use blank screen
            /// </summary>
            [XmlEnum("disabled")]
            Disabled
        };

        private void initTargetDeviceCombos()
        {
            // Multi-Layer support
            comboBoxMultiLayerSupport.Items.Clear();
            comboBoxMultiLayerSupport.Items.AddRange(new string[] { "Enable", "Disable" });
            // screen capture
            comboBoxDeviceCapture.Items.Clear();

            string[] cameraAvailable = new string[maxCameraNum];
            for (int i = 0; i < maxCameraNum; i++)
            {
                cameraAvailable[i] = "Unavailable";
            }
            lock (LockOfCameras)
            {
                int camIndex = 0;
                foreach (string camera in TestUtil.captureDevices)
                {
                    if (camIndex < maxCameraNum)
                    {
                        cameraAvailable[camIndex] = camera;
                        camIndex++;
                    }
                }
            }
            for (int i = 0; i < maxCameraNum; i++)
            {
                if (cameraAvailable[i] != null)
                    comboBoxDeviceCapture.Items.Add(cameraAvailable[i]);
            }

            // FFRD controller
            comboBoxDeviceFfrdController.Items.Clear();
            comboBoxDeviceFfrdController.Items.Add("None");
            lock (LockOfControllers)
            {
                if (TestUtil.fFRDDevices != null)
                {
                    foreach (string fFRDDevice in TestUtil.fFRDDevices)
                        comboBoxDeviceFfrdController.Items.Add(fFRDDevice);
                }
            }
            comboBoxDeviceFfrdController.Items.Add("MAP");

            SetDeviceAudioComboBoxes(null);
        }

        private void SetDeviceAudioComboBoxes(string deviceName)
        {
            comboBoxDevicePlayAudio.Items.Clear();
            comboBoxDevicePlayAudio.Items.AddRange(TestUtil.audioPlayDevices);
            comboBoxDeviceRecordAudio.Items.Clear();
            comboBoxDeviceRecordAudio.Items.AddRange(TestUtil.audioRecordDevices);
            if (deviceName == null)
            {
                return;
            }
            const int maxLen = 1024;
            StringBuilder audioDevice = new StringBuilder().Append('c', maxLen);
            DaVinciAPI.NativeMethods.GetAudioDevice(deviceName, DaVinciAPI.AudioDeviceType.AudioDeviceTypePlayToDevice, audioDevice, maxLen);
            for (int i = 0; i < TestUtil.audioPlayDevices.Length; i++)
            {
                if (TestUtil.audioPlayDevices[i] == audioDevice.ToString())
                {
                    comboBoxDevicePlayAudio.SelectedIndex = i;
                    break;
                }
            }
            audioDevice = new StringBuilder(maxLen).Append('c', maxLen);
            DaVinciAPI.NativeMethods.GetAudioDevice(deviceName, DaVinciAPI.AudioDeviceType.AudioDeviceTypeRecordFromDevice, audioDevice, maxLen);
            for (int i = 0; i < TestUtil.audioRecordDevices.Length; i++)
            {
                if (TestUtil.audioRecordDevices[i] == audioDevice.ToString())
                {
                    comboBoxDeviceRecordAudio.SelectedIndex = i;
                    break;
                }
            }
        }

        private void refreshDevicesNodes()
        {
            // init combo values
            initTargetDeviceCombos();

            // store the selected node
            string oldSelectedDeviceName = null;
            TreeNode oldNode = treeViewDevices.SelectedNode;
            if (treeViewDevices.SelectedNode != null && treeViewDevices.SelectedNode.Parent != null)
                oldSelectedDeviceName = treeViewDevices.SelectedNode.Text;

            TreeNode tempNode = null;
            // Clear all devices.
            foreach (TreeNode tn in treeViewDevices.Nodes)
                tn.Nodes.Clear();
            // Add Android devices
            TreeNode androidTreeNode = getAndroidRootNode();

            if (TestUtil.targetDevices == null)
                return;
            foreach (string atdName in TestUtil.targetDevices)
            {
                tempNode = new TreeNode(atdName);
                androidTreeNode.Nodes.Add(tempNode);
                if (oldSelectedDeviceName != null && atdName == oldSelectedDeviceName)
                {
                    treeViewDevices.SelectedNode = tempNode;
                }
                if ((GetCurrentTargetDevice() == atdName)
                    && Marshal.PtrToStringAnsi(DaVinciAPI.NativeMethods.GetSpecifiedDeviceStatus(atdName)) != "Offline")
                {
                    tempNode.ForeColor = DaVinciCommon.IntelRed;
                    if (oldNode == null)
                        treeViewDevices.SelectedNode = tempNode;
                }
                else
                    tempNode.ForeColor = DaVinciCommon.IntelBlack;
            }

            treeViewDevices.ExpandAll();
        }

        /// <summary>
        /// Construct Devices GUI components.
        /// </summary>
        public ConfigureForm(MainWindow_V mainWindow)
        {
            this.mainWindow = mainWindow;
            InitializeComponent();
            InitializeComponentNonINDE();
        }

        private void refreshHostDeviceProperties()
        {
            comboBoxRecordFromUser.Items.Clear();
            comboBoxRecordFromUser.Items.AddRange(TestUtil.audioRecordDevices);
            StringBuilder audioDevice = new StringBuilder().Append('c', TestUtil.maxLen);
            DaVinciAPI.NativeMethods.GetAudioDevice(null, DaVinciAPI.AudioDeviceType.AudioDeviceTypeRecordFromUser, audioDevice, TestUtil.maxLen);
            for (int i = 0; i < TestUtil.audioRecordDevices.Length; i++)
            {
                if (TestUtil.audioRecordDevices[i] == audioDevice.ToString())
                {
                    comboBoxRecordFromUser.SelectedIndex = i;
                    break;
                }
            }
            comboBoxPlayToUser.Items.Clear();
            comboBoxPlayToUser.Items.AddRange(TestUtil.audioPlayDevices);
            audioDevice = new StringBuilder().Append('c', TestUtil.maxLen);
            DaVinciAPI.NativeMethods.GetAudioDevice(null, DaVinciAPI.AudioDeviceType.AudioDeviceTypePlayDeviceAudio, audioDevice, TestUtil.maxLen);
            for (int i = 0; i < TestUtil.audioPlayDevices.Length; i++)
            {
                if (TestUtil.audioPlayDevices[i] == audioDevice.ToString())
                {
                    comboBoxPlayToUser.SelectedIndex = i;
                    break;
                }
            }
            if (DaVinciAPI.NativeMethods.GetGpuSupport() == DaVinciAPI.BoolType.BoolFalse)
            {
                checkBoxOthersGPU.Checked = false;
            }
            else
            {
                checkBoxOthersGPU.Checked = true;
            }
        }

        private void buttonDevicesRefresh_Click(object sender, EventArgs e)
        {
            this.Enabled = false;
            TestUtil.RefreshTestDevices();
            String lastCaptureDeviceStr = null;
            if (comboBoxDeviceCapture.SelectedItem != null)
                lastCaptureDeviceStr = comboBoxDeviceCapture.SelectedItem.ToString();
            TreeNode tn = treeViewDevices.SelectedNode;
            refreshDevicesNodes();
            SelectCaptureDevice(lastCaptureDeviceStr);
            this.Enabled = true;
        }

        private TreeNode getCurrentTargetDeviceNode()
        {
            string td = GetCurrentTargetDevice();
            if (td == null)
                return null;
            foreach (TreeNode tn in getAndroidRootNode().Nodes)
            {
                if (tn.Text == td)
                    return tn;
            }
            return null;
        }

        private void disableAllPropertyHandlers()
        {
            comboBoxMultiLayerSupport.SelectedIndexChanged -= comboBoxMultiLayerSupport_SelectedIndexChanged;
            comboBoxDevicePlayAudio.SelectedIndexChanged -= comboBoxDevicePlayAudio_SelectedIndexChanged;
            comboBoxDeviceRecordAudio.SelectedIndexChanged -= comboBoxDeviceRecordAudio_SelectedIndexChanged;
            comboBoxDeviceCapture.SelectedIndexChanged -= comboBoxDeviceCapture_SelectedIndexChanged;
        }

        private void enableAllPropertyHandlers()
        {
            comboBoxMultiLayerSupport.SelectedIndexChanged += comboBoxMultiLayerSupport_SelectedIndexChanged;
            comboBoxDevicePlayAudio.SelectedIndexChanged += comboBoxDevicePlayAudio_SelectedIndexChanged;
            comboBoxDeviceRecordAudio.SelectedIndexChanged += comboBoxDeviceRecordAudio_SelectedIndexChanged;
            comboBoxDeviceCapture.SelectedIndexChanged += comboBoxDeviceCapture_SelectedIndexChanged;
        }

        private void unuseComboBox(ComboBox box)
        {
            box.SelectedIndex = -1;
            box.Enabled = false;
        }

        private void useComboBox(ComboBox box, int index = -1)
        {
            if (this.InvokeRequired && !this.IsDisposed)
            {
                this.BeginInvoke((Action<ComboBox, int>)useComboBox, box, index);
            }
            else
            {
                box.Enabled = true;
                if (index < 0 || index >= box.Items.Count)
                    index = -1;
                box.SelectedIndex = index;
            }
        }

        private void useComboBox(ComboBox box, string value)
        {
            if (value == null || value.Trim() == "")
            {
                useComboBox(box, -1);
                return;
            }
            for (int i = 0; i < box.Items.Count; i++)
            {
                if (box.Items[i].ToString().Equals(value))
                {
                    useComboBox(box, i);
                    return;
                }
            }
            // Didn't find the value, select the first item
            useComboBox(box, 0);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        public static ScreenSource IndexToScreenSource(int index)
        {
            if (index == -2)
                return ScreenSource.Disabled;
            else if (index == -1)
                return ScreenSource.ScreenCap;
#if !INDE
            else if (index == 0)
                return ScreenSource.CameraIndex0;
            else if (index == 1)
                return ScreenSource.CameraIndex1;
            else if (index == 2)
                return ScreenSource.CameraIndex2;
            else if (index == 3)
                return ScreenSource.CameraIndex3;
            else if (index == 4)
                return ScreenSource.CameraIndex4;
            else if (index == 5)
                return ScreenSource.CameraIndex5;
            else if (index == 6)
                return ScreenSource.CameraIndex6;
            else if (index == 7)
                return ScreenSource.CameraIndex7;
#endif
            else
            {
                return ScreenSource.Undefined;
            }
        }

        private void refreshDeviceProperties(string deviceName)
        {
            disableAllPropertyHandlers();
            //AndroidTargetDevice atd = device as AndroidTargetDevice;
            if (deviceName == null)
            {
                textBoxDeviceIp.Text = "";
                textBoxDeviceHeight.Text = "";
                textBoxDeviceHeight.Enabled = false;
                textBoxDeviceWidth.Text = "";
                textBoxDeviceWidth.Enabled = false;
                unuseComboBox(comboBoxMultiLayerSupport);
                textBoxConnectedBy.Text = "";
                unuseComboBox(comboBoxDevicePlayAudio);
                unuseComboBox(comboBoxDeviceRecordAudio);
                unuseComboBox(comboBoxDeviceCapture);
                SetDeviceAudioComboBoxes(null);

                textBoxAgentStatus.Text = "";
            }
            else if (deviceName != null)
            {
                textBoxDeviceIp.Text = "";
                textBoxDeviceHeight.Text = DaVinciAPI.NativeMethods.GetTargetDeviceHeight(deviceName).ToString();
                textBoxDeviceHeight.Enabled = true;
                textBoxDeviceWidth.Text = DaVinciAPI.NativeMethods.GetTargetDeviceWidth(deviceName).ToString();
                textBoxDeviceWidth.Enabled = true;
                useComboBox(comboBoxMultiLayerSupport, getMultiLayerSupportIndex(deviceName));
                if (DaVinciAPI.NativeMethods.IsDeviceConnected(deviceName) == DaVinciAPI.BoolType.BoolTrue)
                {
                    textBoxConnectedBy.Text = "127.0.0.1";
                }
                else
                    textBoxConnectedBy.Text = "";
                useComboBox(comboBoxDeviceCapture, getScreenSource(deviceName));
                useComboBox(comboBoxDeviceFfrdController, getHwController(deviceName));
                SetDeviceAudioComboBoxes(deviceName);
                IntPtr text = DaVinciAPI.NativeMethods.GetSpecifiedDeviceStatus(deviceName);
                textBoxAgentStatus.Text = Marshal.PtrToStringAnsi(text);
            }
            enableAllPropertyHandlers();
        }

        private void buttonMakeCurrent_Click(object sender, EventArgs e)
        {
            string td = getSelectedTargetDeviceName();

            if (td == null)
                return;
            this.Enabled = false;
            if ((StatusConvertion(Marshal.PtrToStringAnsi(DaVinciAPI.NativeMethods.GetSpecifiedDeviceStatus(td))) 
                    == DaVinciAPI.TargetDeviceConnStatus.TargetDeviceConnStatusOffline) || 
                (StatusConvertion(Marshal.PtrToStringAnsi(DaVinciAPI.NativeMethods.GetSpecifiedDeviceStatus(td))) 
                    == DaVinciAPI.TargetDeviceConnStatus.TargetDeviceConnStatusConnectedByOthers))
            {
                MessageBox.Show("Cannot setting Offline/ConnectedByOthers device as current device!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                buttonMakeCurrent.Enabled = true;
            }
            else
            {
                TreeNode currentDeviceNode = getCurrentTargetDeviceNode();
                if (currentDeviceNode != null)
                    currentDeviceNode.ForeColor = DaVinciCommon.IntelBlack;
                DaVinciAPI.NativeMethods.SetCurrentTargetDevice(td);

                treeViewDevices.SelectedNode.ForeColor = DaVinciCommon.IntelRed;
                buttonMakeCurrent.Enabled = false;
                // Update current device as the first one in DaVinci.config file
                DaVinciAPI.NativeMethods.SetDeviceOrder(td);
            }
            refreshDeviceProperties(td);
            mainWindow.UpdateStatusDeviceName(td);
            this.Enabled = true;
        }

        /// <summary>
        /// Find the android device by name
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        public string findAndroidTargetDevice(string name)
        {
            if (name == null)
                return null;
            lock (LockOfAndroidDevices)
            {
                foreach (string tdName in TestUtil.targetDevices)
                {
                    if (tdName == name)
                        return tdName;
                }
            }
            return null;
        }

        private string getSelectedTargetDeviceName()
        {
            if (treeViewDevices.SelectedNode == null
                || treeViewDevices.SelectedNode.Parent == null)
                return null;
            for (int i = 0; i < TestUtil.targetDevices.Length; i++)
            {
                if (TestUtil.targetDevices[i] == treeViewDevices.SelectedNode.Text)
                {
                    return TestUtil.targetDevices[i];
                }
            }
            return null;
        }

        /// <summary>
        /// Remove a device
        /// </summary>
        /// <param name="td"></param>
        /// <returns></returns>
        public void removeDevice(string td)
        {
            for (int i = 0; i < TestUtil.targetDevices.Length; i++)
            {
                if (TestUtil.targetDevices[i] == td)
                {
                    TestUtil.targetDevices[i] = "";
                }
                else
                {
                    continue;
                }
            }
        }

        private DaVinciAPI.TargetDeviceConnStatus StatusConvertion(string status)
        {
            if (status == "Connected")
            {
                return DaVinciAPI.TargetDeviceConnStatus.TargetDeviceConnStatusConnected;
            }
            else if (status == "ConnectedByOthers")
            {
                return DaVinciAPI.TargetDeviceConnStatus.TargetDeviceConnStatusConnectedByOthers;
            }
            else if (status == "ReadyForConnection")
            {
                return DaVinciAPI.TargetDeviceConnStatus.TargetDeviceConnStatusReadyToConnect;
            }
            else
            {
                return DaVinciAPI.TargetDeviceConnStatus.TargetDeviceConnStatusOffline;
            }
        }

        private void buttonDevicesDelete_Click(object sender, EventArgs e)
        {
            if (treeViewDevices.SelectedNode == null || treeViewDevices.SelectedNode.Parent == null)
                return;
            string device = getSelectedTargetDeviceName();
            if (device == null)
            {
                treeViewDevices.SelectedNode.Parent.Nodes.Remove(treeViewDevices.SelectedNode);
            }
            else if (device == GetCurrentTargetDevice())
            {
                MessageBox.Show("Cannot remove current device.", "Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            else
            {
                string status = Marshal.PtrToStringAnsi(DaVinciAPI.NativeMethods.GetSpecifiedDeviceStatus(device));
                if (StatusConvertion(status) != DaVinciAPI.TargetDeviceConnStatus.TargetDeviceConnStatusOffline)
                {
                    MessageBox.Show("Cannot remove an Android device which is not offline.", "Information",
                        MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                else
                {
                    treeViewDevices.SelectedNode.Parent.Nodes.Remove(treeViewDevices.SelectedNode);
                    removeDevice(device);
                    DaVinciAPI.NativeMethods.DeleteTargetDevice(device);
                    if (DaVinciAPI.NativeMethods.SaveConfigureData() != 0)
                    {
                        MessageBox.Show("Cannot save configure data correctly!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    }
                }
            }
        }

        private bool isAndroidRootNode(TreeNode tn)
        {
            if (tn == null || tn.Parent != null || tn.Text != "Android")
                return false;
            else
                return true;
        }

        private bool isDuplicatedTreeViewNodeName(string name, TreeNodeCollection tnc = null)
        {
            if (tnc == null)
                tnc = treeViewDevices.Nodes;
            foreach (TreeNode tn in tnc)
                if (tn.Text.Equals(name))
                    return true;
                else if (isDuplicatedTreeViewNodeName(name, tn.Nodes))
                    return true;
            return false;
        }

        private TreeNode getAndroidRootNode()
        {
            foreach (TreeNode tn in treeViewDevices.Nodes)
                if (isAndroidRootNode(tn))
                    return tn;
            throw new Exception("Cannot find the Android root node");
        }

        /// <summary>
        /// lock of android device list
        /// </summary>
        public object LockOfAndroidDevices = new object();

        /// <summary>
        /// lock of cameras
        /// </summary>
        public object LockOfCameras = new object();

        /// <summary>
        /// lock of controllers
        /// </summary>
        public object LockOfControllers = new object();

        /// <summary>
        /// 
        /// </summary>
        public readonly int maxCameraNum = 8;

        private void buttonDevicesAdd_Click(object sender, EventArgs e)
        {
            if (treeViewDevices.SelectedNode == null)
                return;

            using (InputNameForm saveForm = new InputNameForm("", ""))
            {
                if (saveForm.ShowDialog() == DialogResult.OK)
                {
                    string deviceName = saveForm.ScriptName;

                    if (isAndroidRootNode(treeViewDevices.SelectedNode) || isAndroidRootNode(treeViewDevices.SelectedNode.Parent))
                    {
                        if (isDuplicatedTreeViewNodeName(deviceName))
                        {
                            MessageBox.Show(deviceName + " is duplicated or not a valid Android device name.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                        }
                        else
                        {
                            TreeNode tn = new TreeNode(deviceName);
                            getAndroidRootNode().Nodes.Add(tn);
                            treeViewDevices.SelectedNode = tn;
                            DaVinciAPI.NativeMethods.AddNewTargetDevice(deviceName);
                            if (DaVinciAPI.NativeMethods.SaveConfigureData() != 0)
                            {
                                MessageBox.Show("Cannot save configure data correctly!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                            }
                        }
                    }
                    else
                        return;
                }
            }
        }

        private void ConfigureForm_Load(object sender, EventArgs e)
        {
            // Refresh devices nodes at ConfigureForm UI level
            refreshDevicesNodes();
            // Refresh device property list
            refreshHostDeviceProperties();
        }

        private void comboBoxMultiLayerSupport_SelectedIndexChanged(object sender, EventArgs e)
        {
            string selectedDevice = getSelectedTargetDeviceName();
            if (selectedDevice == null)
                return;

            if (comboBoxMultiLayerSupport.SelectedIndex < 1)
            {
                DaVinciAPI.NativeMethods.SetDeviceCurrentMultiLayerMode(selectedDevice, true);
            }
            else
            {
                DaVinciAPI.NativeMethods.SetDeviceCurrentMultiLayerMode(selectedDevice, false);
            }

            string status = Marshal.PtrToStringAnsi(DaVinciAPI.NativeMethods.GetSpecifiedDeviceStatus(selectedDevice));
            if (StatusConvertion(status) != DaVinciAPI.TargetDeviceConnStatus.TargetDeviceConnStatusOffline)
            {
                DaVinciAPI.NativeMethods.Connect(selectedDevice);
            }
            if (DaVinciAPI.NativeMethods.SaveConfigureData() != 0)
            {
                MessageBox.Show("Cannot save configure data correctly!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
        }

        private void comboBoxDevicePlayAudio_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (comboBoxDevicePlayAudio.SelectedIndex < 0) return;
            string selectedDeviceName = getSelectedTargetDeviceName();
            if (selectedDeviceName == null) return;
            DaVinciAPI.NativeMethods.SetAudioDevice(
                selectedDeviceName,
                DaVinciAPI.AudioDeviceType.AudioDeviceTypePlayToDevice,
                comboBoxDevicePlayAudio.SelectedItem.ToString());
            if (DaVinciAPI.NativeMethods.SaveConfigureData() != 0)
            {
                MessageBox.Show("Cannot save configure data correctly!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
        }

        private void comboBoxDeviceRecordAudio_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (comboBoxDeviceRecordAudio.SelectedIndex < 0) return;
            string selectedDeviceName = getSelectedTargetDeviceName();
            if (selectedDeviceName == null) return;
            DaVinciAPI.NativeMethods.SetAudioDevice(
                selectedDeviceName,
                DaVinciAPI.AudioDeviceType.AudioDeviceTypeRecordFromDevice,
                comboBoxDeviceRecordAudio.SelectedItem.ToString());
            if (DaVinciAPI.NativeMethods.SaveConfigureData() != 0)
            {
                MessageBox.Show("Cannot save configure data correctly!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
        }

        private int captureComboBoxItemToIndex(string item)
        {
            if (item == "Screen Capture" || item == "HyperSoftCam")
                return -1;
            else if (item == "Disabled")
                return -2;
            else
            {
                int index = -2;
                string camIndexString = "";
                if (item.Contains('-'))
                {
                    string[] strings = item.Split('-');
                    camIndexString = strings[0];
                }
                else
                {
                    camIndexString = item;
                }
                string tempStr = Regex.Replace(camIndexString, @"[^\d.\d]", "");
                if (Regex.IsMatch(tempStr, @"^[+-]?\d*[.]?\d*$"))
                {
                    Int32.TryParse(tempStr, out index);
                }
                return index;
            }
        }

        private string ScreenSourceToCaptureName(ScreenSource screenSource)
        {
            switch (screenSource)
            {
                case ScreenSource.Disabled:
                    return "Disabled";
                case ScreenSource.ScreenCap:
                    return "HyperSoftCam";
                case ScreenSource.CameraIndex0:
                    return TestUtil.captureDevices[2];
                case ScreenSource.CameraIndex1:
                    return TestUtil.captureDevices[3];
                case ScreenSource.CameraIndex2:
                    return TestUtil.captureDevices[4];
                case ScreenSource.CameraIndex3:
                    return TestUtil.captureDevices[5];
                case ScreenSource.CameraIndex4:
                    return TestUtil.captureDevices[6];
                case ScreenSource.CameraIndex5:
                    return TestUtil.captureDevices[7];
                case ScreenSource.CameraIndex6:
                    return TestUtil.captureDevices[8];
                case ScreenSource.CameraIndex7:
                    return TestUtil.captureDevices[9];
                default:
                    return "Undefined";
            }
        }

        int getScreenSource(String deviceName)
        {
            const int maxSize = 1024;
            const int indexOffset = 2;
            StringBuilder captureName = new StringBuilder(maxSize);
            DaVinciAPI.NativeMethods.GetCaptureDeviceForTargetDevice(deviceName, captureName, maxSize);
            int index = captureComboBoxItemToIndex(captureName.ToString());
            return index + indexOffset;
        }

        string getHwController(String deviceName)
        {
            const int maxSize = 1024;
            StringBuilder hwControllerName = new StringBuilder(maxSize);
            DaVinciAPI.NativeMethods.GetHWAccessoryControllerForTargetDevice(deviceName, hwControllerName, maxSize);
            string hwControllerNameStr = hwControllerName.ToString();
            if (hwControllerNameStr == "")
            {
                return "None";
            }
            else
            {
                return hwControllerNameStr;
            }
        }

        int getMultiLayerSupportIndex(string devName)
        {
            if (DaVinciAPI.NativeMethods.GetDeviceCurrentMultiLayerMode(devName) == DaVinciAPI.BoolType.BoolTrue)
            {
                return 0;
            }
            else
            {
                return 1;
            }
        }

        void SelectCaptureDevice(String lastCapDevStr = null)
        {
            ScreenSource selectedScreenSource = ScreenSource.Undefined;
            if (comboBoxDeviceCapture.SelectedIndex < 0)
                return;
            string selectedDevice = getSelectedTargetDeviceName();
            if (selectedDevice == null)
                return;
            string selectedSourceString = comboBoxDeviceCapture.SelectedItem.ToString();

            ScreenSource currentScreenSource;
            if (comboBoxDeviceCapture.SelectedIndex == 0)
            {
                currentScreenSource = ScreenSource.Disabled;
            }
            else if (comboBoxDeviceCapture.SelectedIndex == 1)
            {
                currentScreenSource = ScreenSource.ScreenCap;
            }
            else
            {
                int index = captureComboBoxItemToIndex(comboBoxDeviceCapture.SelectedItem.ToString());
                currentScreenSource = IndexToScreenSource(index);
            }

            selectedScreenSource = currentScreenSource;

            if (selectedDevice == GetCurrentTargetDevice())
            {
                this.Enabled = false;
                DaVinciAPI.NativeMethods.SetCurrentCaptureDevice(ScreenSourceToCaptureName(selectedScreenSource));
                refreshDeviceProperties();
                this.Enabled = true;
            }
            else
            {
                this.Enabled = false;
                DaVinciAPI.NativeMethods.SetCaptureDeviceForTargetDevice(selectedDevice, ScreenSourceToCaptureName(selectedScreenSource));
                this.Enabled = true;
            }
        }

        private void comboBoxDeviceCapture_SelectedIndexChanged(object sender, EventArgs e)
        {
            SelectCaptureDevice();
            if (DaVinciAPI.NativeMethods.SaveConfigureData() != 0)
            {
                MessageBox.Show("Cannot save configure data correctly!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
        }

        private string DeviceConnectionStatusToString(DaVinciAPI.TargetDeviceConnStatus status)
        {
            switch (status)
            {
                case DaVinciAPI.TargetDeviceConnStatus.TargetDeviceConnStatusConnected:
                    return "Connected";
                case DaVinciAPI.TargetDeviceConnStatus.TargetDeviceConnStatusConnectedByOthers:
                    return "ConnectedByOthers";
                case DaVinciAPI.TargetDeviceConnStatus.TargetDeviceConnStatusReadyToConnect:
                    return "ReadyForConnection";
                default:
                    return "Offline";
            }
        }

        private void textBoxAgentStatus_TextChanged(object sender, EventArgs e)
        {
            string selectedDevice = getSelectedTargetDeviceName();

            if ((selectedDevice != null) && (selectedDevice == GetCurrentTargetDevice()))
            {
                if (textBoxAgentStatus.Text.Trim().Equals(DeviceConnectionStatusToString(DaVinciAPI.TargetDeviceConnStatus.TargetDeviceConnStatusConnected)))
                {
                    buttonDevicesConnectAgent.Enabled = true;
                    buttonDevicesConnectAgent.Text = "Disconnect";
                }
                else if (textBoxAgentStatus.Text.Trim().Equals(DeviceConnectionStatusToString(DaVinciAPI.TargetDeviceConnStatus.TargetDeviceConnStatusReadyToConnect)))
                {
                    buttonDevicesConnectAgent.Enabled = true;
                    buttonDevicesConnectAgent.Text = "Connect";
                }
                else
                {
                    buttonDevicesConnectAgent.Enabled = false;
                    buttonDevicesConnectAgent.Text = "Connect";
                }
            }
            else
            {
                buttonDevicesConnectAgent.Enabled = false;
                buttonDevicesConnectAgent.Text = "Connect";
            }
        }

        private void refreshDeviceProperties()
        {
            TreeNode tn = treeViewDevices.SelectedNode;

            if (tn == null || tn.Parent == null)
                refreshDeviceProperties(null);
            else if (tn.Parent.Text == "Android")
                refreshDeviceProperties(findAndroidTargetDevice(tn.Text));
            else
                refreshDeviceProperties(null);
        }

        private void buttonDevicesConnectAgent_Click(object sender, EventArgs e)
        {
            string selectedDevice = getSelectedTargetDeviceName();

            if (selectedDevice == null)
                return;

            this.Enabled = false;
            string status = Marshal.PtrToStringAnsi(DaVinciAPI.NativeMethods.GetSpecifiedDeviceStatus(selectedDevice));
            bool connected = false;
            if (StatusConvertion(status) == DaVinciAPI.TargetDeviceConnStatus.TargetDeviceConnStatusConnected)
            {
                DaVinciAPI.NativeMethods.Disconnect(selectedDevice);
            }
            else if (StatusConvertion(status) == DaVinciAPI.TargetDeviceConnStatus.TargetDeviceConnStatusReadyToConnect)
            {
                if (0 == DaVinciAPI.NativeMethods.Connect(selectedDevice))
                    connected = true;
            }

            refreshDeviceProperties();
            mainWindow.UpdateStatusAgent(connected);
            this.Enabled = true;
        }

        private string GetCurrentTargetDevice()
        {
            const int maxLen = 1024;
            StringBuilder curDeviceName = new StringBuilder().Append('c', maxLen);
            DaVinciAPI.NativeMethods.GetCurrentDevice(curDeviceName);
            return curDeviceName.ToString();
        }

        private void treeViewDevices_AfterSelect(object sender, TreeViewEventArgs e)
        {
            string selectedDevice = getSelectedTargetDeviceName();
            if (selectedDevice == null)
                buttonMakeCurrent.Enabled = false;
            else if (StatusConvertion(Marshal.PtrToStringAnsi(DaVinciAPI.NativeMethods.GetSpecifiedDeviceStatus(selectedDevice))) == DaVinciAPI.TargetDeviceConnStatus.TargetDeviceConnStatusReadyToConnect)
                buttonMakeCurrent.Enabled = true;
            else
                buttonMakeCurrent.Enabled = false;

            refreshDeviceProperties();
        }

        private void treeViewDevices_BeforeSelect(object sender, TreeViewCancelEventArgs e)
        {
            if (treeViewDevices.SelectedNode != null)
                treeViewDevices.SelectedNode.BackColor = DaVinciCommon.IntelWhite;
            e.Node.BackColor = DaVinciCommon.IntelLightGrey;
        }

        private void comboBoxDeviceFfrdController_SelectedIndexChanged(object sender, EventArgs e)
        {
            string comName = null;
            if (comboBoxDeviceFfrdController.SelectedIndex < 0)
                return;
            string selectedDevice = getSelectedTargetDeviceName();
            if (selectedDevice == null)
            {
                return;
            }
            comName = comboBoxDeviceFfrdController.SelectedItem.ToString();

            this.Enabled = false;
            if (selectedDevice == GetCurrentTargetDevice())
            {
                DaVinciAPI.NativeMethods.SetCurrentHWAccessoryController(comName);
            }
            else
            {
                DaVinciAPI.NativeMethods.SetHWAccessoryControllerForTargetDevice(selectedDevice, comName);
            }
            this.Enabled = true;

            if (DaVinciAPI.NativeMethods.SaveConfigureData() != 0)
            {
                MessageBox.Show("Cannot save configure data correctly!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
        }

        private void comboBoxPlayToUser_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (comboBoxPlayToUser.SelectedIndex < 0) return;
            DaVinciAPI.NativeMethods.SetAudioDevice(
                null, DaVinciAPI.AudioDeviceType.AudioDeviceTypePlayDeviceAudio, comboBoxPlayToUser.SelectedItem.ToString());
        }

        private void comboBoxRecordFromUser_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (comboBoxRecordFromUser.SelectedIndex < 0) return;
            DaVinciAPI.NativeMethods.SetAudioDevice(
                null, DaVinciAPI.AudioDeviceType.AudioDeviceTypeRecordFromUser, comboBoxRecordFromUser.SelectedItem.ToString());
        }

        private void ConfigureForm_FormClosed(object sender, FormClosedEventArgs e)
        {
            if (DaVinciAPI.NativeMethods.SaveConfigureData() != 0)
            {
                MessageBox.Show("Cannot save configure data correctly!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
        }

        private void checkBoxOthersGPU_Click(object sender, EventArgs e)
        {
            if (checkBoxOthersGPU.Checked == false)
            {
                checkBoxOthersGPU.CheckState = CheckState.Unchecked;
                DaVinciAPI.NativeMethods.EnableDisableGpuSupport(DaVinciAPI.BoolType.BoolFalse);
            }
            if (checkBoxOthersGPU.Checked == true)
            {
                checkBoxOthersGPU.CheckState = CheckState.Checked;
                DaVinciAPI.NativeMethods.EnableDisableGpuSupport(DaVinciAPI.BoolType.BoolTrue);
            }
            if (DaVinciAPI.NativeMethods.SaveConfigureData() != 0)
            {
                MessageBox.Show("Cannot save configure data correctly!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
        }
    }
}
