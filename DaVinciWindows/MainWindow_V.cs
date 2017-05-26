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
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using System.Threading;
using System.Timers;
using System.Web;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace DaVinci
{
    /// <summary>
    /// 
    /// </summary>
    public partial class MainWindow_V : Form
    {
        /// <summary>
        /// Current Image Box layout
        /// </summary>
        public DaVinciAPI.Orientation imageBoxLayout = DaVinciAPI.Orientation.OrientationPortrait;

        /// <summary>
        /// 
        /// </summary>
        public static MainWindow_V form;

        /// <summary>
        /// 
        /// </summary>
        public static TestProjectConfig currentTestProjectConfig = null;
        private int deviceCount = 0;

        private int[] pointer = new int[10] { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
        private bool[] mouseDown = new bool[10] { false, false, false, false, false, false, false, false, false, false };
        private int old_ptr_id = 0;
        private bool tagSet = false;

        private DaVinciAPI.TestStatusEventHandler testStatusHandler;

        private void SetToolTips()
        {
            toolTipMainWindow.SetToolTip(buttonAndroidBack, "Press BACK key on Android device");
            toolTipMainWindow.SetToolTip(buttonAndroidHome, "Press HOME key on Android device");
            toolTipMainWindow.SetToolTip(buttonAndroidMenu, "Press MENU key on Android device");
            toolTipMainWindow.SetToolTip(StopTest, "Stop current test");
            toolTipMainWindow.SetToolTip(buttonPower, "Wake up a device or put it into sleep");
            toolTipMainWindow.SetToolTip(buttonLightUp, "Wake up the device");
            toolTipMainWindow.SetToolTip(buttonVolumeUp, "Increase device sound volume");
            toolTipMainWindow.SetToolTip(buttonVolumeDown, "Decrease device sound volume");
            toolTipMainWindow.SetToolTip(buttonVolumeRecord, "Start/stop audio recording");
            toolTipMainWindow.SetToolTip(toolStripButtonNewProject, "Create a test project");
            toolTipMainWindow.SetToolTip(toolStripButtonOpenProject, "Open a test project");
            toolTipMainWindow.SetToolTip(toolStripButtonNewFolder, "Create a folder");
            toolTipMainWindow.SetToolTip(toolStripButtonCreateTest, "Create one specidied kind of test");
            toolTipMainWindow.SetToolTip(toolStripButtonEditTest, "Edit an existing test");
            toolTipMainWindow.SetToolTip(toolStripButtonDelete, "Delete folder or files in project view");
            toolTipMainWindow.SetToolTip(toolStripButtonRefresh, "Refresh the project view");
            toolTipMainWindow.SetToolTip(RunTest, "Run a QScript");
            toolTipMainWindow.SetToolTip(buttonBrightnessUp, "Brighten the device");
            toolTipMainWindow.SetToolTip(buttonBrightnessDown, "Darken the device");
            toolTipMainWindow.SetToolTip(reportButton, "Show report");
            toolTipMainWindow.SetToolTip(cleanButton, "Clean log in LogPanel");
            toolTipMainWindow.SetToolTip(dumpUiButton, "Dump device UI layout");
        }

        void TestStatusEventHandler(DaVinciAPI.TestStatus status)
        {
            if (IsRecordingScript)
            {
                RefreshTestProject();
                IsRecordingScript = false;
            }
            if (IsReplayingScript)
            {
                RefreshTestProject();
                IsReplayingScript = false;
            }
        }

        // Check whether current image box is in vertical mode or not
        private bool isImageBoxVertical()
        {
            switch(getImageBoxLayout())
            {
                case DaVinciAPI.Orientation.OrientationLandscape:
                case DaVinciAPI.Orientation.OrientationReverseLandscape:
                    return false;
                default:
                    return true;
            }
        }

        private SetTextForm setTextForm = null;
        /// <summary>
        /// 
        /// </summary>
        public MainWindow_V()
        {
            form = this;

            InitializeComponent();

            SetToolTips();
            setImageBox(capturedImageBox);

            StartUpdateTextTimer();
            actViewInitWidth = this.tabPageActionsView.Width;
            actViewInitHeight = this.tabPageActionsView.Height;
        }

        /// <summary>
        /// 
        /// </summary>
        public int displayCapturedImageLock = 0;
        /// <summary>
        /// 
        /// </summary>
        public DaVinciAPI.ImageInfo saveImage;
        /// <summary>
        /// 
        /// </summary>
        /// <param name="img"></param>
        public void ShowImgInMainWindow(DaVinciAPI.ImageInfo img)
        {
            if (form != null && !form.IsDisposed)
            {
                if (form.capturedImageBox != null
                    && form.capturedImageBox.InvokeRequired
                    && !form.capturedImageBox.IsDisposed)
                {
                    if (Interlocked.CompareExchange(ref displayCapturedImageLock, 1, 0) == 0)
                    {
                        form.capturedImageBox.BeginInvoke((Action<DaVinciAPI.ImageInfo>)ShowImgInMainWindow, img);
                    }
                    else
                    {
                        DaVinciAPI.NativeMethods.ReleaseImage(ref img);
                    }
                }
                else
                {
                    if (form.capturedImageBox != null && form.capturedImageBox.IsDisposed == false)
                    {
                        form.capturedImageBox.Image = img;
                        saveImage = form.capturedImageBox.Image;
                    }
                    Interlocked.Exchange(ref displayCapturedImageLock, 0);
                }
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="brush"></param>
        /// <param name="coorX"></param>
        /// <param name="coorY"></param>
        /// <param name="orientation"></param>
        /// <param name="radius"></param>
        public void DrawDotOnImageBox(Brush brush, float coorX, float coorY, DaVinciAPI.Orientation orientation ,float radius)
        {
            if (capturedImageBox != null && !capturedImageBox.IsDisposed)
            {
                if (this.InvokeRequired && !this.IsDisposed)
                {
                    this.BeginInvoke((Action<Brush, float, float, DaVinciAPI.Orientation, float>)DrawDotOnImageBox,
                        brush, coorX, coorY, orientation, radius);
                }
                else
                {
                    calculateCoordinate(ref coorX, ref coorY, orientation, imageBoxLayout);
                    capturedImageBox.CreateGraphics().FillEllipse(brush, coorX - radius, coorY - radius, radius + radius, radius + radius);
                }
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="pen"></param>
        /// <param name="dot1"></param>
        /// <param name="dot2"></param>
        /// <param name="frameWidth"></param>
        /// <param name="frameHeight"></param>
        public void DrawLinesOnImageBox(Pen pen, DaVinciAPI.ActionDotInfo dot1, DaVinciAPI.ActionDotInfo dot2, int frameWidth, int frameHeight)
        {
            if (capturedImageBox != null && !capturedImageBox.IsDisposed)
            {
                if (this.InvokeRequired && !this.IsDisposed)
                {
                    this.BeginInvoke((Action<Pen, DaVinciAPI.ActionDotInfo, DaVinciAPI.ActionDotInfo, int, int>)DrawLinesOnImageBox, pen, dot1, dot2, frameWidth, frameHeight);
                }
                else
                {
                    mapCoordinate(ref dot1.coorX, ref dot1.coorY, frameWidth, frameHeight, imageBoxLayout, dot1.orientation); 
                    mapCoordinate(ref dot2.coorX, ref dot2.coorY, frameWidth, frameHeight, imageBoxLayout, dot1.orientation); 
                    capturedImageBox.CreateGraphics().DrawLine(pen, dot1.coorX, dot1.coorY, dot2.coorX, dot2.coorY); 

                }
            }
        }

        private void mapCoordinate(ref float coordinateX, ref float coordinateY, int frameWidth, int frameHeight, DaVinciAPI.Orientation imgBoxOri, DaVinciAPI.Orientation devOri)
        {
            if (((imgBoxOri == DaVinciAPI.Orientation.OrientationLandscape) && (devOri == DaVinciAPI.Orientation.OrientationPortrait)) ||
                ((imgBoxOri == DaVinciAPI.Orientation.OrientationLandscape) && (devOri == DaVinciAPI.Orientation.OrientationLandscape)) ||
                ((imgBoxOri == DaVinciAPI.Orientation.OrientationLandscape) && (devOri == DaVinciAPI.Orientation.OrientationReverseLandscape)) ||
                ((imgBoxOri == DaVinciAPI.Orientation.OrientationLandscape) && (devOri == DaVinciAPI.Orientation.OrientationReversePortrait)))
            {
                float ratioX = (float)capturedImageBox.Width / (float)frameWidth;
                float ratioY = (float)capturedImageBox.Height / (float)frameHeight;
                coordinateX *= ratioX;
                coordinateY *= ratioY;
            }
            else if (((imgBoxOri == DaVinciAPI.Orientation.OrientationPortrait) && (devOri == DaVinciAPI.Orientation.OrientationPortrait)) ||
                ((imgBoxOri == DaVinciAPI.Orientation.OrientationPortrait) && (devOri == DaVinciAPI.Orientation.OrientationLandscape)) ||
                ((imgBoxOri == DaVinciAPI.Orientation.OrientationPortrait) && (devOri == DaVinciAPI.Orientation.OrientationReversePortrait)) ||
                ((imgBoxOri == DaVinciAPI.Orientation.OrientationPortrait) && (devOri == DaVinciAPI.Orientation.OrientationReverseLandscape)))
            {
                float ratioX = (float)capturedImageBox.Width / (float)frameHeight;
                float ratioY = (float)capturedImageBox.Height / (float)frameWidth;
                float tempX = coordinateX;
                float tempY = coordinateY;
                coordinateY = tempX * ratioY;
                coordinateX = (frameHeight - tempY) * ratioX;
            }
            else if (((imgBoxOri == DaVinciAPI.Orientation.OrientationReverseLandscape) && (devOri == DaVinciAPI.Orientation.OrientationReverseLandscape)) ||
                ((imgBoxOri == DaVinciAPI.Orientation.OrientationReverseLandscape) && (devOri == DaVinciAPI.Orientation.OrientationLandscape)) ||
                ((imgBoxOri == DaVinciAPI.Orientation.OrientationReverseLandscape) && (devOri == DaVinciAPI.Orientation.OrientationReversePortrait)) ||
                ((imgBoxOri == DaVinciAPI.Orientation.OrientationReverseLandscape) && (devOri == DaVinciAPI.Orientation.OrientationPortrait)))
            {
                float ratioX = (float)capturedImageBox.Width / (float)frameWidth;
                float ratioY = (float)capturedImageBox.Height / (float)frameHeight;
                coordinateX = capturedImageBox.Width - coordinateX * ratioX;
                coordinateY = capturedImageBox.Height - coordinateY * ratioY;
            }
            else
            {
                float ratioX = (float)capturedImageBox.Width / (float)frameHeight;
                float ratioY = (float)capturedImageBox.Height / (float)frameWidth;
                float tempX = coordinateX;
                float tempY = coordinateY;
                coordinateY = (frameWidth - tempX) * ratioY;
                coordinateX = tempY * ratioX;
            }
        }

        private void calculateCoordinate(ref float coordinateX, ref float coordinateY, DaVinciAPI.Orientation devOri, DaVinciAPI.Orientation imgBoxOri)
        {
            if (((imgBoxOri == DaVinciAPI.Orientation.OrientationPortrait) && (devOri == DaVinciAPI.Orientation.OrientationPortrait)) ||
                ((imgBoxOri == DaVinciAPI.Orientation.OrientationLandscape) && (devOri == DaVinciAPI.Orientation.OrientationLandscape)) ||
                ((imgBoxOri == DaVinciAPI.Orientation.OrientationReversePortrait) && (devOri == DaVinciAPI.Orientation.OrientationReversePortrait)) ||
                ((imgBoxOri == DaVinciAPI.Orientation.OrientationReverseLandscape) && (devOri == DaVinciAPI.Orientation.OrientationReverseLandscape)))
            {
                coordinateX = coordinateX * capturedImageBox.Width / 4096;
                coordinateY = coordinateY * capturedImageBox.Height / 4096;
            }
            else if (((imgBoxOri == DaVinciAPI.Orientation.OrientationPortrait) && (devOri == DaVinciAPI.Orientation.OrientationReversePortrait)) ||
                ((imgBoxOri == DaVinciAPI.Orientation.OrientationLandscape) && (devOri == DaVinciAPI.Orientation.OrientationReverseLandscape)) ||
                ((imgBoxOri == DaVinciAPI.Orientation.OrientationReversePortrait) && (devOri == DaVinciAPI.Orientation.OrientationPortrait)) ||
                ((imgBoxOri == DaVinciAPI.Orientation.OrientationReverseLandscape) && (devOri == DaVinciAPI.Orientation.OrientationLandscape)))
            {
                coordinateX = (4096 - coordinateX) * capturedImageBox.Width / 4096;
                coordinateY = (4096 - coordinateY) * capturedImageBox.Height / 4096;
            }
            else if (((imgBoxOri == DaVinciAPI.Orientation.OrientationPortrait) && (devOri == DaVinciAPI.Orientation.OrientationReverseLandscape)) ||
                ((imgBoxOri == DaVinciAPI.Orientation.OrientationLandscape) && (devOri == DaVinciAPI.Orientation.OrientationPortrait)) ||
                ((imgBoxOri == DaVinciAPI.Orientation.OrientationReversePortrait) && (devOri == DaVinciAPI.Orientation.OrientationLandscape)) ||
                ((imgBoxOri == DaVinciAPI.Orientation.OrientationReverseLandscape) && (devOri == DaVinciAPI.Orientation.OrientationReversePortrait)))
            {
                float tempCoorX = coordinateX;
                float tempCoorY = coordinateY;
                coordinateX = tempCoorY * capturedImageBox.Width / 4096;
                coordinateY = (4096 - tempCoorX) * capturedImageBox.Height / 4096;
            }
            else
            {
                float tempCoorX = coordinateX;
                float tempCoorY = coordinateY;
                coordinateX = (4096 - tempCoorY) * capturedImageBox.Width / 4096;
                coordinateY = tempCoorX * capturedImageBox.Height / 4096;
            }
        }

        private Bitmap getResizedBitmap(Image image, int width, int height)
        {
            if (width > 0 && height > 0)
                return new Bitmap(image, new Size(width * 3 / 4, height * 3 / 4));
            else
                return null;
        }

        // This function is used to set all controls to their corresponding postions
        private void setControlView()
        {
            // if the form is too small, stop resizing for it will cause some issues
            // We use 200*200 as the minimum size
            const int minWidth = 200, minHeight = 200;
            const int padSize = 5;
            if (this.Height <= minHeight || this.Width <= minWidth)
            {
                return;
            }
            // Get Unit Size
            double unitHeight = (this.Height - this.Icon.Height) / 16.5;
            double unitWidth = this.Width / 25;
            float unitFontSize = 5.0F;
            unitFontSize = this.Height < this.Width ? (this.Height / minHeight) : (this.Width / minWidth);

            // Menu strip toolBar
            this.menuStripToolBar.Location = new System.Drawing.Point(0, 0);
            this.menuStripToolBar.Height = (int)(0.5 * unitHeight);

            // New Project button
            this.toolStripButtonNewProject.Location = new System.Drawing.Point((int)(padSize),(int)(this.menuStripToolBar.Location.Y + this.menuStripToolBar.Height + padSize));
            this.toolStripButtonNewProject.Width = (int)unitWidth;
            this.toolStripButtonNewProject.Height = (int)(unitHeight*4/5);

            // Open Project button
            this.toolStripButtonOpenProject.Location = new System.Drawing.Point((int)(this.toolStripButtonNewProject.Location.X + this.toolStripButtonNewProject.Width + padSize), (int)(this.toolStripButtonNewProject.Location.Y));
            this.toolStripButtonOpenProject.Width = (int)(unitWidth);
            this.toolStripButtonOpenProject.Height = (int)(unitHeight * 4 / 5);

            // New Folder button
            this.toolStripButtonNewFolder.Location = new System.Drawing.Point((int)(this.toolStripButtonOpenProject.Location.X + this.toolStripButtonOpenProject.Width + padSize), (int)(this.toolStripButtonNewProject.Location.Y));
            this.toolStripButtonNewFolder.Width = (int)(unitWidth);
            this.toolStripButtonNewFolder.Height = (int)(unitHeight * 4 / 5);

            // Create Test button
            this.toolStripButtonCreateTest.Location = new System.Drawing.Point((int)(this.toolStripButtonNewFolder.Location.X + this.toolStripButtonNewFolder.Width + padSize), (int)(this.toolStripButtonNewProject.Location.Y));
            this.toolStripButtonCreateTest.Width = (int)(unitWidth);
            this.toolStripButtonCreateTest.Height = (int)(unitHeight * 4 / 5);

            // Edit Test button
            this.toolStripButtonEditTest.Location = new System.Drawing.Point((int)(this.toolStripButtonCreateTest.Location.X + this.toolStripButtonCreateTest.Width + padSize), (int)(this.toolStripButtonNewProject.Location.Y));
            this.toolStripButtonEditTest.Width = (int)(unitWidth);
            this.toolStripButtonEditTest.Height = (int)(unitHeight * 4 / 5);

            // Delete button
            this.toolStripButtonDelete.Location = new System.Drawing.Point((int)(this.toolStripButtonEditTest.Location.X + this.toolStripButtonEditTest.Width + padSize), (int)(this.toolStripButtonNewProject.Location.Y));
            this.toolStripButtonDelete.Width = (int)(unitWidth);
            this.toolStripButtonDelete.Height = (int)(unitHeight * 4 / 5);
            // Refresh button
            this.toolStripButtonRefresh.Location = new System.Drawing.Point((int)(this.toolStripButtonDelete.Location.X + this.toolStripButtonDelete.Width + padSize), (int)(this.toolStripButtonNewProject.Location.Y));
            this.toolStripButtonRefresh.Width = (int)(unitWidth);
            this.toolStripButtonRefresh.Height = (int)(unitHeight * 4 / 5);

            // Project view & opcode buttons tabcontrol
            this.tabControlProjectWActions.Location = new System.Drawing.Point(0, (int)(this.menuStripToolBar.Height + this.toolStripButtonNewProject.Height + padSize * 2));
            this.tabControlProjectWActions.Height = (int)(this.Height - this.Icon.Height - this.menuStripToolBar.Height - (this.toolStripButtonNewProject.Height + padSize * 1.5) * 2) - this.toolStripStatus.Height;
            this.tabControlProjectWActions.Width = this.toolStripButtonRefresh.Location.X + this.toolStripButtonRefresh.Width + padSize;
            this.tabControlProjectWActions.Font = new Font(tabControlProjectWActions.Font.Name, (float)(2.5 * unitFontSize), tabControlProjectWActions.Font.Style, tabControlProjectWActions.Font.Unit);

            // TreeView Project
            this.treeViewProject.Location = new System.Drawing.Point((int)(this.tabControlProjectWActions.Location.X), 0);
            this.treeViewProject.Height = this.tabPageProjectView.Height;
            this.treeViewProject.Width = this.tabPageProjectView.Width;
            this.treeViewProject.Font = new Font(treeViewProject.Font.Name, (float)(2.5 * unitFontSize), treeViewProject.Font.Style, treeViewProject.Font.Unit);

            // Run Test button
            this.RunTest.Location = new System.Drawing.Point(0, (int)(this.tabControlProjectWActions.Location.Y + this.tabControlProjectWActions.Height));
            this.RunTest.Height = (int)(unitHeight * 4 / 5);
            this.RunTest.Width = (this.tabControlProjectWActions.Width - padSize) / 2;

            // Stop Test Button
            this.StopTest.Location = new System.Drawing.Point((int)(this.RunTest.Location.X + this.RunTest.Width + padSize), (int)(this.RunTest.Location.Y));
            this.StopTest.Height = this.RunTest.Height;
            this.StopTest.Width = this.RunTest.Width;

            // Image Box
            this.capturedImageBox.Location = new System.Drawing.Point((int)(this.toolStripButtonRefresh.Location.X + this.toolStripButtonRefresh.Width + padSize), (int)(this.menuStripToolBar.Height + padSize));
            if (isImageBoxVertical())
            {
                // Image Box layout
                //this.capturedImageBox.Height = (int)(15*unitHeight);
                this.capturedImageBox.Height = this.tabControlProjectWActions.Location.Y + this.tabControlProjectWActions.Height - this.toolStripButtonNewProject.Location.Y;
                this.capturedImageBox.Width=  (int)(9*unitWidth);

                // Back, Home and Menu buttons
                this.buttonAndroidBack.Location = new System.Drawing.Point((int)(this.capturedImageBox.Location.X), (int)(this.RunTest.Location.Y));
                this.buttonAndroidBack.Height = this.RunTest.Height;
                this.buttonAndroidBack.Width = (int)((this.capturedImageBox.Width - padSize*2)/3);
                this.buttonAndroidHome.Location = new System.Drawing.Point((int)(this.buttonAndroidBack.Location.X + this.buttonAndroidBack.Width + padSize), (int)(this.buttonAndroidBack.Location.Y));
                this.buttonAndroidHome.Height = this.buttonAndroidBack.Height;
                this.buttonAndroidHome.Width = this.buttonAndroidBack.Width;
                this.buttonAndroidMenu.Location = new System.Drawing.Point((int)(this.buttonAndroidHome.Location.X + this.buttonAndroidHome.Width + padSize), (int)(this.buttonAndroidBack.Location.Y));
                this.buttonAndroidMenu.Height = this.buttonAndroidBack.Height;
                this.buttonAndroidMenu.Width = this.buttonAndroidBack.Width;

                // Power button, Light Up button, Volumn Up and Volumn Down buttons
                this.buttonPower.Location = new System.Drawing.Point((int)(this.capturedImageBox.Location.X + this.capturedImageBox.Width + padSize), (int)(this.capturedImageBox.Location.Y));
                this.buttonPower.Width = (int)unitWidth;
                this.buttonPower.Height = (int)unitHeight;
                this.buttonPower.Image = getResizedBitmap(Properties.Resources.power, this.buttonPower.Width, this.buttonPower.Height);
                
                this.buttonLightUp.Location = new System.Drawing.Point((int)(this.buttonPower.Location.X), (int)(this.buttonPower.Location.Y + this.buttonPower.Height + padSize));
                this.buttonLightUp.Width = this.buttonPower.Width;
                this.buttonLightUp.Height = this.buttonPower.Height;
                this.buttonLightUp.Image = getResizedBitmap(Properties.Resources.lightup, this.buttonLightUp.Width, this.buttonLightUp.Height);

                this.buttonVolumeUp.Location = new System.Drawing.Point((int)(this.buttonPower.Location.X), (int)(this.buttonLightUp.Location.Y + this.buttonPower.Height + padSize));
                this.buttonVolumeUp.Width = this.buttonPower.Width;
                this.buttonVolumeUp.Height = this.buttonPower.Height;
                this.buttonVolumeUp.Image = getResizedBitmap(Properties.Resources.volumeup, this.buttonVolumeUp.Width, this.buttonVolumeUp.Height);

                this.buttonVolumeDown.Location = new System.Drawing.Point((int)(this.buttonPower.Location.X), (int)(this.buttonVolumeUp.Location.Y + this.buttonPower.Height + padSize));
                this.buttonVolumeDown.Width = this.buttonPower.Width;
                this.buttonVolumeDown.Height = this.buttonPower.Height;
                this.buttonVolumeDown.Image = getResizedBitmap(Properties.Resources.volumedown, this.buttonVolumeDown.Width, this.buttonVolumeDown.Height);

                this.buttonVolumeRecord.Location = new System.Drawing.Point((int)(this.buttonPower.Location.X), (int)(this.buttonVolumeDown.Location.Y + this.buttonPower.Height + padSize));
                this.buttonVolumeRecord.Width = this.buttonPower.Width;
                this.buttonVolumeRecord.Height = this.buttonPower.Height;
                this.buttonVolumeRecord.Image = getResizedBitmap(Properties.Resources.microphone, this.buttonVolumeRecord.Width, this.buttonVolumeRecord.Height);

                this.buttonBrightnessUp.Location = new System.Drawing.Point((int)(this.buttonPower.Location.X), (int)(this.buttonVolumeRecord.Location.Y + this.buttonPower.Height + padSize));
                this.buttonBrightnessUp.Width = this.buttonPower.Width;
                this.buttonBrightnessUp.Height = this.buttonPower.Height;
                this.buttonBrightnessUp.Image = getResizedBitmap(Properties.Resources.BrightnessUp, this.buttonBrightnessUp.Width, this.buttonBrightnessUp.Height);
                
                this.buttonBrightnessDown.Location = new System.Drawing.Point((int)(this.buttonPower.Location.X), (int)(this.buttonBrightnessUp.Location.Y + this.buttonPower.Height + padSize));
                this.buttonBrightnessDown.Width = this.buttonPower.Width;
                this.buttonBrightnessDown.Height = this.buttonPower.Height;
                this.buttonBrightnessDown.Image = getResizedBitmap(Properties.Resources.BrightnessDown, this.buttonBrightnessDown.Width, this.buttonBrightnessDown.Height);

                // Report button
                this.reportButton.Location = new System.Drawing.Point((int)(this.buttonPower.Location.X), (int)(this.buttonBrightnessDown.Location.Y + buttonPower.Height + padSize));
                this.reportButton.Width = this.buttonPower.Width;
                this.reportButton.Height = this.buttonPower.Height;
                this.reportButton.Image = getResizedBitmap(Properties.Resources.Report, this.reportButton.Width, this.reportButton.Height);

                // Clean button
                this.cleanButton.Location = new System.Drawing.Point((int)(this.buttonPower.Location.X), (int)(this.reportButton.Location.Y + buttonPower.Height + padSize));
                this.cleanButton.Width = this.buttonPower.Width;
                this.cleanButton.Height = this.buttonPower.Height;
                this.cleanButton.Image = getResizedBitmap(Properties.Resources.ClearLog, this.cleanButton.Width, this.cleanButton.Height);

                // Dump UI Layout button
                this.dumpUiButton.Location = new System.Drawing.Point((int)(this.buttonPower.Location.X), (int)(this.cleanButton.Location.Y + buttonPower.Height + padSize));
                this.dumpUiButton.Width = this.buttonPower.Width;
                this.dumpUiButton.Height = this.buttonPower.Height;
                this.dumpUiButton.Image = getResizedBitmap(Properties.Resources.dumpUiLayout, this.dumpUiButton.Width, this.dumpUiButton.Height);

                // TabControl which include Log Message and Performance tab
                this.tabControl.Location = new System.Drawing.Point((int)(this.buttonPower.Location.X + this.buttonPower.Width + padSize), (int)(this.capturedImageBox.Location.Y));
                this.tabControl.Height = this.capturedImageBox.Height + this.RunTest.Height;
                this.tabControl.Width = this.Width - (this.tabControlProjectWActions.Width + this.capturedImageBox.Width + padSize * 2 + this.buttonPower.Width + padSize);

                // RichText Message
                this.richTextBoxMessage.Height = this.tabPageLogMessage.Height;
                this.richTextBoxMessage.Width = this.tabPageLogMessage.Width;

                // Status bar
                this.toolStripStatus.Location = new System.Drawing.Point((int)(this.tabControl.Location.X), (int)(this.tabControl.Location.Y + this.tabControl.Height));
                this.toolStripStatus.Height = (this.Height - (this.Icon.Height + this.menuStripToolBar.Height + padSize + this.tabControl.Height + padSize))/2;
            }
            else
            {
                // Image Box layout
                this.capturedImageBox.Height = (int)(10 * unitHeight);
                this.capturedImageBox.Width = (int)(15 * unitWidth);

                // Back, Home and Menu buttons
                this.buttonAndroidBack.Location = new System.Drawing.Point((int)(this.capturedImageBox.Location.X + this.capturedImageBox.Width/3), (int)(this.capturedImageBox.Location.Y + this.capturedImageBox.Height + padSize));
                this.buttonAndroidBack.Height = (int)(unitHeight/3*2) - (int)(padSize * 2);
                this.buttonAndroidBack.Width = (int)(unitWidth*1.5);
                this.buttonAndroidHome.Location = new System.Drawing.Point((int)(this.buttonAndroidBack.Location.X + this.buttonAndroidBack.Width + padSize), (int)(this.buttonAndroidBack.Location.Y));
                this.buttonAndroidHome.Height = this.buttonAndroidBack.Height;
                this.buttonAndroidHome.Width = this.buttonAndroidBack.Width;
                this.buttonAndroidMenu.Location = new System.Drawing.Point((int)(this.buttonAndroidHome.Location.X + this.buttonAndroidHome.Width + padSize), (int)(this.buttonAndroidBack.Location.Y));
                this.buttonAndroidMenu.Height = this.buttonAndroidBack.Height;
                this.buttonAndroidMenu.Width = this.buttonAndroidBack.Width;

                // Power button, Light Up button, Volumn Up and Volumn Down buttons
                this.buttonPower.Location = new System.Drawing.Point((int)(this.capturedImageBox.Location.X + this.capturedImageBox.Width + padSize), (int)(this.capturedImageBox.Location.Y));
                this.buttonPower.Width = (int)unitWidth;
                this.buttonPower.Height = (int)unitHeight;
                this.buttonLightUp.Location = new System.Drawing.Point((int)(this.buttonPower.Location.X), (int)(this.buttonPower.Location.Y + this.buttonPower.Height + padSize));
                this.buttonLightUp.Width = (int)unitWidth;
                this.buttonLightUp.Height = (int)unitHeight;
                this.buttonVolumeUp.Location = new System.Drawing.Point((int)(this.buttonPower.Location.X), (int)(this.buttonLightUp.Location.Y + this.buttonLightUp.Height + padSize));
                this.buttonVolumeUp.Width = (int)unitWidth;
                this.buttonVolumeUp.Height = (int)unitHeight;
                this.buttonVolumeDown.Location = new System.Drawing.Point((int)(this.buttonPower.Location.X), (int)(this.buttonVolumeUp.Location.Y + this.buttonVolumeUp.Height + padSize));
                this.buttonVolumeDown.Width = (int)unitWidth;
                this.buttonVolumeDown.Height = (int)unitHeight;
                this.buttonVolumeRecord.Location = new System.Drawing.Point((int)(this.buttonPower.Location.X), (int)(this.buttonVolumeDown.Location.Y + this.buttonVolumeDown.Height + padSize));
                this.buttonVolumeRecord.Width = (int)unitWidth;
                this.buttonVolumeRecord.Height = (int)unitHeight;
                this.buttonBrightnessUp.Location = new System.Drawing.Point((int)(this.buttonPower.Location.X), (int)(this.buttonVolumeRecord.Location.Y + this.buttonVolumeRecord.Height + padSize));
                this.buttonBrightnessUp.Width = (int)unitWidth;
                this.buttonBrightnessUp.Height = (int)unitHeight;
                this.buttonBrightnessDown.Location = new System.Drawing.Point((int)(this.buttonPower.Location.X), (int)(this.buttonBrightnessUp.Location.Y + this.buttonBrightnessUp.Height + padSize));
                this.buttonBrightnessDown.Width = (int)unitWidth;
                this.buttonBrightnessDown.Height = (int)unitHeight;
                this.reportButton.Location = new System.Drawing.Point((int)(this.buttonPower.Location.X), (int)(this.buttonBrightnessDown.Location.Y + this.buttonBrightnessDown.Height) + padSize);
                this.reportButton.Width = (int)unitWidth;
                this.reportButton.Height = (int)unitHeight;
                this.cleanButton.Location = new System.Drawing.Point((int)(this.buttonPower.Location.X), (int)(this.reportButton.Location.Y + this.reportButton.Height) + padSize);
                this.cleanButton.Width = (int)unitWidth;
                this.cleanButton.Height = (int)unitHeight;
                this.dumpUiButton.Location = new System.Drawing.Point((int)(this.buttonPower.Location.X), (int)(this.cleanButton.Location.Y + this.cleanButton.Height) + padSize);
                this.dumpUiButton.Width = (int)unitWidth;
                this.dumpUiButton.Height = (int)unitHeight;

                // TabControl which include Log Message and Performance tab
                this.tabControl.Location = new System.Drawing.Point((int)(this.capturedImageBox.Location.X), (int)(this.menuStripToolBar.Height + this.capturedImageBox.Height + padSize * 2));
                this.tabControl.Height = this.Height - this.Icon.Height - (this.menuStripToolBar.Height + this.capturedImageBox.Height + padSize * 2);
                this.tabControl.Width = this.Width - this.tabControlProjectWActions.Width - padSize;
                
                // RichText Message
                this.richTextBoxMessage.Height = this.tabPageLogMessage.Height;
                this.richTextBoxMessage.Width = this.tabPageLogMessage.Width;
                
                // Status bar
                this.toolStripStatus.Location = new System.Drawing.Point((int)(this.capturedImageBox.Location.X), (int)(this.capturedImageBox.Location.Y + this.capturedImageBox.Height));
            }
            //richTextBoxMessage.Font = new Font(richTextBoxMessage.Font.Name, 2 * unitFontSize, richTextBoxMessage.Font.Style, richTextBoxMessage.Font.Unit);

            toolStripButtonNewProject.Font = new Font(toolStripButtonNewProject.Font.Name, 2.0F*unitFontSize, toolStripButtonNewProject.Font.Style, toolStripButtonNewProject.Font.Unit);
            toolStripButtonNewFolder.Font = new Font(toolStripButtonNewFolder.Font.Name, 2.0F * unitFontSize, toolStripButtonNewFolder.Font.Style, toolStripButtonNewProject.Font.Unit);
            toolStripButtonOpenProject.Font = new Font(toolStripButtonOpenProject.Font.Name, 2.0F * unitFontSize, toolStripButtonOpenProject.Font.Style, toolStripButtonOpenProject.Font.Unit);
            toolStripButtonEditTest.Font = new Font(toolStripButtonEditTest.Font.Name, 2.0F * unitFontSize, toolStripButtonEditTest.Font.Style, toolStripButtonEditTest.Font.Unit);
            toolStripButtonCreateTest.Font = new Font(toolStripButtonCreateTest.Font.Name, 2.0F * unitFontSize, toolStripButtonCreateTest.Font.Style, toolStripButtonCreateTest.Font.Unit);
            toolStripButtonDelete.Font = new Font(toolStripButtonDelete.Font.Name, 2.0F * unitFontSize, toolStripButtonDelete.Font.Style, toolStripButtonDelete.Font.Unit);
            toolStripButtonRefresh.Font = new Font(toolStripButtonRefresh.Font.Name, 2.0F * unitFontSize, toolStripButtonRefresh.Font.Style, toolStripButtonRefresh.Font.Unit);
        }

        private System.Timers.Timer agentCheckerTimer;
        private void startRefresher()
        {
            agentCheckerTimer = new System.Timers.Timer();
            agentCheckerTimer.Elapsed += new ElapsedEventHandler(timerBackToControl);
            agentCheckerTimer.Interval = 2000.0;
            agentCheckerTimer.AutoReset = false;
            agentCheckerTimer.Enabled = true;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="connected">agent connection</param>
        public void UpdateStatusAgent(bool connected)
        {
            if (this.InvokeRequired && !this.IsDisposed)
            {
                this.BeginInvoke((Action<bool>)UpdateStatusAgent, connected);
            }
            else
            {
                if (connected)
                {
                    this.toolStripStatusLabelAgent.Text = "Agent: Connected";
                    this.toolStripStatusLabelAgent.BackColor = DaVinciCommon.IntelGreen;
                }
                else
                {
                    this.toolStripStatusLabelAgent.Text = "Agent: Disconnected";
                    this.toolStripStatusLabelAgent.BackColor = DaVinciCommon.IntelRed;
                }
            }
        }

        private void UpdateStatusCoordinate(int x = -1, int y = -1)
        {
            string coordinateString;
            if (this.InvokeRequired && !this.IsDisposed)
            {
                this.BeginInvoke((Action<int, int>)UpdateStatusCoordinate, x, y);
            }
            else
            {
                if (x < 0 || x > 4096 || y < 0 || y > 4096)
                    coordinateString = "Coordinate X=?? Y=??";
                else
                    coordinateString = "Coordinate X=" + x.ToString() + " Y=" + y.ToString();
                this.toolStripStatusLabelCoordinate.Text = coordinateString;
            }
        }

        private void UpdateRunningModeAndBtnStatus(string mode)
        {
            if (this.InvokeRequired && !this.IsDisposed)
            {
                this.BeginInvoke((Action<string>)UpdateRunningModeAndBtnStatus, mode);
            }
            else
            {
                if (mode == "Stop")
                {
                    this.toolStripRunningMode.Text = "Running Mode: " + "Stop";
                    this.RunTest.Enabled = true;
                    this.toolStripButtonEditTest.Enabled = true;
                    this.toolStripButtonDelete.Enabled = true;

                }
                else if (mode == "Recording")
                {
                    this.toolStripRunningMode.Text = "Running Mode: " + "Recording";
                    this.RunTest.Enabled = false;
                    this.toolStripButtonEditTest.Enabled = false;
                    this.toolStripButtonDelete.Enabled = false;
                }
                else
                {
                    this.toolStripRunningMode.Text = "Running Mode: " + "Replaying";
                    this.toolStripButtonEditTest.Enabled = false;
                    this.toolStripButtonDelete.Enabled = false;
                }
            }
        }

        private void getDevicesCount()
        {
            foreach (string devName in TestUtil.targetDevices)
            {
                if (!Marshal.PtrToStringAnsi(DaVinciAPI.NativeMethods.GetSpecifiedDeviceStatus(devName)).Equals(
                    Marshal.PtrToStringUni(Marshal.StringToHGlobalUni("Offline"))
                    ))
                {
                    deviceCount++;
                }
            }
        }

        private void timerBackToControl(object sender, System.Timers.ElapsedEventArgs e)
        {
            if (form == null) return;
            deviceCount = 0;
            getDevicesCount();
            bool agentConnected = false;
            string checkDeviceName = "";
            if (TestUtil.targetDevices != null)
            {
                foreach (string devName in TestUtil.targetDevices)
                {
                    if (deviceCount >= 1)
                    {
                        const int maxLen = 1024;
                        StringBuilder curDevName = new StringBuilder().Append('c', maxLen);
                        DaVinciAPI.NativeMethods.GetCurrentDevice(curDevName);
                        checkDeviceName = curDevName.ToString();
                    }
                    else
                    {
                        checkDeviceName = "";
                    }
                    if (checkDeviceName != "")
                    {
                        if (DaVinciAPI.NativeMethods.IsDeviceConnected(checkDeviceName) == DaVinciAPI.BoolType.BoolTrue)
                        {
                            agentConnected = true;
                            UpdateStatusDeviceName(checkDeviceName);
                            break;
                        }
                        else
                        {
                            agentConnected = false;
                            UpdateStatusDeviceName(checkDeviceName);
                        }
                    }
                    else
                    {
                        agentConnected = false;
                        UpdateStatusDeviceName("None");
                    }
                }
            }
            else
            {
                UpdateStatusDeviceName("None");
            }
            // update agent status
            UpdateStatusAgent(agentConnected);

            if (form != null && !form.IsDisposed)
            {
                agentCheckerTimer.Interval = 4000.0;
                agentCheckerTimer.AutoReset = false;
                agentCheckerTimer.Enabled = true;
            }
            else
            {
                agentCheckerTimer.Enabled = false;
            }
            // Update running mode
            if (!IsRecordingScript && !IsReplayingScript)
            {
                UpdateRunningModeAndBtnStatus("Stop");
            }
            else
            {
                if (IsRecordingScript)
                {
                    UpdateRunningModeAndBtnStatus("Recording");
                }
                else
                {
                    UpdateRunningModeAndBtnStatus("Replaying");
                }
            }

            // Update test apk name
            UpdateTestApkName(getApkPackageName());
        }

        private void MainWindow_V_Load(object sender, EventArgs e)
        {
            richTextBoxMessage.LanguageOption = RichTextBoxLanguageOptions.UIFonts;
            setControlView();
            SetImageBoxRect(capturedImageBox.Width, capturedImageBox.Height);
            TestUtil.RefreshTestDevices();
            if (TestUtil.targetDevices != null)
            {
                foreach (string devName in TestUtil.targetDevices)
                {
                    if (DaVinciAPI.NativeMethods.IsDeviceConnected(devName) == DaVinciAPI.BoolType.BoolTrue)
                    {
                        UpdateStatusDeviceName(devName);
                        UpdateStatusAgent(true);
                        break;
                    }
                    else
                    {
                        UpdateStatusDeviceName("None");
                        UpdateStatusAgent(false);
                        break;
                    }
                }
            }
            else
            {
                UpdateStatusDeviceName("None");
                UpdateStatusAgent(false);
            }
            this.Refresh();
            getDevicesCount();
            startRefresher();

            testStatusHandler = new DaVinciAPI.TestStatusEventHandler(TestStatusEventHandler);
            DaVinciAPI.NativeMethods.SetTestStatusEventHandler(testStatusHandler);
            this.setTag(this);
            setControls(scaleWidth, scaleHeight, tabPageActionsView);
        }

        #region Methods and variables for UpdateText
        private System.Windows.Forms.Timer updateTextTimer = new System.Windows.Forms.Timer();
        private StringBuilder textBoxBuffer = new StringBuilder();
        private const int maxTextBoxBufferSize = 10 * 1024 * 1024;

        private void StartUpdateTextTimer()
        {
            // Start the timer to update the message box timely.
            // This aims to avoid the UI freeze caused by the flood
            // of BeginInvoke() calls triggered from CPP.
            updateTextTimer.Interval = 100; // 100ms is enough for good UX
            updateTextTimer.Tick += UpdateTextTimerHandler;
            updateTextTimer.Start();
        }

        private void StopUpdateTextTimer()
        {
            updateTextTimer.Stop();
        }

        /// <summary>
        /// Print DaVinci log on Main GUI's message box
        /// </summary>
        /// <param name="text"> A string to be shown in the message box and on console. </param>
        public void UpdateText(String text)
        {
            lock (textBoxBuffer)
            {
                if (textBoxBuffer.Length + text.Length < maxTextBoxBufferSize)
                {
                    textBoxBuffer.Append(text + "\r\n");
                }
            }
        }

        private void UpdateTextInternal(string text)
        {
            if (form != null && !form.IsDisposed && form.richTextBoxMessage != null && !form.richTextBoxMessage.IsDisposed)
            {
                // preserve the half MaxLength to avoid memory leakage when appending text to the text box
                if (form.richTextBoxMessage.Text.Length > form.richTextBoxMessage.MaxLength)
                {
                    form.richTextBoxMessage.Text = form.richTextBoxMessage.Text.Substring(form.richTextBoxMessage.Text.Length - form.richTextBoxMessage.MaxLength / 2);
                }
                form.richTextBoxMessage.AppendText(text);
            }
        }

        private void UpdateTextTimerHandler(Object myObject, EventArgs myEventArgs)
        {
            lock (textBoxBuffer)
            {
                if (form != null && !form.IsDisposed && form.richTextBoxMessage != null && !form.richTextBoxMessage.IsDisposed && textBoxBuffer.Length > 0)
                {
                    form.richTextBoxMessage.BeginInvoke((Action<String>)UpdateTextInternal, textBoxBuffer.ToString());
                }
                textBoxBuffer.Clear();
            }
        }
        #endregion
        /// <summary>
        /// Rotate the image box layout
        /// </summary>
        public void setImageBoxLayout()
        {
            switch(imageBoxLayout)
            {
                case DaVinciAPI.Orientation.OrientationPortrait:
                    imageBoxLayout = DaVinciAPI.Orientation.OrientationLandscape;
                    break;
                case DaVinciAPI.Orientation.OrientationLandscape:
                    imageBoxLayout = DaVinciAPI.Orientation.OrientationReversePortrait;
                    break;
                case DaVinciAPI.Orientation.OrientationReversePortrait:
                    imageBoxLayout = DaVinciAPI.Orientation.OrientationReverseLandscape;
                    break;
                case DaVinciAPI.Orientation.OrientationReverseLandscape:
                    imageBoxLayout = DaVinciAPI.Orientation.OrientationPortrait;
                    break;
                default:
                    imageBoxLayout = DaVinciAPI.Orientation.OrientationPortrait;
                    break;
            }
            DaVinciAPI.ImageBoxInfo imageBoxInfo;
            imageBoxInfo.orientation = imageBoxLayout;
            imageBoxInfo.height = capturedImageBox.Height;
            imageBoxInfo.width = capturedImageBox.Width;
            DaVinciAPI.NativeMethods.OnSetRotateImageBoxInfo(ref imageBoxInfo);
        }

        /// <summary>
        /// Get current image box layout
        /// </summary>
        /// <returns></returns>
        private DaVinciAPI.Orientation getImageBoxLayout()
        {
            return imageBoxLayout;
        }

        private int imageBoxWidth = 447, imageBoxHeight = 787;
        /// <summary>
        /// set width and height for imagebox
        /// </summary>
        /// <param name="width">the width of imagebox</param>
        /// <param name="height">the height of imagebox</param>
        private void SetImageBoxRect(int width, int height)
        {
            imageBoxWidth = width;
            imageBoxHeight = height;
        }

        static ImageBox imageBox = null;
        /// <summary>
        /// Set image box
        /// </summary>
        /// <param name="theImageBox"></param>
        private void setImageBox(ImageBox theImageBox)
        {
            SetImageBoxRect(theImageBox.Width, theImageBox.Height);
            imageBox = theImageBox;
        }


        private void refreshImageBoxLayout()
        {
            setControlView();
            SetImageBoxRect(capturedImageBox.Width, capturedImageBox.Height);
        }

        private void rotateToolStripMenuItem_Click(object sender, EventArgs e)
        {
            setImageBoxLayout();
            refreshImageBoxLayout();
        }

        private Hashtable nodeStatus = new Hashtable();

        // Load one layer in folder structures
        private void LoadProjectFilesInDirectory(string baseDirectory, TreeNode baseNode)
        {
            string[] supportExtension = { "*.qs", "*.xml" };
            string[] firstLayer = Directory.GetDirectories(baseDirectory);
            foreach (string firstDir in firstLayer)
            {
                TreeNode firstDirNode = new TreeNode(Path.GetFileName(firstDir));
                baseNode.Nodes.Add(firstDirNode);
                // If firstDir has subfolders, then TreeView should notify users this node is "clickable"
                // by adding a "DummyNode" to this node.
                string[] secondLayer = Directory.GetDirectories(firstDir);
                if (secondLayer.Length != 0)
                {
                    TreeNode secondDirNode = new TreeNode("DummyNode");
                    firstDirNode.Nodes.Add(secondDirNode);
                }
                else
                {
                    // Else if firstDir does not have subfolders but has files (*.qs, *.xml) to show, both of these
                    // two kinds of files will be added to the node.
                    foreach (string ext in supportExtension)
                    {
                        string[] scriptFilesToShow = Directory.GetFiles(firstDir, ext, SearchOption.TopDirectoryOnly);
                        foreach (string file in scriptFilesToShow)
                        {
                            TreeNode node = new TreeNode(Path.GetFileName(file));
                            node.ForeColor = Color.Purple;
                            firstDirNode.Nodes.Add(node);
                        }
                    }
                }
            }

            foreach (string ext in supportExtension)
            {
                string[] showFiles = Directory.GetFiles(baseDirectory, ext, SearchOption.TopDirectoryOnly);
                foreach (string file in showFiles)
                {
                    TreeNode node = new TreeNode(Path.GetFileName(file));
                    node.ForeColor = Color.Purple;
                    baseNode.Nodes.Add(node);
                }
            }
        }

        private void RestoreNodesStatus(TreeNode nodes)
        {
            foreach (TreeNode node in nodes.Nodes)
            {
                if (nodeStatus[node.FullPath] != null)
                {
                    if ((TreeViewAction)nodeStatus[node.FullPath] == TreeViewAction.Expand)
                    {
                        node.Expand();
                    }
                    else
                    {
                        node.Collapse();
                    }
                    RestoreNodesStatus(node);
                }
            }
        }

        private void LoadProjectFiles(TestProjectConfig config)
        {
            try
            {
                treeViewProject.Nodes.Clear();
                if (config == null)
                    return;
                if (!Directory.Exists(config.BaseFolder))
                {
                    MessageBox.Show("The project folder does not exists!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    return;
                }
                TreeNode root = new TreeNode(config.Name + " (" + config.BaseFolder + ")");
                treeViewProject.Nodes.Add(root);
                LoadProjectFilesInDirectory(config.BaseFolder, root);
                root.Expand();
                RestoreNodesStatus(root);

                if (this.reportButton.Enabled == false && Directory.Exists(dirName + "\\_Logs"))
                {
                    this.reportButton.Enabled = true;
                }
                
            }
            catch (Exception)
            {
                MessageBox.Show("Unable to enumerate project folder for Q scripts!", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            DaVinciAPI.TestProjectConfig testProjConfig;
            if (config.OSType == TestProjectConfig.OperatingSystemType.Android)
                testProjConfig.deviceType = DaVinciAPI.TargetDeviceType.TargetDeviceTypeAndroid;
            else if (config.OSType == TestProjectConfig.OperatingSystemType.Windows)
                testProjConfig.deviceType = DaVinciAPI.TargetDeviceType.TargetDeviceTypeWindows;
            else if (config.OSType == TestProjectConfig.OperatingSystemType.WindowsMobile)
                testProjConfig.deviceType = DaVinciAPI.TargetDeviceType.TargetDeviceTypeWindowsMobile;
            else if (config.OSType == TestProjectConfig.OperatingSystemType.ChromeOS)
                testProjConfig.deviceType = DaVinciAPI.TargetDeviceType.TargetDeviceTypeChrome;
            else if (config.OSType == TestProjectConfig.OperatingSystemType.iOS)
                testProjConfig.deviceType = DaVinciAPI.TargetDeviceType.TargetDeviceTypeiOS;
            else
            {
                MessageBox.Show("Unknown operating system type: " + config.OSType.ToString() + ", in project config!",
                    "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            testProjConfig.Name = config.Name;
            testProjConfig.BaseFolder = config.BaseFolder;
            testProjConfig.Application = config.Application;
            testProjConfig.ActivityName = config.ActivityName;
            testProjConfig.PackageName = config.PackageName;
            testProjConfig.PushDataSource = config.PushDataSource;
            testProjConfig.PushDataTarget = config.PushDataTarget;
            DaVinciAPI.NativeMethods.SetTestProjectConfig(ref testProjConfig);
        }

        private void LoadTestProjectConfig(TestProjectConfig config)
        {
            currentTestProjectConfig = config;
            if (config != null)
            {
                LoadProjectFiles(currentTestProjectConfig);
            }
        }

        private void toolStripButtonNewProject_Click(object sender, EventArgs e)
        {
            nodeStatus.Clear();
            using (TestProjectWizard wizard = new TestProjectWizard())
            {
                if (wizard.ShowDialog() == DialogResult.OK)
                {
                    SetProjectFileName(Path.Combine(wizard.Config.BaseFolder, wizard.Config.Name + ".qproj"));
                    LoadTestProjectConfig(wizard.Config);
                    setQSFileName(null);
                    dirName = currentTestProjectConfig.BaseFolder;
                    report_dirName = dirName;
                    this.reportButton.Enabled = false;
                }
            }
        }

        private void toolStripButtonOpenProject_Click(object sender, EventArgs e)
        {
            nodeStatus.Clear();
            using (OpenFileDialog dialog = new OpenFileDialog())
            {
                dialog.Filter = "DaVinci Test Project|*.qproj";
                dialog.FilterIndex = 1;
                dialog.Title = "Open a DaVinci Test Project Configuration";
                dialog.RestoreDirectory = true;
                if (dialog.ShowDialog() == DialogResult.OK && dialog.FileName != "")
                {
                    try
                    {
                        TestProjectConfig config = TestProjectConfig.Load(dialog.OpenFile());
                        config.BaseFolder = Path.GetDirectoryName(dialog.FileName);
                        SetProjectFileName(dialog.FileName);
                        LoadTestProjectConfig(config);
                        setQSFileName(null);
                        dirName = config.BaseFolder;
                        report_dirName = dirName;
                        if (Directory.Exists(dirName + "\\_Logs"))
                        {
                            this.reportButton.Enabled = true;
                        }
                    }
                    catch (Exception)
                    {
                        MessageBox.Show("Unable to load the test project file!", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
            }
        }

        /// <summary>
        /// 
        /// </summary>
        public static string dirName;
        /// <summary>
        /// 
        /// </summary>
        public static TreeNode curNode;
        private void toolStripButtonNewFolder_Click(object sender, EventArgs e)
        {
            if (currentTestProjectConfig == null)
                return;
            TreeNode currentDirNode = treeViewProject.SelectedNode;
            if (currentDirNode == null)
                currentDirNode = treeViewProject.Nodes[0];
            else if (currentDirNode.Text == "")
            {
                // Insert a directory when insert another.
                return;
            }
            else if (TestUtil.isQSTreeNode(currentDirNode))
                currentDirNode = currentDirNode.Parent;
            string dir = TestUtil.getNodeFullDirectoryPath(currentDirNode);
            TreeNode newNode = new TreeNode();
            currentDirNode.Nodes.Insert(0, newNode);
            treeViewProject.SelectedNode = newNode;
            newNode.BeginEdit();
            curNode = newNode;
        }
        static string report_dirName;
        private void treeViewProject_AfterSelect(object sender, TreeViewEventArgs e)
        {
            report_dirName = TestUtil.getNodeFullDirectoryPath(e.Node);

            if (Directory.Exists(report_dirName + "\\_Logs") == false)
            {
                this.reportButton.Enabled = false;
            }
            else
            {
                this.reportButton.Enabled = true;
            }

            if (TestUtil.isQSTreeNode(e.Node))
            {
                setQSFileName(TestUtil.getNodeFullPath(e.Node));
            }
            else
            {
                setQSFileName(null);
                curNode = treeViewProject.SelectedNode;
            }
        }

        private void treeViewProject_BeforeSelect(object sender, TreeViewCancelEventArgs e)
        {
            if (treeViewProject.SelectedNode != null)
            {
                treeViewProject.SelectedNode.BackColor = DaVinciCommon.IntelWhite;
            }
            e.Node.BackColor = DaVinciCommon.IntelLightGrey;
        }

        private void treeViewProject_AfterLabelEdit(object sender, NodeLabelEditEventArgs e)
        {
            char[] invalidChar = Path.GetInvalidFileNameChars();
            if (e.Node.Level <= 0 || TestUtil.isQSTreeNode(e.Node))
            {
                e.CancelEdit = true;
                return;
            }
            if (e.Node.Text.Trim() == "")
            {
                // Is a new node
                string dir = TestUtil.getNodeFullDirectoryPath(e.Node.Parent);
                if (((e.Label == null || e.Label.Trim() == "")) && ((this.ActiveControl != this.capturedImageBox)&&(this.ActiveControl != this.buttonAndroidBack)))
                {
                    MessageBox.Show("Folder name cannot be empty!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    e.Node.Parent.Nodes.Remove(e.Node);
                    return;
                }
                else
                {
                    bool invalidCheck = false;
                    foreach (char ch in invalidChar)
                    {
                        if (e.Label.Contains(ch))
                        {
                            invalidCheck = true;
                            break;
                        }
                    }
                    if (invalidCheck)
                    {
                        MessageBox.Show("Folder name should not contain invalid characters!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                        e.Node.Parent.Nodes.Remove(e.Node);
                        return;
                    }

                }
                string newDir = Path.Combine(dir, e.Label);
                if (Directory.Exists(newDir))
                {
                    MessageBox.Show(e.Label + " already exists.", "Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    e.Node.Parent.Nodes.Remove(e.Node);
                    return;
                }
                // Strange that we need to explicitly did this.
                e.Node.Text = e.Label;
                Directory.CreateDirectory(newDir);
                this.reportButton.Enabled = false;
            }
            else
            {
                // Change directory name
                if (e.Label == null)
                    return;
                if (e.Label.Trim() == "")
                {
                    MessageBox.Show("Folder name cannot be empty!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    e.CancelEdit = true;
                    return;
                }
                string dir = TestUtil.getNodeFullDirectoryPath(e.Node);
                string newDir = Path.Combine(Path.GetDirectoryName(dir), e.Label);

                if (Directory.Exists(newDir))
                {
                    MessageBox.Show(e.Label + " already exists.", "Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    e.CancelEdit = true;
                    return;
                }
                Directory.Move(dir, newDir);
            }
        }

        private void treeViewProject_BeforeLabelEdit(object sender, NodeLabelEditEventArgs e)
        {
            if (e.Node == null || e.Node.Level <= 0 || TestUtil.isQSTreeNode(e.Node))
            {
                e.CancelEdit = true;
            }
        }

        // Delete a directory or a file
        private void DeleteDirOrFile(string path)
        {
            if (Directory.Exists(path))
            {
                foreach (string d in Directory.GetFileSystemEntries(path))
                {
                    if (File.Exists(d))
                        File.Delete(d);
                    else
                        DeleteDirOrFile(d);
                }
                Directory.Delete(path);
            }
            else
                File.Delete(path);
        }

        private void toolStripButtonDelete_Click(object sender, EventArgs e)
        {
            if (treeViewProject.SelectedNode != null)
            {
                string timeStampFile = "";
                string aviFile = "";
                string path = TestUtil.getNodeFullPath(treeViewProject.SelectedNode);
                string dialogString = "The removed file(s) or directories will be permanently removed from disk. Are you sure to remove ";
                if (treeViewProject.SelectedNode == treeViewProject.Nodes[0])
                {
                    dialogString = dialogString + "the whole project?";
                }
                else if (TestUtil.isQSTreeNode(treeViewProject.SelectedNode))
                {
                    dialogString = dialogString + "QScript file " + Path.GetFileName(path) + "?";
                    if (path != null)
                    {
                        timeStampFile = path.Replace(".qs", ".qts");
                        aviFile = path.Replace(".qs", ".avi");
                    }
                }
                else
                {
                    dialogString = dialogString + "directory " + Path.GetFileName(path) + "?";
                }

                DialogResult result = MessageBox.Show(
                    dialogString,
                    "Confirm",
                    MessageBoxButtons.OKCancel,
                    MessageBoxIcon.Question);
                if (result == DialogResult.OK)
                {
                    try
                    {
                        DeleteDirOrFile(path);
                        if (File.Exists(timeStampFile))
                        {
                            DeleteDirOrFile(timeStampFile);
                        }
                        if (File.Exists(aviFile))
                        {
                            DeleteDirOrFile(aviFile);
                        }
                        if (treeViewProject.SelectedNode == treeViewProject.Nodes[0])
                        {
                            LoadTestProjectConfig(null);
                        }
                        treeViewProject.Nodes.Remove(treeViewProject.SelectedNode);
                    }
                    catch (Exception)
                    {
                        MessageBox.Show("Unable to delete " + path, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
            }
            //Set current and default APK to None
            currentApkLocation = "";
            defaultApkLocation = "";
        }


        static String qs_filename;
        static string qproj_filename = "";
        //static String qAudioFileName = "test";
        /// <summary>
        /// 
        /// </summary>
        /// <param name="fname"></param>
        public static void setQSFileName(String fname)
        {
            qs_filename = fname;
        }

        private static void SetProjectFileName(string fileName)
        {
            qproj_filename = fileName;
        }

        private static string GetProjectFileName()
        {
            return qproj_filename;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        public static String getQSFileName()
        {
            return qs_filename;
        }

        private void toolStripButtonEditTest_Click(object sender, EventArgs e)
        {
            QScriptEditor qsEditor = new QScriptEditor();
            qsEditor.Show();
            string file = getQSFileName();
            if (file != null)
            {
                qsEditor.loadQScript(getQSFileName());
            }
        }

        private void configureToolStripMenuItem_Click(object sender, EventArgs e)
        {
            using (ConfigureForm config_form = new ConfigureForm(this))
            {
                config_form.ShowDialog();
            }
        }

        /// <summary> Refresh test project on Main GUI. </summary>
        public void RefreshTestProject()
        {
            if (!this.IsDisposed)
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke((Action)RefreshTestProjectInternal);
                }
                else
                {
                    RefreshTestProjectInternal();
                }
            }
        }

        private void RefreshTestProjectInternal()
        {
            if (currentTestProjectConfig == null)
            {
                return;
            }
            TreeNode selectedNode = treeViewProject.SelectedNode;
            if (selectedNode == null)
            {
                selectedNode = treeViewProject.Nodes[0];
            }
            if (selectedNode.Text == "")
            {
                return;
            }
            LoadProjectFiles(currentTestProjectConfig);
        }

        private void Refresh_Click(object sender, EventArgs e)
        {
            RefreshTestProjectInternal();
        }

        private void buttonAndroidBack_MouseDown(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnButtonAction(DaVinciAPI.ButtonAction.ButtonActionBack, DaVinciAPI.ButtonActionType.Down);
        }

        private void buttonAndroidBack_MouseUp(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnButtonAction(DaVinciAPI.ButtonAction.ButtonActionBack, DaVinciAPI.ButtonActionType.Up);
        }

        private void buttonAndroidHome_MouseDown(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnButtonAction(DaVinciAPI.ButtonAction.ButtonActionHome, DaVinciAPI.ButtonActionType.Down);
        }

        private void buttonAndroidHome_MouseUp(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnButtonAction(DaVinciAPI.ButtonAction.ButtonActionHome, DaVinciAPI.ButtonActionType.Up);
        }

        private void buttonAndroidMenu_MouseDown(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnButtonAction(DaVinciAPI.ButtonAction.ButtonActionMenu, DaVinciAPI.ButtonActionType.Down);
        }

        private void buttonAndroidMenu_MouseUp(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnButtonAction(DaVinciAPI.ButtonAction.ButtonActionMenu, DaVinciAPI.ButtonActionType.Up);
        }

        private void capturedImageBox_Click(object sender, EventArgs e)
        {
            this.ActiveControl = this.capturedImageBox;
            capturedImageBox.Select();
        }

        private void capturedImageBox_MouseDown(object sender, MouseEventArgs e)
        {
            DaVinciAPI.MouseAction mouseAction;
            mouseAction.actionType = DaVinciAPI.MouseActionType.MouseActionTypeDown;
            mouseAction.button = DaVinciAPI.MouseButton.MouseButtonLeft;
            
            mouseAction.clicks = e.Clicks;
            mouseAction.delta = 0;
            if (old_ptr_id == 0)
            {
                mouseAction.ptr = -1;
            }
            else
            {
                old_ptr_id = 0;
                mouseAction.ptr = 0;
            }
            mouseAction.x = e.X;
            mouseAction.y = e.Y;

            DaVinciAPI.ImageBoxInfo imageBoxInfo;
            imageBoxInfo.height = this.capturedImageBox.Height;
            imageBoxInfo.width = this.capturedImageBox.Width;
            imageBoxInfo.orientation = DaVinciAPI.Orientation.OrientationPortrait;

            DaVinciAPI.NativeMethods.OnMouseAction(ref imageBoxInfo, ref mouseAction);
        }

        private void capturedImageBox_MouseEnter(object sender, EventArgs e)
        {
            this.Cursor = Cursors.Hand;
            this.ActiveControl = this.capturedImageBox;
        }

        private void capturedImageBox_MouseLeave(object sender, EventArgs e)
        {
            this.Cursor = Cursors.Default;
            this.ActiveControl = this.buttonAndroidBack;
            if (curNode != null && curNode.Text.Trim() == "")
            {
                curNode.BeginEdit();
            }
            UpdateStatusCoordinate();
        }

        private DaVinciAPI.MouseButton buttonActionTransform(System.Windows.Forms.MouseButtons button)
        {
            DaVinciAPI.MouseButton buttonAction;
            if (button == System.Windows.Forms.MouseButtons.Left)
            {
                buttonAction = DaVinciAPI.MouseButton.MouseButtonLeft;
            }
            else if (button == System.Windows.Forms.MouseButtons.Middle)
            {
                buttonAction = DaVinciAPI.MouseButton.MouseButtonMiddle;
            }
            else
            {
                buttonAction = DaVinciAPI.MouseButton.MouseButtonRight;
            }
            return buttonAction;
        }

        private void capturedImageBox_MouseMove(object sender, MouseEventArgs e)
        {
            DaVinciAPI.MouseAction mouseAction;
            mouseAction.button = buttonActionTransform(e.Button);
            if (mouseAction.button == DaVinciAPI.MouseButton.MouseButtonLeft)
            {
                mouseAction.actionType = DaVinciAPI.MouseActionType.MouseActionTypeMove;
                mouseAction.clicks = e.Clicks;
                mouseAction.delta = 0;
                if (old_ptr_id == 0)
                {
                    mouseAction.ptr = -1;
                }
                else
                {
                    old_ptr_id = 0;
                    mouseAction.ptr = 0;
                }
                mouseAction.x = e.X;
                mouseAction.y = e.Y;

                DaVinciAPI.ImageBoxInfo imageBoxInfo;
                imageBoxInfo.height = this.capturedImageBox.Height;
                imageBoxInfo.width = this.capturedImageBox.Width;
                imageBoxInfo.orientation = DaVinciAPI.Orientation.OrientationPortrait;

                DaVinciAPI.NativeMethods.OnMouseAction(ref imageBoxInfo, ref mouseAction);
            }
            int x_pos = (int)(e.X * 4096 / (this.capturedImageBox.Width));
            int y_pos = (int)(e.Y * 4096 / (this.capturedImageBox.Height));

            UpdateStatusCoordinate(x_pos, y_pos);
        }

        private void capturedImageBox_MouseUp(object sender, MouseEventArgs e)
        {
            DaVinciAPI.MouseAction mouseAction;
            mouseAction.actionType = DaVinciAPI.MouseActionType.MouseActionTypeUp;
            mouseAction.button = DaVinciAPI.MouseButton.MouseButtonLeft;

            mouseAction.clicks = e.Clicks;
            mouseAction.delta = 0;
            if (old_ptr_id == 0)
            {
                mouseAction.ptr = -1;
            }
            else
            {
                old_ptr_id = 0;
                mouseAction.ptr = 0;
            }
            mouseAction.x = e.X;
            mouseAction.y = e.Y;

            DaVinciAPI.ImageBoxInfo imageBoxInfo;
            imageBoxInfo.height = this.capturedImageBox.Height;
            imageBoxInfo.width = this.capturedImageBox.Width;
            imageBoxInfo.orientation = DaVinciAPI.Orientation.OrientationPortrait;

            DaVinciAPI.NativeMethods.OnMouseAction(ref imageBoxInfo, ref mouseAction);
        }

        /// <summary> true if this object is recording script. </summary>
        public bool IsRecordingScript = false;
        /// <summary> true if this object is replaying script. </summary>
        public bool IsReplayingScript = false;
        private void RunTest_Click(object sender, EventArgs e)
        {
            const int maxSize = 1024;
            IsRecordingScript = false;
            IsReplayingScript = true;
            StringBuilder deviceName = new StringBuilder(maxSize);
            if (DaVinciAPI.NativeMethods.GetCurrentTargetDevice(deviceName, maxSize) == DaVinciAPI.BoolType.BoolFalse)
            {
                // script will continue to run even if no target device is connected.
                UpdateText("Warning: No Current Device is set!");
            }
            if (DaVinciAPI.NativeMethods.GetTestStatus() == DaVinciAPI.TestStatus.TestStatusStopped)
            {
                if (getQSFileName() == null)
                {
                    UpdateText("Sorry, no QScript to run now, please load a qs first");
                    return;
                }
                DaVinciAPI.NativeMethods.ReplayQScript(getQSFileName());
            }
        }

        private void StopTest_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.StopTest();
        }

        /// <summary>
        /// 
        /// </summary>
        public static CreateTestSelect testSelectForm;

        private void toolStripButtonCreateTest_Click(object sender, EventArgs e)
        {
            testSelectForm = new CreateTestSelect();
            testSelectForm.ShowDialog();
        }

        private void MainWindow_V_Resize(object sender, EventArgs e)
        {
            setControlView();
            SetImageBoxRect(capturedImageBox.Width, capturedImageBox.Height);
            this.Refresh();
        }

        private void toolStripMenuItemHighResolution_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.Calibrate(1);
        }

        private void toolStripMenuItemFocusDistance_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.Calibrate(3);
        }

        private void toolStripMenuItemCamDistance_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.Calibrate(4);
        }

        private void toolStripMenuItemCamPara_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.Calibrate(5);
        }

        private void toolStripMenuItemOverallCali_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.Calibrate(6);
        }

        private void toolStripMenuItemResetCali_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.Calibrate(-1);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="name"></param>
        public void UpdateStatusDeviceName(string name)
        {
            if (this.InvokeRequired && !this.IsDisposed)
            {
                this.BeginInvoke((Action<string>)UpdateStatusDeviceName, name);
            }
            else
            {
                if (name == null || name == "" || name == "null")
                {
                    this.toolStripStatusLabelDevice.Text = "Device: None";
                }
                else
                {
                    this.toolStripStatusLabelDevice.Text = "Device: " + name;
                }
            }
        }

        private void UpdateTestApkName(string name)
        {
            if (this.InvokeRequired && !this.IsDisposed)
            {
                this.BeginInvoke((Action<string>)UpdateTestApkName, name);
            }
            else
            {
                if (name == null || name == "" || name == "null")
                {
                    this.toolStripStatusLabelApkName.Text = "Current APK/Package: None";
                    this.toolStripStatusLabelApkName.BackColor = DaVinciCommon.IntelWhite;
                }
                else
                {
                    this.toolStripStatusLabelApkName.Text = "Current APK/Package: " + name;
                    this.toolStripStatusLabelApkName.BackColor = DaVinciCommon.IntelYellow;
                }
            }
        }

        private void richTextBoxMessage_LinkClicked(object sender, LinkClickedEventArgs e)
        {
            try
            {
                System.Diagnostics.Process.Start(e.LinkText.Replace("%20", " "));
            }
            catch (System.Exception)
            {
                Console.WriteLine("richTextBoxMessage_LinkClicked exception!");
            }
        }

        private void report_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.GenerateReportIndex(report_dirName);

            string indexURL = report_dirName + "\\_Logs\\index.html";
            if (indexURL.Length < 3 || !File.Exists(indexURL))
            {
                Console.WriteLine("Error: Your URL " + indexURL + " is invalid !");
                return;
            }
            using (Process ps = new Process())
            {
                ps.StartInfo.FileName = "iexplore.exe";
                ps.StartInfo.Arguments = indexURL;
                ps.Start();
            }
        }

        private void cleanButton_Click(object sender, EventArgs e)
        {
            form.richTextBoxMessage.Clear();
        }

        private void capturedImageBox_DragDrop(object sender, DragEventArgs e)
        {
            string [] fileNames = (string[])(e.Data.GetData(DataFormats.FileDrop));
            foreach(string fileName in fileNames)
            {
                if (fileName.EndsWith(".apk"))
                {
                    StartWorkingThread(DaVinciAPI.AppLifeCycleAction.AppActionInstall, fileName);
                }
            }
        }

        private void capturedImageBox_DragEnter(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                e.Effect = DragDropEffects.All;
            }
            else
            {
                e.Effect = DragDropEffects.None;
            }
        }

        private void MainWindow_V_FormClosed(object sender, FormClosedEventArgs e)
        {
            StopUpdateTextTimer();
            DaVinciAPI.NativeMethods.SetTestStatusEventHandler(null);
            Console.WriteLine("MainWindow Closed");
            if (workingThread != null && workingThread.IsAlive == true)
            {
                workingThread.Abort();
                workingThread.Join();
            }
            DaVinciAPI.NativeMethods.StopTest();
            form = null;
        }

        private static String DaVinciDir = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);

        private void snapshotToolStripMenuItem_Click(object sender, EventArgs e)
        {
            String saveImgName = Path.Combine(DaVinciDir, "snapshot-" + System.DateTime.Now.ToString("yyyyMMddHHmmss") + ".png");
            if (DaVinciAPI.NativeMethods.SaveSnapshot(saveImgName) == DaVinciAPI.BoolType.BoolTrue)
            {
                UpdateText("Snapshot: file://" + saveImgName.Replace(" ", "%20"));
            }
        }

        private bool isAudioRecording = false;
        private void buttonVolumeRecord_Click(object sender, EventArgs e)
        {
            // If not in audio record process, click record button should update the background image to show that the microphone is in use,
            // if record stop, then background image should be updated to original one.

            if (isAudioRecording)
            {
                buttonVolumeRecord.Image = getResizedBitmap(Properties.Resources.microphone, buttonVolumeRecord.Width, buttonVolumeRecord.Height);
                isAudioRecording = false;
                DaVinciAPI.NativeMethods.OnAudioRecord(DaVinciAPI.BoolType.BoolFalse);
            }
            else
            {
                buttonVolumeRecord.Image = getResizedBitmap(Properties.Resources.microphone_inuse, buttonVolumeRecord.Width, buttonVolumeRecord.Height);
                isAudioRecording = true;
                DaVinciAPI.NativeMethods.OnAudioRecord(DaVinciAPI.BoolType.BoolTrue);
            }
        }

        private void buttonLightUp_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnButtonAction(DaVinciAPI.ButtonAction.ButtonActionLightUp);
        }

        private void buttonVolumeUp_MouseDown(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnButtonAction(DaVinciAPI.ButtonAction.ButtonActionVolumeUp, DaVinciAPI.ButtonActionType.Down);
        }

        private void buttonVolumeUp_MouseUp(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnButtonAction(DaVinciAPI.ButtonAction.ButtonActionVolumeUp, DaVinciAPI.ButtonActionType.Up);
        }

        private void buttonVolumeDown_MouseDown(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnButtonAction(DaVinciAPI.ButtonAction.ButtonActionVolumeDown, DaVinciAPI.ButtonActionType.Down);
        }

        private void buttonVolumeDown_MouseUp(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnButtonAction(DaVinciAPI.ButtonAction.ButtonActionVolumeDown, DaVinciAPI.ButtonActionType.Up);
        }

        private void buttonPower_MouseDown(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnButtonAction(DaVinciAPI.ButtonAction.ButtonActionPower, DaVinciAPI.ButtonActionType.Down);
        }

        private void buttonPower_MouseUp(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnButtonAction(DaVinciAPI.ButtonAction.ButtonActionPower, DaVinciAPI.ButtonActionType.Up);
        }

        private void buttonBrightnessUp_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnButtonAction(DaVinciAPI.ButtonAction.ButtonActionBrightnessUp);
        }

        private void buttonBrightnessDown_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnButtonAction(DaVinciAPI.ButtonAction.ButtonActionBrightnessDown);
        }

        private void highResolutionToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.SetCaptureResolution("HighResolution");
        }

        private void highSpeedToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.SetCaptureResolution("HighSpeed");
        }

        private void aIResolutionToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.SetCaptureResolution("AIResolution");
        }

        /// <summary>
        /// 
        /// </summary>
        public static string currentApkLocation = "";
        /// <summary>
        /// 
        /// </summary>
        public static string defaultApkLocation = "";
        private string getApkPackageName()
        {
            if (currentApkLocation != "")
            {
                return Path.GetFileName(currentApkLocation);
            }
            else
            {
                return "";
            }
        }

        private void SetCurrentAPK(string apkFileName)
        {
            string originalApkLocation = "";
            if (apkFileName.EndsWith(".apk"))
            {
                originalApkLocation = apkFileName;
            }
            else
            {
                MessageBox.Show("Not an APK file! Please drag-drop an APK file to set default APK!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return;
            }
            // Copy the apk file to the project folder
            string dstPath = Path.Combine(Path.GetDirectoryName(qproj_filename), Path.GetFileName(originalApkLocation));
            string srcPath = originalApkLocation;
            try
            {
                // If apk already exists in dstPath, do not copy the apk file to the desPath
                if (srcPath != dstPath)
                {
                    File.Copy(srcPath, dstPath, true);
                }
                currentApkLocation = dstPath;
                defaultApkLocation = currentApkLocation;
            }
            catch (Exception)
            {
                MessageBox.Show("Unable to copy APK to the project folder!",
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
            // Update project file
            currentTestProjectConfig.Name = Path.GetFileNameWithoutExtension(qproj_filename);
            currentTestProjectConfig.Application = Path.GetFileName(currentApkLocation);
            currentTestProjectConfig.PackageName = "";
            currentTestProjectConfig.ActivityName = "";
            int apkInfoSize = 0;
            string[] apkInfo = null;
            if (currentTestProjectConfig.Application != "")
            {
                // GetPackageActivity
                apkInfoSize = DaVinciAPI.NativeMethods.GetPackageActivity(null, currentApkLocation);
                if (apkInfoSize > 0)
                {
                    if (apkInfo == null)
                    {
                        apkInfo = new string[apkInfoSize];
                        for (int i = 0; i < apkInfoSize; i++)
                        {
                            apkInfo[i] = new StringBuilder().Append('0', 256).ToString();
                        }
                        DaVinciAPI.NativeMethods.GetPackageActivity(apkInfo, currentApkLocation);
                        currentTestProjectConfig.PackageName = apkInfo[0];
                        currentTestProjectConfig.ActivityName = apkInfo[1];
                    }
                }
                else
                {
                    MessageBox.Show("Unable to find package name: " + currentApkLocation,
                        "Warning",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Exclamation);
                }
            }
            // If recording does not begin, then if current APK has been switched to another one,
            // TestProjectConfig should be updated
            if (!IsRecordingScript)
            {
                // GetTestProject
                IntPtr testProjConfigPtr = DaVinciAPI.NativeMethods.GetTestProjectConfig();
                DaVinciAPI.TestProjectConfig tstProjConfig = (DaVinciAPI.TestProjectConfig)Marshal.PtrToStructure(testProjConfigPtr, typeof(DaVinciAPI.TestProjectConfig));
                // SetTestProject
                tstProjConfig.ActivityName = currentTestProjectConfig.ActivityName;
                tstProjConfig.PackageName = currentTestProjectConfig.PackageName;
                tstProjConfig.Application = currentTestProjectConfig.Application;
                DaVinciAPI.NativeMethods.SetTestProjectConfig(ref tstProjConfig);
            }
        }

        private void ResetCurrentAPK()
        {
            defaultApkLocation = "";
            currentApkLocation = "";
            // If user want to reset current apk, then apk info should be cleaned in project file
            if (currentTestProjectConfig != null)
            {
                currentTestProjectConfig.Application = null;
                currentTestProjectConfig.ActivityName = null;
                currentTestProjectConfig.PackageName = null;
                currentTestProjectConfig.Save(qproj_filename);
                RefreshTestProject();
                MessageBox.Show("Reset Current APK to None!", "Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
        }

        private void UpdateProjectFile()
        {
            try
            {
                if (!IsRecordingScript)
                {
                    currentTestProjectConfig.Save(qproj_filename);
                }
            }
            catch (Exception)
            {
                MessageBox.Show("Fail to update the project file!",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
                return;
            }
        }
        private void treeViewProject_DragDrop(object sender, DragEventArgs e)
        {
            if (GetProjectFileName() == "")
            {
                MessageBox.Show("Please New or Open a project first!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return;
            }
            // TODO: Need process drag more than 1 apks to project view panel
            string[] fileNames = (string[])(e.Data.GetData(DataFormats.FileDrop));
            foreach (string fileName in fileNames)
            {
                SetCurrentAPK(fileName);
            }
            UpdateProjectFile();
        }

        private void treeViewProject_DragEnter(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                e.Effect = DragDropEffects.All;
            }
            else
            {
                e.Effect = DragDropEffects.None;
            }
        }

        private void treeViewProject_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.Control && e.KeyCode == Keys.R)
            {
                ResetCurrentAPK();
            }
        }

        private void setCurrentAPKToolStripMenuItem_Click(object sender, EventArgs e)
        {
            string fileName = "";
            if (GetProjectFileName() == "")
            {
                MessageBox.Show("Please New or Open a project first!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return;
            }
            using (OpenFileDialog dialog = new OpenFileDialog())
            {
                dialog.Filter = "Android APK file|*.apk";
                dialog.FilterIndex = 1;
                dialog.Title = "Open an APK file";
                if (dialog.ShowDialog() == DialogResult.OK && dialog.FileName != "")
                {
                    fileName = dialog.FileName;
                    SetCurrentAPK(fileName);
                }
            }
            UpdateProjectFile();
        }

        private void resetCurrentAPKToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ResetCurrentAPK();
        }

        private void UpdateProgress(int value)
        {
            if (this.InvokeRequired && !this.IsDisposed)
            {
                this.BeginInvoke((Action<int>)UpdateProgress, value);
            }
            else
            {
                this.toolStripProgressBar.Value = value;
            }
        }

        private void SetProgressEnd()
        {
            if (backgroundWorker.IsBusy)
            {
                backgroundWorker.CancelAsync();
            }
            UpdateProgress(this.toolStripProgressBar.Maximum);
        }

        private void SetProgressBarProperty()
        {
            this.toolStripProgressBar.Value = 0;
            this.toolStripProgressBar.Visible = true;
            this.toolStripProgressBar.ForeColor = DaVinciCommon.IntelBlue;
        }
        private void EnableAppBtns(bool isEnable)
        {
            if (isEnable)
            {
                this.buttonUninstallApp.Enabled = true;
                this.buttonInstallApp.Enabled = true;
                this.buttonStartApp.Enabled = true;
                this.buttonStopApp.Enabled = true;
            }
            else
            {
                this.buttonUninstallApp.Enabled = false;
                this.buttonInstallApp.Enabled = false;
                this.buttonStartApp.Enabled = false;
                this.buttonStopApp.Enabled = false;
            }
        }
        private void EnableDataBtns(bool isEnable)
        {
            if (isEnable)
            {
                this.buttonPushData.Enabled = true;
                this.buttonClearData.Enabled = true;
            }
            else
            {
                this.buttonPushData.Enabled = false;
                this.buttonClearData.Enabled = false;
            }
        }

        Thread workingThread;
        private void StartWorkingThread(DaVinciAPI.AppLifeCycleAction action, string info1 = "", string info2 = "")
        {
            SetProgressBarProperty();
            backgroundWorker.WorkerReportsProgress = true;
            if (!backgroundWorker.IsBusy)
            {
                backgroundWorker.RunWorkerAsync();
            }
            try
            {
                workingThread = new Thread(() =>
                {
                    switch(action)
                    {
                        case DaVinciAPI.AppLifeCycleAction.AppActionInstall:
                            if (info1 != "")
                            {
                                DaVinciAPI.NativeMethods.OnAppAction(DaVinciAPI.AppLifeCycleAction.AppActionInstall, info1, "");
                            }
                            else
                            {
                                DaVinciAPI.NativeMethods.OnAppAction(DaVinciAPI.AppLifeCycleAction.AppActionInstall, currentApkLocation, "");
                            }
                            break;
                        case DaVinciAPI.AppLifeCycleAction.AppActionUninstall:
                            if (currentTestProjectConfig != null)
                            {
                                DaVinciAPI.NativeMethods.OnAppAction(DaVinciAPI.AppLifeCycleAction.AppActionUninstall, currentTestProjectConfig.PackageName, "");
                            }
                            break;
                        case DaVinciAPI.AppLifeCycleAction.AppActionPushData:
                            DaVinciAPI.NativeMethods.OnAppAction(DaVinciAPI.AppLifeCycleAction.AppActionPushData, info1, info2);
                            break;
                        case DaVinciAPI.AppLifeCycleAction.AppActionClearData:
                            if (currentTestProjectConfig != null)
                            {
                                DaVinciAPI.NativeMethods.OnAppAction(DaVinciAPI.AppLifeCycleAction.AppActionClearData, currentTestProjectConfig.PackageName, "");
                            }
                            break;
                        case DaVinciAPI.AppLifeCycleAction.AppActionStart:
                            if (currentTestProjectConfig != null)
                            {
                                DaVinciAPI.NativeMethods.OnAppAction(DaVinciAPI.AppLifeCycleAction.AppActionStart, currentTestProjectConfig.PackageName + "/" + currentTestProjectConfig.ActivityName, "");
                            }
                            break;
                        case DaVinciAPI.AppLifeCycleAction.AppActionStop:
                            if (currentTestProjectConfig != null)
                            {
                                DaVinciAPI.NativeMethods.OnAppAction(DaVinciAPI.AppLifeCycleAction.AppActionStop, currentTestProjectConfig.PackageName, "");
                            }
                            break;
                        default:
                            break;
                    }
                });
                workingThread.Name = "App Action Working Thread";
                workingThread.Start();
                if ((action != DaVinciAPI.AppLifeCycleAction.AppActionPushData)
                        && (action != DaVinciAPI.AppLifeCycleAction.AppActionClearData))
                {
                    EnableAppBtns(false);
                }
                else
                {
                    EnableDataBtns(false);
                }
                while (workingThread.IsAlive)
                {
                    Application.DoEvents();
                }
                if (!workingThread.IsAlive)
                {
                    SetProgressEnd();
                    Thread.Sleep(1000);
                    if ((action != DaVinciAPI.AppLifeCycleAction.AppActionPushData)
                        && (action != DaVinciAPI.AppLifeCycleAction.AppActionClearData))
                    {
                        EnableAppBtns(true);
                    }
                    else
                    {
                        EnableDataBtns(true);
                    }
                    this.toolStripProgressBar.Visible = false;
                }
            }
            catch (ThreadAbortException)
            {
                Thread.ResetAbort();
            }
        }
        private void buttonInstallApp_Click(object sender, EventArgs e)
        {
            StartWorkingThread(DaVinciAPI.AppLifeCycleAction.AppActionInstall);
        }

        private void buttonUninstallApp_Click(object sender, EventArgs e)
        {
            StartWorkingThread(DaVinciAPI.AppLifeCycleAction.AppActionUninstall);
        }

        private void buttonPushData_Click(object sender, EventArgs e)
        {
            string info1 = "";
            string info2 = "";
            IntPtr testProjConfigPtr = DaVinciAPI.NativeMethods.GetTestProjectConfig();
            DaVinciAPI.TestProjectConfig testProjConfig = (DaVinciAPI.TestProjectConfig)Marshal.PtrToStructure(testProjConfigPtr, typeof(DaVinciAPI.TestProjectConfig));
            using (PushDataConfigure pushDialog = new PushDataConfigure())
            {
                if (pushDialog.ShowDialog() == DialogResult.Cancel)
                {
                    info1 = pushDialog.textSourceContext;
                    info2 = pushDialog.textTargetContext;
                    testProjConfig.PushDataSource = info1;
                    testProjConfig.PushDataTarget = info2;
                    DaVinciAPI.NativeMethods.SetTestProjectConfig(ref testProjConfig);
                }
            }
            StartWorkingThread(DaVinciAPI.AppLifeCycleAction.AppActionPushData, info1, info2);
        }

        private void buttonClearData_Click(object sender, EventArgs e)
        {
            StartWorkingThread(DaVinciAPI.AppLifeCycleAction.AppActionClearData);
        }

        private void buttonStartApp_Click(object sender, EventArgs e)
        {
            StartWorkingThread(DaVinciAPI.AppLifeCycleAction.AppActionStart);
        }

        private void buttonStopApp_Click(object sender, EventArgs e)
        {
            StartWorkingThread(DaVinciAPI.AppLifeCycleAction.AppActionStop);
        }

        private void buttonOCR_Click(object sender, EventArgs e)
        {
            if (IsRecordingScript)
            {
                UpdateText("Adding an OCR opcode into a recorded QScript!");
                DaVinciAPI.NativeMethods.OnOcr();
            }
            else
            {
                UpdateText("You can only use OCR when recording a QScript!");
            }
        }

        private void buttonTilt_Click(object sender, EventArgs e)
        {
            using (TiltForm tiltForm = new TiltForm())
            {
                tiltForm.ShowDialog();
            }
        }

        private void buttonSetText_Click(object sender, EventArgs e)
        {
            setTextForm = new SetTextForm();
            setTextForm.ShowDialog();
        }

        /// <summary>
        /// 
        /// </summary>
        public static GetRnRWizard getRnRWizard;

        private void crossPlatformScriptingToolStripMenuItem_Click(object sender, EventArgs e)
        {
            getRnRWizard = new GetRnRWizard();
            getRnRWizard.ShowDialog();
        }

        private int degreeStep = 5;
        private int distanceStep = 1;
        private void tiltPadUpLeft_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnTilt(DaVinciAPI.TiltAction.TiltActionUpLeft, degreeStep, degreeStep);
        }

        private void tiltPadUp_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnTilt(DaVinciAPI.TiltAction.TiltActionUp, degreeStep, 0);
        }

        private void tiltPadUpRight_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnTilt(DaVinciAPI.TiltAction.TiltActionUpRight, degreeStep, degreeStep);
        }

        private void tiltPadLeft_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnTilt(DaVinciAPI.TiltAction.TiltActionLeft, degreeStep, 0);
        }

        private void tiltPadReset_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnTilt(DaVinciAPI.TiltAction.TiltActionReset, 90, 90);
        }

        private void tiltPadRight_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnTilt(DaVinciAPI.TiltAction.TiltActionRight, degreeStep, 0);
        }

        private void tiltPadDownLeft_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnTilt(DaVinciAPI.TiltAction.TiltActionDownLeft, degreeStep, degreeStep);
        }

        private void tiltPadDown_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnTilt(DaVinciAPI.TiltAction.TiltActionDown, degreeStep, 0);
        }

        private void tiltPadDownRight_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnTilt(DaVinciAPI.TiltAction.TiltActionDownRight, degreeStep, degreeStep);
        }

        private void AxisZClockwiseButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnZRotate(DaVinciAPI.ZAXISTiltAction.ZAXISTiltActionClockwise, degreeStep, 2);
        }

        private void AxisZAntiClockwiseButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnZRotate(DaVinciAPI.ZAXISTiltAction.ZAXISTiltActionAnticlockwise, degreeStep, 2);
        }

        private void AxisZResetButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnZRotate(DaVinciAPI.ZAXISTiltAction.ZAXISTiltActionReset, 360, 2);
        }

        private void PowBtnPushPressButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnPowerButtonPusher(DaVinciAPI.PowerButtonPusherAction.PowerButtonPusherPress);
        }

        private void PowBtnPushReleaseButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnPowerButtonPusher(DaVinciAPI.PowerButtonPusherAction.PowerButtonPusherRelease);
        }

        private void PowBtnPushShortPressButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnPowerButtonPusher(DaVinciAPI.PowerButtonPusherAction.PowerButtonPusherShortPress);
        }

        private void CamUpButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnMoveHolder(distanceStep);
        }

        private void CamDownButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnMoveHolder(distanceStep * (-1));
        }

        private void EarPhonePushButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnEarphonePuller(DaVinciAPI.EarphonePullerAction.EarphonePullerPlugIn);
        }

        private void EarPhonePullButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnEarphonePuller(DaVinciAPI.EarphonePullerAction.EarphonePullerPullOut);
        }

        private void USB1PlugInButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnUsbSwitch(DaVinciAPI.UsbSwitchAction.UsbOnePlugIn);
        }

        private void USB1PlugOutButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnUsbSwitch(DaVinciAPI.UsbSwitchAction.UsbOnePullOut);
        }

        private void USB2PlugInButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnUsbSwitch(DaVinciAPI.UsbSwitchAction.UsbTwoPlugIn);
        }

        private void USB2PlugOutButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnUsbSwitch(DaVinciAPI.UsbSwitchAction.UsbTwoPullOut);
        }

        // Set each component's tag with its initial size
        private void setTag(Control cons)
        {
            foreach (Control con in cons.Controls.Cast<Control>())
            {
                con.Tag = con.Width + ":" + con.Height + ":" + con.Location.X + ":" + con.Location.Y + ":" + con.Font.Size;
                if (con.Controls.Count > 0)
                    setTag(con);
            }
            this.tagSet = true;
        }

        private void setControls(float widthScale, float heightScale, Control cons)
        {
            // In case resizing was called before form load.
            if (!this.tagSet)
                return;

            foreach (Control con in cons.Controls.Cast<Control>())
            {
                try
                {
                    string[] mytag = con.Tag.ToString().Split(new char[] { ':' });
                    con.Width = (int)(Convert.ToSingle(mytag[0]) * widthScale);
                    con.Height = (int)(Convert.ToSingle(mytag[1]) * heightScale);
                    con.Location = new Point((int)(Convert.ToSingle(mytag[2]) * widthScale), (int)(Convert.ToSingle(mytag[3]) * heightScale));
                    Single currentSize = Convert.ToSingle(mytag[4]) * Math.Min(widthScale, heightScale);
                    con.Font = new Font(con.Font.Name, currentSize, con.Font.Style, con.Font.Unit);
                }
                catch
                {
                    //showLineLog("Warning: tag unset for control: "+ con.ToString());
                    continue;
                }
                if (con.Controls.Count > 0)
                {
                    setControls(widthScale, heightScale, con);
                }
            }
        }

        private float scaleWidth = 1.0f;
        private float scaleHeight = 1.0f;
        private int actViewInitWidth;
        private int actViewInitHeight;
        private void tabPageActionsView_Resize(object sender, EventArgs e)
        {
            scaleWidth = (float)tabPageActionsView.Width / (float)actViewInitWidth;
            scaleHeight = (float)tabPageActionsView.Height / (float)actViewInitHeight;
            setControls(scaleWidth, scaleHeight, tabPageActionsView);
        }

        private void userGuideToolStripMenuItem_Click(object sender, EventArgs e)
        {
            string home = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
            try
            {
                Process.Start(home + "\\..\\Docs");
            }
            catch (Exception)
            {
                // ignore any exception: directory not exist, permission denied etc.
            }
        }

        private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            using (AboutBox about = new AboutBox())
            {
                about.ShowDialog();
            }
        }

        private void templeRunToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.SVMPlayNoTilt();
        }

        private void treeViewProject_AfterExpand(object sender, TreeViewEventArgs e)
        {
            nodeStatus[e.Node.FullPath] = e.Action;
            if (e.Node.Parent != null)
            {
                // Clear the "DummyNode" if it exist as a sub-node under the selected node
                e.Node.Nodes.Clear();
                string expandNodePath = TestUtil.getNodeFullPath(e.Node);
                LoadProjectFilesInDirectory(expandNodePath, e.Node);
            }
        }

        private void treeViewProject_AfterCollapse(object sender, TreeViewEventArgs e)
        {
            nodeStatus[e.Node.FullPath] = e.Action;
        }

        private void backgroundWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            for (int i = toolStripProgressBar.Minimum; i < toolStripProgressBar.Maximum / 10 * 9; i++)
            {
                if (backgroundWorker.CancellationPending)
                {
                    e.Cancel = true;
                    return;
                }
                Thread.Sleep(300);
                backgroundWorker.ReportProgress(i);
            }
        }

        private void backgroundWorker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            UpdateProgress(e.ProgressPercentage);
        }

        private void dumpUiButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnButtonAction(DaVinciAPI.ButtonAction.ButtonActionDumpUiLayout);
        }

        private void Relay1ConnectButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnRelayController(DaVinciAPI.RelayControllerAction.RelayOneConnect);
        }

        private void Relay1DisonnectButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnRelayController(DaVinciAPI.RelayControllerAction.RelayOneDisconnect);
        }

        private void Relay2ConnectButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnRelayController(DaVinciAPI.RelayControllerAction.RelayTwoConnect);
        }

        private void Relay2DisonnectButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnRelayController(DaVinciAPI.RelayControllerAction.RelayTwoDisconnect);
        }

        private void Relay3ConnectButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnRelayController(DaVinciAPI.RelayControllerAction.RelayThreeConnect);
        }

        private void Relay3DisonnectButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnRelayController(DaVinciAPI.RelayControllerAction.RelayThreeDisconnect);
        }

        private void Relay4ConnectButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnRelayController(DaVinciAPI.RelayControllerAction.RelayFourConnect);
        }

        private void Relay4DisonnectButton_Click(object sender, EventArgs e)
        {
            DaVinciAPI.NativeMethods.OnRelayController(DaVinciAPI.RelayControllerAction.RelayFourDisconnect);
        }
    }
}
