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
using System.Runtime.InteropServices;
using System.Threading;
using System.Timers;
using System.Reflection;


namespace DaVinci
{
    /// <summary>
    /// 
    /// </summary>
    public partial class QScriptEditor : Form
    {
        /// <summary>
        /// 
        /// </summary>
        public QScriptEditor editor;
        // Record initialized size of form in order to resize each components when resizing
        private int initialWidth;
        private int initialHeight;
        private bool tagSet = false;

        private string[] paraIntCheckList = { "Coordinate", "Offset", "Point", "Degree", "Distance", "Instr Count", "Delta", "Type" };
        private string[] paraFileNameCheckList = { "File", "Image" };

        private System.IntPtr vidInst;

        /// <summary>
        /// 
        /// </summary>
        public QScriptEditor()
        {
            editor = this;
            InitializeComponent();
            this.initialWidth = this.Width;
            this.initialHeight = this.Height;
            ImageBoxChangingHandler();
            this.setComboBox();
        }

        // Script configuration
        /// <summary>
        /// 
        /// </summary>
        public class conftype
        {
            /// <summary>
            /// 
            /// </summary>
            public string name;
            /// <summary>
            /// 
            /// </summary>
            public string value;
            /// <summary>
            /// 
            /// </summary>
            /// <param name="s"></param>
            public conftype(string s)
            {
                this.name = s;
                this.value = "";
            }
            /// <summary>
            /// 
            /// </summary>
            /// <param name="s1"></param>
            /// <param name="s2"></param>
            public conftype(string s1, string s2)
            {
                this.name = s1;
                this.value = s2;
            }
            /// <summary>
            /// 
            /// </summary>
            /// <returns></returns>
            public override string ToString()
            {
                return (this.name + "=" + this.value);
            }
        };
        /// <summary>
        /// 
        /// </summary>
        public static System.Collections.Generic.List<conftype> configList;

        private bool scriptSave(string fileName)
        {
            if (this.scriptFile == null)
                return false;
            if (fileName != "")
            {
                this.scriptFile = fileName;
                this.Text = Path.GetFullPath(this.scriptFile);
            }
            using (StreamWriter sw = new StreamWriter(this.scriptFile))
            {
                sw.WriteLine("[Configuration]");
                foreach (conftype conf in configList)
                    sw.WriteLine(conf.ToString());
                sw.WriteLine("[Events and Actions]");
                foreach (actionlinetype action in this.actions)
                    sw.WriteLine(action.ToString());
            }
            return true;
        }

        private void showLineLogSync(string s)
        {
            // preserve the half MaxLength to avoid memory leakage when appending text to the text box
            if (this.textBoxLog.Text.Length > this.textBoxLog.MaxLength)
            {
                this.textBoxLog.Text = this.textBoxLog.Text.Substring(this.textBoxLog.Text.Length - this.textBoxLog.MaxLength / 2);
            }
            this.textBoxLog.Text += s + "\r\n";
            this.textBoxLog.SelectionStart = this.textBoxLog.Text.Length;
            this.textBoxLog.ScrollToCaret();
        }

        private void showLineLog(string s)
        {
            if (this.InvokeRequired)
            {
                this.BeginInvoke((Action<string>)showLineLogSync, s);
            }
            else
            {
                this.showLineLogSync(s);
            }
        }

        private void VideoFileSaveWorker(object sender, DoWorkEventArgs e)
        {
            string saveTo = (string)e.Argument;
            try
            {
                File.Copy(this.videoFile, saveTo, true);
                e.Result = saveTo;
            }
            catch
            {
                e.Result = null;
            }
        }

        private void VideoFileSaveCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            if (!this.IsDisposed)
            {
                DaVinci.TestUtil.EnableDisableControls(this, true);
                if (e.Result != null)
                {
                    this.videoFile = (string)e.Result;
                    //this.aviLoadToolStripButton.ToolTipText = "Load Avi File Only. Current: " + this.videoFile;
                    this.showLineLog("Complete saving video file " + e.Result);
                }
                else
                {
                    this.showLineLog("Failed to save video file!");
                }
            }
        }

        private bool timeSave(string fileName)
        {
            if (this.timeFile == null)
                return false;
            if (!File.Exists(this.timeFile))
            {
                showLineLog("Timestamp File " + this.timeFile + " does not exist. Fail to save timestamp file.");
                return false;
            }

            if (fileName == "")
            {
                if (String.Equals(this.scriptFile, Path.ChangeExtension(this.timeFile, ".qs")))
                {
                    return false;
                }
                else
                {
                    DialogResult r = MessageBox.Show("Timestamp is changed. Save it?", "Save Timestamp", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                    if (r == DialogResult.No)
                        return false;
                    else
                    {
                        try
                        {
                            File.Copy(this.timeFile, Path.ChangeExtension(this.scriptFile, ".qts"), true);
                        }
                        catch
                        {
                            return false;
                        }
                        this.timeFile = Path.ChangeExtension(this.scriptFile, ".qts");
                        return true;
                    }
                }
            }
            else if (String.Equals(fileName, this.timeFile))
            {
                return false;
            }
            else
            {
                try
                {
                    File.Copy(this.timeFile, fileName, true);
                }
                catch
                {
                    return false;
                }
                this.timeFile = fileName;
                return true;
            }
        }

        private bool videoSave(string fileName)
        {
            if (this.videoFile == null)
                return false;
            if (!File.Exists(this.videoFile))
            {
                showLineLog("Video File " + this.videoFile + " does not exist. Fail to save video file.");
                return false;
            }

            if (fileName == "")
            {
                if (String.Equals(this.scriptFile, Path.ChangeExtension(this.videoFile, ".qs")))
                {
                    return false;
                }
                else
                {
                    DialogResult r = MessageBox.Show("Video is changed. Save it?", "Save Video", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                    if (r == DialogResult.No)
                        return false;
                    else
                    {
                        DaVinci.TestUtil.EnableDisableControls(this, false);
                        this.showLineLog("Saving video file, please wait...");
                        using (BackgroundWorker worker = new BackgroundWorker())
                        {
                            worker.DoWork += VideoFileSaveWorker;
                            worker.RunWorkerCompleted += VideoFileSaveCompleted;
                            worker.RunWorkerAsync(Path.ChangeExtension(this.scriptFile, ".avi"));
                        }
                        return true;
                    }
                }
            }
            else if (String.Equals(fileName, this.videoFile))
            {
                return false;
            }
            else
            {
                DaVinci.TestUtil.EnableDisableControls(this, false);
                this.showLineLog("Saving video file, please wait...");
                using (BackgroundWorker worker = new BackgroundWorker())
                {
                    worker.DoWork += VideoFileSaveWorker;
                    worker.RunWorkerCompleted += VideoFileSaveCompleted;
                    worker.RunWorkerAsync(fileName);
                }
                return true;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        public void saveToFiles()
        {
            if (this.scriptSave(""))
            {
                this.isScriptFileChanged = false;
                // TODO: May need to support following toolstripstatus label
                //this.toolStripStatusLabel1.Text = "%%";
            }
            this.videoSave("");
            this.timeSave("");
        }

        private void saveToFiles(string scriptFileName, string videoFileName, string timeFileName)
        {
            if (this.scriptSave(scriptFileName))
            {
                this.isScriptFileChanged = false;
                // TODO: May need to update the toolStripStatusLabel
                //this.toolStripStatusLabel1.Text = "%%";
            }
            this.videoSave(videoFileName);
            this.timeSave(timeFileName);
        }

        private conftype parseConfigLine(string line)
        {
            if (line == null) return null;

            int length = 0, pos = 0;
            while (pos < line.Length && (line[pos] == ' ' || line[pos] == '\t'))
                pos++;
            int i = pos;
            while (i < line.Length && line[i] != ' ' && line[i] != '\t' && line[i] != '=')
            {
                i++;
                length++;
            }
            if (length <= 0 || i >= line.Length || line[i] != '=') return null;
            conftype conf = new conftype(line.Substring(pos, length));

            pos = pos + length + 1;
            while (pos < line.Length && (line[pos] == ' ' || line[pos] == '\t'))
                pos++;
            i = pos;
            length = 0;
            while (i < line.Length && line[i] != '\n')
            {
                i++;
                length++;
            }
            if (length > 0)
                conf.value = line.Substring(pos, length);

            return conf;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="line"></param>
        /// <returns></returns>
        public static actionlinetype parseActionLine(string line)
        {
            if (line == null) return null;

            List<string> tokens = new List<string>();
            int length = 0, pos = 0;
            int i, j;
            // get label and timestamp
            for (i = 0; i < line.Length && line[i] != '\n' && line[i] != ':'; i++)
            {
                char c = line[i];
                if (c == ' ' || c == '\t')
                {
                    if (length > 0)
                    {
                        tokens.Add(line.Substring(pos, length));
                        length = 0;
                    }
                    pos = i + 1;
                }
                else
                {
                    length++;
                }
            }

            if (length > 0)
                tokens.Add(line.Substring(pos, length));

            if (tokens.Count != 2)
                return null;

            length = 0;
            pos = i + 1;
            j = pos;
            string opcode = null;
            // get opcode
            for (j = i + 1; j < line.Length && line[j] != '\n'; j++)
            {
                char c = line[j];
                if (c == ' ' || c == '\t')
                {
                    if (length > 0)
                    {
                        break;
                    }
                    pos = j + 1;
                }
                else
                {
                    length++;
                }
            }
            if (length > 0)
            {
                opcode = line.Substring(pos, length);
                tokens.Add(opcode);
            }
            // No opcode
            if (opcode == null)
                return null;

            length = 0;
            pos = j + 1;
            // get the parameters
            if (opcode.Equals("OPCODE_ERROR") || opcode.Equals("OPCODE_MESSAGE"))
            {
                if (pos < line.Length)
                    tokens.Add(line.Substring(pos));
            }
            else
            {
                for (j = pos; j < line.Length && line[j] != '\n'; j++)
                {
                    char c = line[j];
                    if (c == ' ' || c == '\t')
                    {
                        if (length > 0)
                        {
                            tokens.Add(line.Substring(pos, length));
                            length = 0;
                        }
                        pos = j + 1;
                    }
                    else
                    {
                        length++;
                    }
                }

                if (length > 0)
                    tokens.Add(line.Substring(pos, length));
            }
            if (tokens.Count <= 2) return null;
            actionlinetype action = new actionlinetype();
            if (!int.TryParse(tokens[0], out action.label)) return null;
            if (!double.TryParse(tokens[1], out action.time)) return null;
            action.name = tokens[2];
            if (tokens.Count > 3)
            {
                action.parameters = new string[tokens.Count - 3];
                for (int k = 0; k < tokens.Count - 3; k++)
                {
                    action.parameters[k] = tokens[k + 3];
                }
            }
            return action;
        }

        // Check the qscript when loading
        // Now we only check the label duplicating
        private bool checkQScript(List<conftype> conf, List<actionlinetype> checkedActions)
        {
            bool result = true;
            // check label duplicating
            for (int i = 0; i < checkedActions.Count; i++)
            {
                int label = checkedActions[i].label;
                for (int j = i + 1; j < checkedActions.Count; j++)
                    if (checkedActions[j].label == label)
                    {
                        showLineLog("Warning: Label " + label.ToString() + " is duplicated.");
                        result = false;
                    }
            }
            return result;
        }

        private void flashListBoxActions()
        {
            this.listBoxActions.Items.Clear();
            for (int i = 0; i < this.actions.Count; i++)
                this.listBoxActions.Items.Add(this.actions[i].ToString());
            //Show an empty line in the end of the list for insertion
            this.listBoxActions.Items.Add("");
        }

        private void listBoxActionsSelect(int index)
        {
            this.listBoxActions.ClearSelected();
            this.listBoxActions.SetSelected(index, true);
        }

        private void setComboBox()
        {
            this.comboBoxAction.Items.AddRange(new object[] {
                "OPCODE_AI_BUILTIN",
                "OPCODE_AUDIO_MATCH_ON",
                "OPCODE_AUDIO_MATCH_OFF",
                "OPCODE_AUDIO_PLAY",
                "OPCODE_AUDIO_RECORD_START",
                "OPCODE_AUDIO_RECORD_STOP",
                "OPCODE_BACK",
                "OPCODE_BATCH_PROCESSING",
                "OPCODE_BRIGHTNESS",
                "OPCODE_CALL_EXTERNAL_SCRIPT",
                "OPCODE_CALL_SCRIPT",
                "OPCODE_CLEAR_DATA",
                "OPCODE_CLICK",
                "OPCODE_CLICK_MATCHED_IMAGE",
                "OPCODE_CLICK_MATCHED_IMAGE_XY",
                "OPCODE_CLICK_MATCHED_REGEX",
                "OPCODE_CLICK_MATCHED_TXT",
                "OPCODE_CONCURRENT_IF_MATCH",
                "OPCODE_CRASH_OFF",
                "OPCODE_CRASH_ON",
                "OPCODE_DEVICE_POWER",
                "OPCODE_DRAG",
                "OPCODE_DUMP_UI_LAYOUT",
                "OPCODE_ELAPSED_TIME",
                "OPCODE_ERROR",
                "OPCODE_EXIT",
                "OPCODE_FLICK_OFF",
                "OPCODE_FLICK_ON",
                "OPCODE_FPS_MEASURE_START",
                "OPCODE_FPS_MEASURE_STOP",
                "OPCODE_POWER_MEASURE_START",
                "OPCODE_POWER_MEASURE_STOP",
                "OPCODE_GOTO",
                "OPCODE_HOLDER_UP",
                "OPCODE_HOLDER_DOWN",
                "OPCODE_HOME",
                "OPCODE_IF_MATCH_AUDIO",
                "OPCODE_IF_MATCH_IMAGE",
                "OPCODE_IF_MATCH_IMAGE_WAIT",
                "OPCODE_IF_MATCH_REGEX",
                "OPCODE_IF_MATCH_TXT",
                "OPCODE_IF_SOURCE_IMAGE_MATCH",
                "OPCODE_IF_SOURCE_LAYOUT_MATCH",
                "OPCODE_IMAGE_CHECK",
                "OPCODE_IMAGE_MATCHED_TIME",
                "OPCODE_INSTALL_APP",
                "OPCODE_KEYBOARD_EVENT",
                "OPCODE_LOOP",
                "OPCODE_MATCH_OFF",
                "OPCODE_MATCH_ON",
                "OPCODE_MENU",
                "OPCODE_MESSAGE",
                "OPCODE_MOUSE_EVENT",
                "OPCODE_MULTI_TOUCHDOWN",
                "OPCODE_MULTI_TOUCHDOWN_HORIZONTAL",
                "OPCODE_MULTI_TOUCHMOVE",
                "OPCODE_MULTI_TOUCHMOVE_HORIZONTAL",
                "OPCODE_MULTI_TOUCHUP",
                "OPCODE_MULTI_TOUCHUP_HORIZONTAL",
                "OPCODE_NEXT_SCRIPT",
                "OPCODE_OCR",
                "OPCODE_PREVIEW",
                "OPCODE_PUSH_DATA",
                "OPCODE_SET_CLICKING_HORIZONTAL",
                "OPCODE_SET_TEXT",
                "OPCODE_SET_VARIABLE",
                "OPCODE_START_APP",
                "OPCODE_STOP_APP",
                "OPCODE_SWIPE",
                "OPCODE_TILTDOWN",
                "OPCODE_TILTLEFT",
                "OPCODE_TILTRIGHT",
                "OPCODE_TILTTO",
                "OPCODE_TILTUP",
                "OPCODE_TOUCHDOWN",
                "OPCODE_TOUCHDOWN_HORIZONTAL",
                "OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY",
                "OPCODE_TOUCHMOVE",
                "OPCODE_TOUCHMOVE_HORIZONTAL",
                "OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY",
                "OPCODE_TOUCHUP",
                "OPCODE_TOUCHUP_HORIZONTAL",
                "OPCODE_TOUCHUP_MATCHED_IMAGE_XY",
                "OPCODE_UNINSTALL_APP",
                "OPCODE_USB_IN",
                "OPCODE_USB_OUT",
                "OPCODE_VOLUME_DOWN",
                "OPCODE_VOLUME_UP",
            });
        }

        private void scriptLoad()
        {
            if (File.Exists(this.scriptFile))
            {
                String[] qs_lines = System.IO.File.ReadAllLines(this.scriptFile);
                int line = 0;

                // Go to the line of [Configuration]
                while (line < qs_lines.Length)
                {
                    if (qs_lines[line].IndexOf("[Configuration]") != -1)
                    {
                        line++;
                        break;
                    }
                    else
                        line++;
                }
                // Read configurations
                configList = new List<conftype>();
                while (line < qs_lines.Length && qs_lines[line].IndexOf("[Events and Actions]") == -1)
                {
                    conftype conf = parseConfigLine(qs_lines[line]);
                    if (conf != null)
                        configList.Add(conf);
                    else if (qs_lines[line].Length > 0)
                        showLineLog("Unrecognized config line: " + qs_lines[line]);
                    line++;
                }
                // TODO: Need to add back!
                //this.configComboBox.SelectedIndex = -1;

                // Read actions
                line++;
                this.actions = new List<actionlinetype>();
                while (line < qs_lines.Length)
                {
                    actionlinetype action = parseActionLine(qs_lines[line]);
                    if (action != null)
                    {
                        this.actions.Add(action);
                    }
                    else
                    {
                        showLineLog("Unrecognized action in line " + (line + 1).ToString() + ", which will be removed if you save the current scripts!");
                    }
                    line++;
                }

                checkQScript(configList, this.actions);
                flashListBoxActions();
                if (this.actions.Count > 0)
                    listBoxActionsSelect(0);
            }
        }

        /// <summary>
        /// 
        /// </summary>
        public int displayVideoImageLock = 0;

        private void ShowImage(DaVinciAPI.ImageInfo img)
        {
            if (editor != null)
            {
                if (editor.imageBox != null
                    && editor.imageBox.InvokeRequired
                    && !editor.imageBox.IsDisposed)
                {
                    if (Interlocked.CompareExchange(ref displayVideoImageLock, 1, 0) == 0)
                    {
                        editor.imageBox.BeginInvoke((Action<DaVinciAPI.ImageInfo>)ShowImage, img);
                    }
                    else
                    {
                        DaVinciAPI.NativeMethods.ReleaseImage(ref img);
                    }
                }
                else
                {
                    if (editor.imageBox != null && editor.imageBox.IsDisposed == false)
                    {
                        editor.imageBox.Image = img;
                    }
                    Interlocked.Exchange(ref displayVideoImageLock, 0);
                }
            }
        }

        Bitmap saveBitmap;
        DaVinciAPI.ImageInfo saveImg;

        private void DisplayImg(ref DaVinciAPI.ImageInfo img)
        {
            DaVinciAPI.ImageInfo showImg;

            saveBitmap = DaVinciCommon.ConvertToBitmap(img.iplImage);
            DaVinciAPI.NativeMethods.ReleaseImage(ref saveImg);
            saveImg = DaVinciAPI.NativeMethods.CopyImageInfo(ref img);
            if (isImageBoxVertical)
            {
                showImg = DaVinciAPI.NativeMethods.RotateImage(ref img, 90);
                DaVinciAPI.NativeMethods.ReleaseImage(ref img);
            }
            else
            {
                showImg = img;
            }
            
            ShowImage(showImg);
        }

        private void videoLoad()
        {
            foreach (conftype confItem in configList)
            {
                // TODO: Could the magis string "RecordedVideoType" be replaced by function?
                if (confItem.name.Equals("RecordedVideoType"))
                {
                    if (confItem.value.Trim().Length > 0)
                        this.videoFile = Path.ChangeExtension(videoFile, "." + confItem.value.Trim().ToLower());
                    break;
                }
            }
            
            if (File.Exists(videoFile))
            {
                // Resume
                this.timeStampList = null;
                this.qs_trace = null;
                int initFrameIndex = 0;

                int frameCount = 0;
                vidInst = DaVinciAPI.NativeMethods.OpenVideo(videoFile, ref frameCount);
                try
                {
                    DaVinciAPI.ImageEventHandler qScriptImageEventHandler = new DaVinciAPI.ImageEventHandler(DisplayImg);
                    GCHandle gcImageEventHandler = GCHandle.Alloc(qScriptImageEventHandler);
                    DaVinciAPI.NativeMethods.SetScriptVideoImgHandler(qScriptImageEventHandler);
                    DaVinciAPI.NativeMethods.ShowVideoFrame(vidInst, initFrameIndex);

                    if (frameCount >= 1)
                    {
                        this.trackBar.Maximum = frameCount - 1;
                    }
                    else
                    {
                        this.trackBar.Maximum = 0;
                    }
                    this.trackBar.Value = initFrameIndex;
                    this.trackBar.Enabled = true;
                    this.toolStripVideo.Enabled = true;
                }
                catch
                {
                    this.showLineLog("Video file \"" + videoFile + "\" cannot be loaded!");
                    this.trackBar.Enabled = false;
                    this.toolStripVideo.Enabled = false;
                    return;
                }
            }
            else
            {
                this.showLineLog("Video file \"" + videoFile + "\" doesn't exist!");
                this.trackBar.Enabled = false;
                this.toolStripVideo.Enabled = false;
                return;
            }

            if (File.Exists(timeFile))
            {
                // load time stamp file
                TimeStampFileReader qtsReader = new TimeStampFileReader(timeFile);
                this.timeStampList = qtsReader.getTimeStampList();
                this.qs_trace = qtsReader.getTrace();

                if (this.timeStampList.Count <= this.trackBar.Maximum)
                {
                    this.showLineLog("Warning: Time Stamp file \"" + timeFile + "\" has less time stamps than video!");
                    double time = this.timeStampList[this.timeStampList.Count - 1];
                    for (int i = this.timeStampList.Count; i <= this.trackBar.Maximum; i++)
                        this.timeStampList.Add(time);
                }
                else if (this.timeStampList.Count > this.trackBar.Maximum + 1)
                {
                    this.showLineLog("Warning: Time Stamp file \"" + timeFile + "\" has more time stamps than video.");
                    this.timeStampList.RemoveRange(this.trackBar.Maximum + 1, this.timeStampList.Count - this.trackBar.Maximum - 1);
                }
            }
            else
            {
                this.showLineLog("Time Stamp file \"" + timeFile + "\" doesn't exist!");
            }
        }

        /// <summary>
        /// Load QS file
        /// </summary>
        /// <param name="scriptName">QS file name</param>
        public void loadQScript(string scriptName)
        {
            this.scriptFile = scriptName;
            this.videoFile = Path.ChangeExtension(scriptFile, ".avi");
            //this.aviLoadToolStripButton.ToolTipText = "Load Avi File Only. Current: " + this.videoFile;
            this.timeFile = Path.ChangeExtension(scriptFile, ".qts");

            if (this.isScriptFileChanged)
            {
                if (MessageBox.Show("File has been changed and the change will be deleted.\nAre you sure to quit file " + this.Text + "?", "QScriptEditor", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.No)
                    return;
            }

            // Load the script file
            this.Text = Path.GetFullPath(this.scriptFile);
            this.isScriptFileChanged = false;
            // TODO: May need update toolStripStatusLabel
            //this.toolStripStatusLabel1.Text = "%%";

            this.scriptLoad();

            // Load the video file
            this.videoLoad();
        }

        private int getLastTraceNumberFromStamp(double stamp)
        {
            int i;
            for (i = 0; i < this.qs_trace.Count; i++)
            {
                if (this.qs_trace[i].time > stamp)
                    break;
            }
            return i - 1;
        }

        /// <summary>
        /// Get the stamp index in the stamp list whose stamp is right before 'stamp'.
        /// If not found, return -1.
        /// </summary>
        /// <param name="stamp"> the searching stamp </param>
        /// <returns> found stamp index</returns>
        private int getStampNumberBeforeStamp(double stamp)
        {
            int i;
            for (i = 0; i < this.timeStampList.Count; i++)
            {
                if (this.timeStampList[i] > stamp)
                    break;
            }
            return i - 1;
        }

        /// <summary>
        /// Get the frame number corresponding to the label, which is the frame right before the labeled action's execution.
        /// The searching starts from end+1 to end %trace count. 
        /// </summary>
        /// <param name="label">The action's label </param>
        /// <param name="end">The index where the searching ends</param>
        /// <returns>return the found index, -1 for not found.</returns>
        private int getFrameNumberFromLabel(int label, int end = 0)
        {
            if (end < this.trackBar.Minimum || end > this.trackBar.Maximum)
                return -1;

            int endTraceNumber = (getLastTraceNumberFromStamp(this.timeStampList[end]) + this.qs_trace.Count) % this.qs_trace.Count;
            for (int i = (endTraceNumber + 1) % this.qs_trace.Count; i != endTraceNumber; i = ((i + 1) % this.qs_trace.Count))
            {
                if (this.qs_trace[i].label == label)
                    return getStampNumberBeforeStamp(this.qs_trace[i].time);
            }

            if (this.qs_trace[endTraceNumber].label == label)
                return getStampNumberBeforeStamp(this.qs_trace[endTraceNumber].time);

            return -1;
        }

        private void listBoxActions_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            if (!File.Exists(timeFile))
            {
                MessageBox.Show("Time file (*.qts) is missing!",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
                return;
            }
            // change video frame
            if (this.actions == null || this.timeStampList == null ||
                this.listBoxActions.SelectedIndex < 0 || this.listBoxActions.SelectedIndex >= this.actions.Count ||
                this.listBoxActions.SelectedIndices.Count > 1)
                return;

            actionlinetype action = this.actions[this.listBoxActions.SelectedIndex];
            int timeIndex;

            if (this.qs_trace != null)
            {
                // Keep find the next execution of the label by double clicking
                timeIndex = getFrameNumberFromLabel(action.label, this.trackBar.Value);
                if (timeIndex >= 0)
                {
                    this.trackBar.Value = timeIndex;
                    // Following is for bug 3377 fix. The root cause for bug 3377 is:
                    // If double click the same OPCODE before and after rotate the ImageBox,
                    // The track bar value is not changed, thus the ValueChanged event will
                    // not be triggered and the red point will not be painted on the frame.
                    if (this.trackBar.Value < this.trackBar.Maximum)
                    {
                        this.trackBar.Value++;
                        this.trackBar.Value--;
                    }
                    else
                    {
                        // If double click the last OPCODE in QSEditor and value of trackBar is the maximum one,
                        // then decrease it first and increase it.
                        this.trackBar.Value--;
                        this.trackBar.Value++;
                    }
                    return;
                }
            }

            // If This action is not executed, find the corresponding frame based on the timestamp.
            for (timeIndex = 0; timeIndex < this.timeStampList.Count; timeIndex++)
            {
                if (this.timeStampList[timeIndex] > action.time)
                    break;
            }
            if (timeIndex <= 0)
            {
                this.trackBar.Value = 0;
            }
            else
            {
                this.trackBar.Value = timeIndex - 1;
            }
        }


        /// <summary>
        /// Enable or disable a control and all its parents except the topmost form.
        /// </summary>
        /// <param name="c"></param>
        /// <param name="enable">To enable or disable</param>
        private void EnableDisableControlChain(Control c, bool enable)
        {
            if (c == this)
            {
                return;
            }
            c.Enabled = enable;
            EnableDisableControlChain(c.Parent, enable);
        }

        private void decideDataGridViewComboBoxValue(DataGridViewComboBoxCell combox, string value)
        {
            if (value == null || value.Trim().Equals(""))
                return;
            for (int i = 0; i < combox.Items.Count; i++)
            {
                if (combox.Items[i].ToString().Equals(value))
                    combox.Value = value;
            }
        }

        private void flashDataGridViewActionProperty(actionlinetype actionline)
        {
            this.dataGridViewActionProperty.Rows.Clear();

            DataGridViewRow row;

            // label
            row = new DataGridViewRow();
            DataGridViewTextBoxCell labelName = new DataGridViewTextBoxCell();
            labelName.Value = "Label";
            row.Cells.Add(labelName);
            DataGridViewTextBoxCell labelValue = new DataGridViewTextBoxCell();
            labelValue.Value = actionline.label;
            row.Cells.Add(labelValue);
            this.dataGridViewActionProperty.Rows.Add(row);

            // time
            row = new DataGridViewRow();
            DataGridViewTextBoxCell timeName = new DataGridViewTextBoxCell();
            timeName.Value = "Time";
            row.Cells.Add(timeName);
            DataGridViewTextBoxCell timeValue = new DataGridViewTextBoxCell();
            timeValue.Value = (int)(actionline.time);
            row.Cells.Add(timeValue);
            this.dataGridViewActionProperty.Rows.Add(row);

            // parameters with information updating
            if (actionline.name == "OPCODE_SET_CLICKING_HORIZONTAL")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Orientation";
                row.Cells.Add(paraName);
                DataGridViewComboBoxCell paraValue = new DataGridViewComboBoxCell();
                paraValue.Items.Add("0");
                paraValue.Items.Add("1");
                //paraValue.Items.Add("2");
                //paraValue.Items.Add("3");
                if (actionline.parameters != null && actionline.parameters.Length == 1)
                    decideDataGridViewComboBoxValue(paraValue, actionline.parameters[0]);
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            if (actionline.name == "OPCODE_DEVICE_POWER" || actionline.name == "OPCODE_BRIGHTNESS")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Action";
                row.Cells.Add(paraName);
                DataGridViewComboBoxCell paraValue = new DataGridViewComboBoxCell();
                paraValue.Items.Add("0");
                paraValue.Items.Add("1");
                if (actionline.parameters != null && actionline.parameters.Length == 1)
                    decideDataGridViewComboBoxValue(paraValue, actionline.parameters[0]);
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_SET_TEXT")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Text";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null)
                {
                    foreach (string str in actionline.parameters)
                    {
                        paraValue.Value += " ";
                        paraValue.Value += str;
                    }
                    row.Cells.Add(paraValue);
                }
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_CLICK" ||
                     actionline.name == "OPCODE_TOUCHDOWN" ||
                     actionline.name == "OPCODE_TOUCHUP" ||
                     actionline.name == "OPCODE_TOUCHMOVE")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Coordinate X";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Coordinate Y";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "[Orientation]";
                row.Cells.Add(paraName);
                DataGridViewComboBoxCell paraValue1 = new DataGridViewComboBoxCell();
                paraValue1.Items.Add("0");
                paraValue1.Items.Add("1");
                paraValue1.Items.Add("2");
                paraValue1.Items.Add("3");
                if (actionline.parameters != null && actionline.parameters.Length >= 3)
                    decideDataGridViewComboBoxValue(paraValue1, actionline.parameters[2]);
                row.Cells.Add(paraValue1);
                this.dataGridViewActionProperty.Rows.Add(row);
                // To show Key-Value pair after "orientation" for OPCODE_TOUCH_DOWN
                if (actionline.parameters != null && actionline.parameters.Length >= 4)
                {
                    string kvParameter = actionline.parameters[3];
                    if (kvParameter.Contains('&') && kvParameter.Contains('='))
                    {
                        string[] kvPairs = kvParameter.Split('&');
                        foreach (string kvPair in kvPairs)
                        {
                            row = new DataGridViewRow();
                            paraName = new DataGridViewTextBoxCell();
                            paraName.Value = kvPair.Split('=')[0];
                            row.Cells.Add(paraName);
                            paraValue = new DataGridViewTextBoxCell();
                            paraValue.Value = kvPair.Split('=')[1];
                            row.Cells.Add(paraValue);
                            this.dataGridViewActionProperty.Rows.Add(row);
                        }
                    }
                }
            }
            else if (actionline.name == "OPCODE_TOUCHDOWN_HORIZONTAL" ||
                     actionline.name == "OPCODE_TOUCHUP_HORIZONTAL" ||
                     actionline.name == "OPCODE_TOUCHMOVE_HORIZONTAL")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Coordinate X";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 2)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Coordinate Y";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_SWIPE" || actionline.name == "OPCODE_DRAG")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "From Coordinate X";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 4)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "From Coordinate Y";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 4)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "To Coordinate X";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 4)
                    paraValue.Value = actionline.parameters[2];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "To Coordinate Y";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 4)
                    paraValue.Value = actionline.parameters[3];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_OCR")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Rule";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                /* OCR can not only be these values, it can be any value 
                DataGridViewComboBoxCell paraValue = new DataGridViewComboBoxCell();
                paraValue.Items.Add("1");
                paraValue.Items.Add("2");
                paraValue.Items.Add("4");
                paraValue.Items.Add("8");
                paraValue.Items.Add("12");
                */
                if (actionline.parameters != null && actionline.parameters.Length == 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_PREVIEW")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Preview Image";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Reference Image";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Label1";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 3)
                    paraValue.Value = actionline.parameters[2];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Label2";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 4)
                    paraValue.Value = actionline.parameters[3];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_TILTUP" ||
                    actionline.name == "OPCODE_TILTDOWN" ||
                    actionline.name == "OPCODE_TILTLEFT" ||
                    actionline.name == "OPCODE_TILTRIGHT")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Degree";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "[Unused Parameter]";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_TILTTO")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Arm0 Degree";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 2)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Arm1 Degree";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_HOLDER_UP" || actionline.name == "OPCODE_HOLDER_DOWN")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Distance";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "[Unused Parameter]";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_USB_IN" || actionline.name == "OPCODE_USB_OUT")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "USB Port Number";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "[Unused Parameter]";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_IF_MATCH_AUDIO")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Audio File1";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 4)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Audio File2";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 4)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Label1";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 4)
                    paraValue.Value = actionline.parameters[2];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Label2";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 4)
                    paraValue.Value = actionline.parameters[3];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_IF_MATCH_IMAGE")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Image Path";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = TestUtil.ImageObject.Parse(actionline.parameters[0]).ImageName;
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Angle";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = TestUtil.ImageObject.Parse(actionline.parameters[0]).Angle;
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Angle Error";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = TestUtil.ImageObject.Parse(actionline.parameters[0]).AngleError;
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Reference Match Ratio";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = TestUtil.ImageObject.Parse(actionline.parameters[0]).RatioReferenceMatch;
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Label1";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Label2";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 3)
                    paraValue.Value = actionline.parameters[2];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "[Algorithm]";
                row.Cells.Add(paraName);
                DataGridViewComboBoxCell paraValue1 = new DataGridViewComboBoxCell();
                paraValue1.Items.Add("0");
                paraValue1.Items.Add("1");
                if (actionline.parameters != null && actionline.parameters.Length >= 4)
                {
                    decideDataGridViewComboBoxValue(paraValue1, actionline.parameters[3]);
                }
                row.Cells.Add(paraValue1);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_MATCH_ON")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "[Count Tolerance]";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_IF_SOURCE_IMAGE_MATCH")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Source Image";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Target Image";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Label1";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 3)
                    paraValue.Value = actionline.parameters[2];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Label2";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 4)
                    paraValue.Value = actionline.parameters[3];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "[Algorithm]";
                row.Cells.Add(paraName);
                DataGridViewComboBoxCell paraValue1 = new DataGridViewComboBoxCell();
                paraValue1.Items.Add("0");
                paraValue1.Items.Add("1");
                if (actionline.parameters != null && actionline.parameters.Length >= 5)
                {
                    decideDataGridViewComboBoxValue(paraValue1, actionline.parameters[4]);
                }
                row.Cells.Add(paraValue1);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_IF_SOURCE_LAYOUT_MATCH")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Source Layout";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Target Layout";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Label1";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 3)
                    paraValue.Value = actionline.parameters[2];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Label2";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 4)
                    paraValue.Value = actionline.parameters[3];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_IF_MATCH_IMAGE_WAIT")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Image Path";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = TestUtil.ImageObject.Parse(actionline.parameters[0]).ImageName;
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Angle";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = TestUtil.ImageObject.Parse(actionline.parameters[0]).Angle;
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Angle Error";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = TestUtil.ImageObject.Parse(actionline.parameters[0]).AngleError;
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Reference Match Ratio";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = TestUtil.ImageObject.Parse(actionline.parameters[0]).RatioReferenceMatch;
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Label1";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Label2";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 3)
                    paraValue.Value = actionline.parameters[2];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Timeout";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 4)
                    paraValue.Value = actionline.parameters[3];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "[Algorithm]";
                row.Cells.Add(paraName);
                DataGridViewComboBoxCell paraValue1 = new DataGridViewComboBoxCell();
                paraValue1.Items.Add("0");
                paraValue1.Items.Add("1");
                if (actionline.parameters != null && actionline.parameters.Length >= 5)
                {
                    decideDataGridViewComboBoxValue(paraValue1, actionline.parameters[4]);
                }
                row.Cells.Add(paraValue1);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_IMAGE_CHECK")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Image Path";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = TestUtil.ImageObject.Parse(actionline.parameters[0]).ImageName;
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Reference Match Ratio";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = TestUtil.ImageObject.Parse(actionline.parameters[0]).RatioReferenceMatch;
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Label";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Timeout";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 3)
                    paraValue.Value = actionline.parameters[2];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_FPS_MEASURE_START" || actionline.name == "OPCODE_FPS_MEASURE_STOP")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "[Image Path]";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_CLICK_MATCHED_IMAGE_XY" ||
                actionline.name == "OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY" ||
                actionline.name == "OPCODE_TOUCHUP_MATCHED_IMAGE_XY" ||
                actionline.name == "OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Image Path";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Offset X";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Offset Y";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 3)
                    paraValue.Value = actionline.parameters[2];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Coordinate X";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 4)
                    paraValue.Value = actionline.parameters[3];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Coordinate Y";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 5)
                    paraValue.Value = actionline.parameters[4];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "[Orientation]";
                row.Cells.Add(paraName);
                DataGridViewComboBoxCell paraValue1 = new DataGridViewComboBoxCell();
                paraValue1.Items.Add("0");
                paraValue1.Items.Add("1");
                paraValue1.Items.Add("2");
                paraValue1.Items.Add("3");
                if (actionline.parameters != null && actionline.parameters.Length >= 6)
                    decideDataGridViewComboBoxValue(paraValue1, actionline.parameters[5]);
                row.Cells.Add(paraValue1);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_CLICK_MATCHED_IMAGE")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Image Path";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Offset X";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Offset Y";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 3)
                    paraValue.Value = actionline.parameters[2];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "[Orientation]";
                row.Cells.Add(paraName);
                DataGridViewComboBoxCell paraValue1 = new DataGridViewComboBoxCell();
                paraValue1.Items.Add("0");
                paraValue1.Items.Add("1");
                paraValue1.Items.Add("2");
                paraValue1.Items.Add("3");
                if (actionline.parameters != null && actionline.parameters.Length >= 4)
                    decideDataGridViewComboBoxValue(paraValue1, actionline.parameters[3]);
                row.Cells.Add(paraValue1);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_AUDIO_PLAY")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Audio File";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_AUDIO_RECORD_START")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Audio File";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_AUDIO_RECORD_STOP")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Audio File";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_IMAGE_MATCHED_TIME")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Image Path";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_GOTO")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Label";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_AI_BUILTIN")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Xml File Path";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_NEXT_SCRIPT" ||
                     actionline.name == "OPCODE_CALL_SCRIPT")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "QS FileName";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_CALL_EXTERNAL_SCRIPT")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Command/FileName";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_BATCH_PROCESSING")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Ini FileName";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_MULTI_TOUCHDOWN" ||
                actionline.name == "OPCODE_MULTI_TOUCHUP" ||
                actionline.name == "OPCODE_MULTI_TOUCHMOVE")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Coordinate X";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Coordinate Y";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Pointer";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 3)
                    paraValue.Value = actionline.parameters[2];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "[Orientation]";
                row.Cells.Add(paraName);
                DataGridViewComboBoxCell paraValue1 = new DataGridViewComboBoxCell();
                paraValue1.Items.Add("0");
                paraValue1.Items.Add("1");
                paraValue1.Items.Add("2");
                paraValue1.Items.Add("3");
                if (actionline.parameters != null && actionline.parameters.Length >= 4)
                    decideDataGridViewComboBoxValue(paraValue1, actionline.parameters[3]);
                row.Cells.Add(paraValue1);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_MULTI_TOUCHDOWN_HORIZONTAL" ||
                     actionline.name == "OPCODE_MULTI_TOUCHUP_HORIZONTAL" ||
                     actionline.name == "OPCODE_MULTI_TOUCHMOVE_HORIZONTAL")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Coordinate X";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 3)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Coordinate Y";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 3)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Pointer";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length == 3)
                    paraValue.Value = actionline.parameters[2];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_CONCURRENT_IF_MATCH")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Image Path";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = TestUtil.ImageObject.Parse(actionline.parameters[0]).ImageName;
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Angle";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = TestUtil.ImageObject.Parse(actionline.parameters[0]).Angle;
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Angle Error";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = TestUtil.ImageObject.Parse(actionline.parameters[0]).AngleError;
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Instr Count";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "[Match Once?]";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 3)
                    paraValue.Value = actionline.parameters[2];
                row.Cells.Add(paraValue);
                /*
                DataGridViewComboBoxCell paraComboValue = new DataGridViewComboBoxCell();
                paraComboValue.Items.Add("1");
                if (actionline.parameters != null && actionline.parameters.Length >= 3)
                    paraComboValue.Value = actionline.parameters[2];
                row.Cells.Add(paraComboValue);
                 */
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_IF_MATCH_TXT")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Text";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Ocr_para";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Label1";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 3)
                    paraValue.Value = actionline.parameters[2];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Label2";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 4)
                    paraValue.Value = actionline.parameters[3];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_IF_MATCH_REGEX")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Pattern";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Ocr_para";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Label1";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 3)
                    paraValue.Value = actionline.parameters[2];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Label2";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 4)
                    paraValue.Value = actionline.parameters[3];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_KEYBOARD_EVENT")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Type";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Key Code";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_SET_VARIABLE")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Variable name";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Value";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_LOOP")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Variable name";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Target Label";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_MESSAGE")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Message";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_ERROR")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Message";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_MOUSE_EVENT")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Type";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "CoordinateX";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "CoordinateY";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 3)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Delta";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 4)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_CLICK_MATCHED_TXT")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Text";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Num matched";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "X Offset";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 3)
                    paraValue.Value = actionline.parameters[2];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Y Offset";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 4)
                    paraValue.Value = actionline.parameters[3];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "[Orientation]";
                row.Cells.Add(paraName);
                DataGridViewComboBoxCell paraValue1 = new DataGridViewComboBoxCell();
                paraValue1.Items.Add("0");
                paraValue1.Items.Add("1");
                paraValue1.Items.Add("2");
                paraValue1.Items.Add("3");
                if (actionline.parameters != null && actionline.parameters.Length >= 5)
                    decideDataGridViewComboBoxValue(paraValue1, actionline.parameters[4]);
                row.Cells.Add(paraValue1);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_CLICK_MATCHED_REGEX")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Pattern";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Num matched";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue.Value = actionline.parameters[1];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "X Offset";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 3)
                    paraValue.Value = actionline.parameters[2];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Y Offset";
                row.Cells.Add(paraName);
                paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 4)
                    paraValue.Value = actionline.parameters[3];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "[Orientation]";
                row.Cells.Add(paraName);
                DataGridViewComboBoxCell paraValue1 = new DataGridViewComboBoxCell();
                paraValue1.Items.Add("0");
                paraValue1.Items.Add("1");
                paraValue1.Items.Add("2");
                paraValue1.Items.Add("3");
                if (actionline.parameters != null && actionline.parameters.Length >= 5)
                    decideDataGridViewComboBoxValue(paraValue1, actionline.parameters[4]);
                row.Cells.Add(paraValue1);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_VOLUME_DOWN" || actionline.name == "OPCODE_VOLUME_UP")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Action";
                row.Cells.Add(paraName);
                DataGridViewComboBoxCell paraValue = new DataGridViewComboBoxCell();
                paraValue.Items.Add("0");
                paraValue.Items.Add("1");
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_INSTALL_APP")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Install to SD Cards";
                row.Cells.Add(paraName);
                DataGridViewComboBoxCell paraValue = new DataGridViewComboBoxCell();
                paraValue.Items.Add("0");
                paraValue.Items.Add("1");
                // Set the default value of installation location
                paraValue.Value = "0";
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                    paraValue.Value = actionline.parameters[0];
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                paraName = new DataGridViewTextBoxCell();
                paraName.Value = "APK Name";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue1 = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                    paraValue1.Value = actionline.parameters[1];
                row.Cells.Add(paraValue1);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_START_APP")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "[Full Activity Name]";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                {
                    paraValue.Value = actionline.parameters[0];
                }
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_STOP_APP" || actionline.name == "OPCODE_UNINSTALL_APP" || actionline.name == "OPCODE_CLEAR_DATA")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Package Name";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                {
                    paraValue.Value = actionline.parameters[0];
                }
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_PUSH_DATA")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraNameSource = new DataGridViewTextBoxCell();
                paraNameSource.Value = "Source Path";
                row.Cells.Add(paraNameSource);
                DataGridViewTextBoxCell paraValueSrc = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                {
                    paraValueSrc.Value = actionline.parameters[0];
                }
                row.Cells.Add(paraValueSrc);
                this.dataGridViewActionProperty.Rows.Add(row);

                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraNameTarget = new DataGridViewTextBoxCell();
                paraNameTarget.Value = "Target Path";
                row.Cells.Add(paraNameTarget);
                DataGridViewTextBoxCell paraValueTgt = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 2)
                {
                    paraValueTgt.Value = actionline.parameters[1];
                }
                row.Cells.Add(paraValueTgt);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else if (actionline.name == "OPCODE_DUMP_UI_LAYOUT")
            {
                row = new DataGridViewRow();
                DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                paraName.Value = "Dump file name";
                row.Cells.Add(paraName);
                DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                if (actionline.parameters != null && actionline.parameters.Length >= 1)
                {
                    paraValue.Value = actionline.parameters[0];
                }
                row.Cells.Add(paraValue);
                this.dataGridViewActionProperty.Rows.Add(row);
            }
            else
            {
                if (actionline.parameters != null)
                {
                    for (int i = 0; i < actionline.parameters.Length; i++)
                    {
                        row = new DataGridViewRow();
                        DataGridViewTextBoxCell paraName = new DataGridViewTextBoxCell();
                        paraName.Value = "Parameter" + (i + 1).ToString();
                        row.Cells.Add(paraName);
                        DataGridViewTextBoxCell paraValue = new DataGridViewTextBoxCell();
                        paraValue.Value = actionline.parameters[i];
                        row.Cells.Add(paraValue);
                        this.dataGridViewActionProperty.Rows.Add(row);
                    }
                }
            }
        }

        private void listBoxActions_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (this.listBoxActions.SelectedIndex < this.actions.Count && this.listBoxActions.SelectedIndex >= 0)
            {
                this.comboBoxAction.Text = this.actions[this.listBoxActions.SelectedIndex].name;
                actionlinetype actionline = this.actions[this.listBoxActions.SelectedIndex];
                flashDataGridViewActionProperty(actionline);
                toolStripButtonPrevStep.Enabled = true;
                toolStripButtonNextStep.Enabled = true;
            }
            else
            {
                flashDataGridViewActionProperty(new actionlinetype());
                toolStripButtonPrevStep.Enabled = false;
                toolStripButtonNextStep.Enabled = false;
            }

            isEditingPictureMode = false;
        }

        // Check whether a label is already used in the current scripts.
        private bool isLabelInScript(int l)
        {
            for (int i = 0; i < this.actions.Count; i++)
                if (this.actions[i].label == l)
                    return true;
            return false;
        }

        // Get the smallest unused label
        private int getUnusedLabel()
        {
            if (this.actions == null)
                return 0;
            int label = 1;
            while (isLabelInScript(label) || isLabelInQTS(label))
                label++;
            return label;
        }

        private bool isLabelInQTS(int l)
        {
            if (this.qs_trace == null)
                return false;
            for (int i = 0; i < this.qs_trace.Count; i++)
                if (this.qs_trace[i].label == l)
                    return true;
            return false;
        }

        private void comboBoxAction_TextChanged(object sender, EventArgs e)
        {
            actionlinetype actionline = new actionlinetype(this.comboBoxAction.Text);
            actionline.label = getUnusedLabel();
            if (this.actions != null && this.listBoxActions.SelectedIndex >= 0)
                if (this.listBoxActions.SelectedIndex < this.actions.Count)
                    actionline.time = this.actions[this.listBoxActions.SelectedIndex].time;
                else
                    actionline.time = this.actions[this.actions.Count - 1].time;
            flashDataGridViewActionProperty(actionline);

            if (actionline.name == "OPCODE_HOME")
            {
                this.textBoxActionInfo.Text = "OPCODE_HOME: no operand, just send an HOME action to device.";
            }
            else if (actionline.name == "OPCODE_MENU")
            {
                this.textBoxActionInfo.Text = "OPCODE_MENU: no operand, just send an MENU action to device.";
            }
            else if (actionline.name == "OPCODE_BACK")
            {
                this.textBoxActionInfo.Text = "OPCODE_BACK: no operand, just send an BACK action to device.";
            }
            else if (actionline.name == "OPCODE_SET_CLICKING_HORIZONTAL")
            {
                this.textBoxActionInfo.Text = "OPCODE_SET_CLICKING_HORIZONTAL: set entering horizontal mode, no operand.";
            }
            else if (actionline.name == "OPCODE_PREVIEW")
            {
                this.textBoxActionInfo.Text = "OPCODE_PREVIEW:  <preview_image_file> <reference_image_file> <label1> <label2>: \r\n check if <source_image_file> matches the <target_image_file>. If yes, go to <label1>; if not or the image doesn't exist go to <label2>.";
            }
            else if (actionline.name == "OPCODE_MATCH_ON")
            {
                this.textBoxActionInfo.Text = "OPCODE_MATCH_ON: turn on matching in result checker, and then the image in playing video will be checked against the training video.";
            }
            else if (actionline.name == "OPCODE_MATCH_OFF")
            {
                this.textBoxActionInfo.Text = "OPCODE_MATCH_OFF: turn off matching in result checker, and then the image will not be checked until it is turned on again.";
            }
            else if (actionline.name == "OPCODE_FLICK_ON")
            {
                this.textBoxActionInfo.Text = "OPCODE_FLICK_ON: Change the flick detection state to on.";
            }
            else if (actionline.name == "OPCODE_FLICK_OFF")
            {
                this.textBoxActionInfo.Text = "OPCODE_FLICK_OFF: Change the flick detection state to off.";
            }
            else if (actionline.name == "OPCODE_CRASH_ON")
            {
                this.textBoxActionInfo.Text = "OPCODE_CRASH_ON:enable app crash detection via matching template crash dialog.";
            }
            else if (actionline.name == "OPCODE_CRASH_OFF")
            {
                this.textBoxActionInfo.Text = "OPCODE_CRASH_OFF:disable app crash detection (default).";
            }
            else if (actionline.name == "OPCODE_EXIT")
            {
                this.textBoxActionInfo.Text = "OPCODE_EXIT: exit the script directly.";
            }
            else if (actionline.name == "OPCODE_DEVICE_POWER")
                this.textBoxActionInfo.Text = "OPCODE_DEVICE_POWER: Operate devices power button.\r\nPress button: OPCODE_DEVICE_POWER 0. \r\nRelease button: OPCODE_DEVICE_POWER 1.";
            else if (actionline.name == "OPCODE_CLICK")
                this.textBoxActionInfo.Text = "OPCODE_CLICK: click on one point (X, Y).";
            else if (actionline.name == "OPCODE_TOUCHDOWN")
                this.textBoxActionInfo.Text = "OPCODE_TOUCHDOWN: touch down action with two operands (X, Y).";
            else if (actionline.name == "OPCODE_TOUCHDOWN_HORIZONTAL")
                this.textBoxActionInfo.Text = "OPCODE_TOUCHDOWN_HORIZONTAL: touch down action in horizontal mode with two operands (X, Y).";
            else if (actionline.name == "OPCODE_TOUCHUP")
                this.textBoxActionInfo.Text = "OPCODE_TOUCHUP: touch up action with two operands (X, Y).";
            else if (actionline.name == "OPCODE_TOUCHUP_HORIZONTAL")
                this.textBoxActionInfo.Text = "OPCODE_TOUCHUP_HORIZONTAL: touch up action in horizontal mode with two operands (X, Y).";
            else if (actionline.name == "OPCODE_TOUCHMOVE")
                this.textBoxActionInfo.Text = "OPCODE_TOUCHMOVE: touch move action with two operands (X, Y).";
            else if (actionline.name == "OPCODE_TOUCHMOVE_HORIZONTAL")
                this.textBoxActionInfo.Text = "OPCODE_TOUCHMOVE_HORIZONTAL: touch move action in horizontal mode with two operands (X, Y).";
            else if (actionline.name == "OPCODE_START_APP")
                this.textBoxActionInfo.Text = "OPCODE_START_APP: start the application.";
            else if (actionline.name == "OPCODE_STOP_APP")
                this.textBoxActionInfo.Text = "OPCODE_STOP_APP: stop the application.";
            else if (actionline.name == "OPCODE_INSTALL_APP")
                this.textBoxActionInfo.Text = "OPCODE_INSTALL_APP: install application.";
            else if (actionline.name == "OPCODE_UNINSTALL_APP")
                this.textBoxActionInfo.Text = "OPCODE_UNINSTALL_APP: uninstall application.";
            else if (actionline.name == "OPCODE_SWIPE")
                this.textBoxActionInfo.Text = "OPCODE_SWIPE: swipe action with four operands - source (X1, Y1) and destiny (X2, Y2).";
            else if (actionline.name == "OPCODE_DRAG")
                this.textBoxActionInfo.Text = "OPCODE_DRAG: drag action with four operands - source position (X1, Y1) and destiny (X2, Y2)";
            else if (actionline.name == "OPCODE_OCR")
            {
                this.textBoxActionInfo.Text = "OPCODE_OCR: recognize image as text and return the text, using only operand1 as the rule below: \r\n1 - PARAM_THICK_FONT; \r\n2 - PARAM_SMALL_FONT; \r\n4 - PARAM_ROI_UPPER; \r\n8 - PARAM_ROI_MIDDLE; \r\n12- PARAM_ROI_LOWER;";
            }
            else if (actionline.name == "OPCODE_TILTUP")
                this.textBoxActionInfo.Text = "OPCODE_TILTUP: tilt the plate upward for a defined degree as in operand1 (operand2 is written while not used).";
            else if (actionline.name == "OPCODE_TILTDOWN")
                this.textBoxActionInfo.Text = "OPCODE_TILTDOWN: tilt the plate downward for a defined degree as in operand1 (operand2 is written while not used).";
            else if (actionline.name == "OPCODE_TILTLEFT")
                this.textBoxActionInfo.Text = "OPCODE_TILTLEFT: tilt the plate to the left for a defined degree as in operand1 (operand2 is written while not used).";
            else if (actionline.name == "OPCODE_TILTRIGHT")
                this.textBoxActionInfo.Text = "OPCODE_TILTRIGHT: tilt the plate to the right for a defined degree as in operand1 (operand2 is written while not used).";
            else if (actionline.name == "OPCODE_TILTTO")
            {
                this.textBoxActionInfo.Text = "OPCODE_TILTTO: tilt the plate to a specified arm0/arm1 degree as in operand1 and operand2.";
            }
            else if (actionline.name == "OPCODE_HOLDER_UP")
            {
                this.textBoxActionInfo.Text = "OPCODE_HOLDER_UP: lift the holder upward for a defined distance as in operand1 (operand2 is written while not used).";
            }
            else if (actionline.name == "OPCODE_HOLDER_DOWN")
            {
                this.textBoxActionInfo.Text = "OPCODE_HOLDER_DOWN: lift the holder downward for a defined distance as in operand1 (operand2 is written while not used).";
            }
            else if (actionline.name == "OPCODE_USB_IN")
            {
                this.textBoxActionInfo.Text = "OPCODE_USB_IN: plug in the USB port defined as in operand1 (operand2 is written while not used).";
            }
            else if (actionline.name == "OPCODE_USB_OUT")
            {
                this.textBoxActionInfo.Text = "OPCODE_USB_OUT: plug out the USB port defined as in operand1 (operand2 is written while not used).";
            }
            else if (actionline.name == "OPCODE_POWER_MEASURE_START")
            {
                this.textBoxActionInfo.Text = "OPCODE_POWER_MEASURE_START: Start measuring device power consumption.";
            }
            else if (actionline.name == "OPCODE_POWER_MEASURE_STOT")
            {
                this.textBoxActionInfo.Text = "OPCODE_POWER_MEASURE_STOT: Stot measuring device power consumption.";
            }
            else if (actionline.name == "OPCODE_IF_MATCH_AUDIO")
            {
                this.textBoxActionInfo.Text = "OPCODE_IF_MATCH_AUDIO: audio_file1/audio_file2/label1/label2 \r\n check if audio_file1 matches audio_file2. If yes, go to label1; otherwise go to label2.";
            }
            else if (actionline.name == "OPCODE_IF_MATCH_IMAGE")
            {
                this.textBoxActionInfo.Text = "OPCODE_IF_MATCH_IMAGE: image_path[|angle,[angle error]]/label1/label2/algorithm \r\n check if the current frame matches the image provided by image_path given an optional angle in degree. If yes, go to label1; if not or the image doesn't exist go to label2. \r\n During runtime, the param label1 and label2 are resolved to ip1 and ip2.";
            }
            else if (actionline.name == "OPCODE_IF_SOURCE_IMAGE_MATCH")
            {
                this.textBoxActionInfo.Text = "OPCODE_IF_SOURCE_IMAGE_MATCH <source_image_file> <target_image_file> <label1> <label2> <algorithm>: \r\n check if <source_image_file> matches the <target_image_file>. If yes, go to <label1>; if not or the image doesn't exist go to <label2>.";
            }
            else if (actionline.name == "OPCODE_IF_SOURCE_LAYOUT_MATCH")
            {
                this.textBoxActionInfo.Text = "OPCODE_IF_SOURCE_LAYOUT_MATCH <source_layout_file> <target_layout_file> <label1> <label2>: \r\n check if <source_layout_file> matches the <target_layout_file>. If yes, go to <label1>; if not, go to <label2>.";
            }
            else if (actionline.name == "OPCODE_IF_MATCH_IMAGE_WAIT")
            {
                this.textBoxActionInfo.Text = "OPCODE_IF_MATCH_IMAGE_WAIT: image_path[|angle,[angle error]]/label1/label2/timeout/algorithm \r\n wait until the current frame matches the image provided by image_path. If yes, go to label1; if timeout go to label2.";
            }
            else if (actionline.name == "OPCODE_IMAGE_CHECK")
            {
                this.textBoxActionInfo.Text = "OPCODE_IMAGE_CHECK: reference_image_path?referenceMatchRaio=<value> <label> <timeout>\r\n Check if reference image matches current frame until matches or timeout happpens, if not matched go to label";
            }
            else if (actionline.name == "OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY")
                this.textBoxActionInfo.Text = "OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY: image_path/offX/offY/X/Y \r\n If there is a matched image, click it with offset; if not or the image doesn't exist, click coordinate (X,Y).";
            else if (actionline.name == "OPCODE_TOUCHUP_MATCHED_IMAGE_XY")
                this.textBoxActionInfo.Text = "OPCODE_TOUCHUP_MATCHED_IMAGE_XY: image_path/offX/offY/X/Y \r\n If there is a matched image, click it with offset; if not or the image doesn't exist, click coordinate (X,Y).";
            else if (actionline.name == "OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY")
                this.textBoxActionInfo.Text = "OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY: image_path/offX/offY/X/Y \r\n If there is a matched image, click it with offset; if not or the image doesn't exist, click coordinate (X,Y).";
            else if (actionline.name == "OPCODE_CLICK_MATCHED_IMAGE_XY")
                this.textBoxActionInfo.Text = "OPCODE_CLICK_MATCHED_IMAGE_XY: image_path/offX/offY/X/Y \r\n If there is a matched image, click it with offset; if not or the image doesn't exist, click coordinate (X,Y).";
            else if (actionline.name == "OPCODE_CLICK_MATCHED_IMAGE")
            {
                this.textBoxActionInfo.Text = "OPCODE_CLICK_MATCHED_IMAGE: image_path \r\n Click on the image matched in the last OPCODE_IF_MATCH_IMAGE call with the same image_path.";
            }
            else if (actionline.name == "OPCODE_AUDIO_PLAY")
            {
                this.textBoxActionInfo.Text = "OPCODE_AUDIO_PLAY: audio_file \r\n Play the specified audio file audio_file.";
            }
            else if (actionline.name == "OPCODE_AUDIO_RECORD_START")
            {
                this.textBoxActionInfo.Text = "OPCODE_AUDIO_RECORD_START: audio_file \r\n Start recording audio into the specified audio file audio_file.";
            }
            else if (actionline.name == "OPCODE_AUDIO_RECORD_STOP")
            {
                this.textBoxActionInfo.Text = "OPCODE_AUDIO_RECORD_STOP: audio_file \r\n Stop recording audio and close the specified audio file audio_file.";
            }
            else if (actionline.name == "OPCODE_IMAGE_MATCHED_TIME")
            {
                this.textBoxActionInfo.Text = "OPCODE_IMAGE_MATCHED_TIME: image_path \r\n Report the elapsed time of the latest matching of the provided image file.";
            }
            else if (actionline.name == "OPCODE_GOTO")
            {
                this.textBoxActionInfo.Text = "OPCODE_GOTO: label \r\n Unconditional jump.";
            }
            else if (actionline.name == "OPCODE_PUSH_DATA")
            {
                this.textBoxActionInfo.Text = "OPCODE_PUSH_DATA: push data for application.";
            }
            else if (actionline.name == "OPCODE_CLEAR_DATA")
            {
                this.textBoxActionInfo.Text = "OPCODE_CLEAR_DATA: clear data for application.";
            }
            else if (actionline.name == "OPCODE_AI_BUILTIN")
            {
                this.textBoxActionInfo.Text = "OPCODE_AI_BUILTIN: run built-in AI. It has one parameter which is the xml file path.";
            }
            else if (actionline.name == "OPCODE_NEXT_SCRIPT")
                this.textBoxActionInfo.Text = "OPCODE_NEXT_SCRIPT: jump to the next script.";
            else if (actionline.name == "OPCODE_CALL_SCRIPT")
                this.textBoxActionInfo.Text = "OPCODE_CALL_SCRIPT: call an internal script.";
            else if (actionline.name == "OPCODE_MESSAGE")
            {
                this.textBoxActionInfo.Text = "OPCODE_MESSAGE: allow the user to output some words on the screen or message box.";
            }
            else if (actionline.name == "OPCODE_ERROR")
            {
                this.textBoxActionInfo.Text = "OPCODE_ERROR: throw an error message and record a user-defined error.";
            }
            else if (actionline.name == "OPCODE_MULTI_TOUCHDOWN")
                this.textBoxActionInfo.Text = "OPCODE_MULTI_TOUCHDOWN: touch down action with three operands (X, Y, ptr) for multi-touch.";
            else if (actionline.name == "OPCODE_MULTI_TOUCHDOWN_HORIZONTAL")
                this.textBoxActionInfo.Text = "OPCODE_MULTI_TOUCHDOWN_HORIZONTAL: touch down action in horizontal mode with three operands (X, Y, ptr) for multi-touch.";
            else if (actionline.name == "OPCODE_MULTI_TOUCHUP")
                this.textBoxActionInfo.Text = "OPCODE_MULTI_TOUCHUP: touch up action with three operands (X, Y, ptr) for multi-touch.";
            else if (actionline.name == "OPCODE_MULTI_TOUCHUP_HORIZONTAL")
                this.textBoxActionInfo.Text = "OPCODE_MULTI_TOUCHUP_HORIZONTAL: touch up action in horizontal mode with three operands (X, Y, ptr) for multi-touch.";
            else if (actionline.name == "OPCODE_MULTI_TOUCHMOVE")
                this.textBoxActionInfo.Text = "OPCODE_MULTI_TOUCHMOVE: touch move action with three operands (X, Y, ptr) for multi-touch.";
            else if (actionline.name == "OPCODE_MULTI_TOUCHMOVE_HORIZONTAL")
                this.textBoxActionInfo.Text = "OPCODE_MULTI_TOUCHMOVE_HORIZONTAL: touch move action in horizontal mode with three operands (X, Y, ptr) for multi-touch.";
            else if (actionline.name == "OPCODE_CALL_EXTERNAL_SCRIPT")
            {
                this.textBoxActionInfo.Text = "OPCODE_CALL_EXTERNAL_SCRIPT: call external script";
            }
            else if (actionline.name == "OPCODE_ELAPSED_TIME")
            {
                this.textBoxActionInfo.Text = "OPCODE_ELAPSED_TIME: no operand, output elapsed time in milliseconds.";
            }
            else if (actionline.name == "OPCODE_FPS_MEASURE_START")
            {
                this.textBoxActionInfo.Text = "OPCODE_FPS_MEASURE_START: [start.png]: start FPS measurement if start.png is matched, or the timestamp >= start_timeStamp.";
            }
            else if (actionline.name == "OPCODE_FPS_MEASURE_STOP")
            {
                this.textBoxActionInfo.Text = "OPCODE_FPS_MEASURE_STOP: [stop.png]: stop FPS measurement if the stop.png is matched, or the timestamp >= stop_timeStamp.";
            }
            else if (actionline.name == "OPCODE_CONCURRENT_IF_MATCH")
            {
                this.textBoxActionInfo.Text = "OPCODE_CONCURRENT_IF_MATCH: <image_file_name> <instr_count> [match_once?]. \r\n It's used for handling dynamic scenarios, like random Ads. It compare every frames and do specified actions if matching the image. The actions start from next instruction and its block length is specified by instr_count. \r\n operand string: image file name to be compared. \r\n operand 1: instruction count for handling dynamic scenarios.\r\n [match_once]: optional. if 1, it means it will only match once if it can be matched.";
            }
            else if (actionline.name == "OPCODE_IF_MATCH_REGEX")
            {
                this.textBoxActionInfo.Text = "OPCODE_IF_MATCH_REGEX: pattern ocr_param line1 line2. \r\n Similar to OPCODE_IF_MATCH_TXT but matching a regular expression pattern instead of a string.";
            }
            else if (actionline.name == "OPCODE_IF_MATCH_TXT")
            {
                this.textBoxActionInfo.Text = "OPCODE_IF_MATCH_TXT <string> <label1> <label2>: similar to OPCODE_IF_MATCH_IMAGE but matches text.";
            }
            else if (actionline.name == "OPCODE_KEYBOARD_EVENT")
            {
                this.textBoxActionInfo.Text = "OPCODE_KEYBOARD_EVENT <type> <keycode>: Keyboard event.";
            }
            else if (actionline.name == "OPCODE_MOUSE_EVENT")
            {
                this.textBoxActionInfo.Text = "OPCODE_MOUSE_EVENT: <type> <Normalized X> <Normalized Y> <Delta>: mouse events.";
            }
            else if (actionline.name == "OPCODE_SET_VARIABLE")
            {
                this.textBoxActionInfo.Text = "OPCODE_SET_VARIABLE <variable_name> <value>: Declare a variable and set its initial value. The <value> operand could be a constant or an expression.";
            }
            else if (actionline.name == "OPCODE_LOOP")
            {
                this.textBoxActionInfo.Text = "OPCODE_LOOP <variable_name> <target_label>: Loop to <target_label> until <variable_name> is less or equal to 0.";
            }
            else if (actionline.name == "OPCODE_CLICK_MATCHED_TXT")
            {
                this.textBoxActionInfo.Text = "OPCODE_CLICK_MATCHED_TXT: <Text>: similar to OPCODE_CLICK_MATCH_IMAGE but matches text.";
            }
            else if (actionline.name == "OPCODE_CLICK_MATCHED_REGEX")
            {
                this.textBoxActionInfo.Text = "OPCODE_CLICK_MATCHED_REGEX: pattern num_matched x_offset y_offset \r\n Similar to OPCODE_CLICK_MATCHED_TXT but click the previously matching regular expression with OPCODE_IF_MATCH_REGEX.";
            }
            else if (actionline.name == "OPCODE_BATCH_PROCESSING")
            {
                this.textBoxActionInfo.Text = "OPCODE_BATCH_PROCESSING: string_operand contains the name of an .ini file, which contains the batched commands to execute.";
            }
            else if (actionline.name == "OPCODE_AUDIO_MATCH_ON")
            {
                this.textBoxActionInfo.Text = "OPCODE_AUDIO_MATCH_ON: Turn on audio matching.";
            }
            else if (actionline.name == "OPCODE_AUDIO_MATCH_OFF")
            {
                this.textBoxActionInfo.Text = "OPCODE_AUDIO_MATCH_OFF: Turn off audio matching.";
            }
            else if (actionline.name == "OPCODE_VOLUME_DOWN")
            {
                this.textBoxActionInfo.Text = "OPCODE_VOLUME_DOWN: Press volume down button with mode 0 for \"UP\" and 1 for \"DOWN\" .";
            }
            else if (actionline.name == "OPCODE_VOLUME_UP")
            {
                this.textBoxActionInfo.Text = "OPCODE_VOLUME_UP: Press volume up button with mode 0 for \"UP\" and 1 for \"DOWN\".";
            }
            else if (actionline.name == "OPCODE_BRIGHTNESS")
            {
                this.textBoxActionInfo.Text = "OPCODE_BRIGHTNESS: Adjust brightness of target device with parameter 0 for brightness up and 1 for brightness down.";
            }
            else if (actionline.name == "OPCODE_SET_TEXT")
            {
                this.textBoxActionInfo.Text = "OPCODE_SET_TEXT: Set the text to the application field via agent";
            }
            else if (actionline.name == "OPCODE_DUMP_UI_LAYOUT")
            {
                this.textBoxActionInfo.Text = "OPCODE_DUMP_UI_LAYOUT: Dump the layout information of the device into a specified file.";
            }
            else
            {
                this.textBoxActionInfo.Text = "Unknown opcode: " + actionline.name + ". To be added to the QEditor.";
            }
        }

        private int getActionsIndexFromLabel(int label)
        {
            if (this.actions == null)
                return -1;
            for (int i = 0; i < this.actions.Count; i++)
            {
                if (this.actions[i].label == label)
                {
                    return i;
                }
            }
            return -1;
        }

        private void DrawCircle(Graphics g, Pen pen, float centerX, float centerY, float radius)
        {
            g.DrawEllipse(pen, centerX - radius, centerY - radius, radius + radius, radius + radius);
        }

        private void DrawLine(Graphics g, Pen pen, PointF startPos, PointF endPos)
        {
            g.DrawLine(pen, startPos, endPos);
        }

        /// <summary>
        /// Check whether the action at the specified index is vertical or not.
        /// </summary>
        /// <param name="selectedIndex"></param>
        /// <returns></returns>
        private DaVinciConfig.Orientation GetActionOrientation(int selectedIndex)
        {
            DaVinciConfig.Orientation orientation = DaVinciConfig.Orientation.Unknown;
            actionlinetype theAction = this.actions[selectedIndex];
            int orientationParamIdx = -1;
            switch (theAction.name)
            {
                case "OPCODE_TOUCHDOWN_HORIZONTAL":
                case "OPCODE_TOUCHUP_HORIZONTAL":
                case "OPCODE_TOUCHMOVE_HORIZONTAL":
                case "OPCODE_MULTI_TOUCHDOWN_HORIZONTAL":
                case "OPCODE_MULTI_TOUCHUP_HORIZONTAL":
                case "OPCODE_MULTI_TOUCHMOVE_HORIZONTAL":
                    orientation = DaVinciConfig.Orientation.Landscape;
                    break;
                case "OPCODE_CLICK":
                case "OPCODE_TOUCHDOWN":
                case "OPCODE_TOUCHUP":
                case "OPCODE_TOUCHMOVE":
                    // default to Portrait for these opcode
                    orientation = DaVinciConfig.Orientation.Portrait;
                    orientationParamIdx = 2;
                    break;
                case "OPCODE_CLICK_MATCHED_IMAGE":
                    orientationParamIdx = 3;
                    break;
                case "OPCODE_MULTI_TOUCHDOWN":
                case "OPCODE_MULTI_TOUCHUP":
                case "OPCODE_MULTI_TOUCHMOVE":
                    // default to Portrait for these opcode
                    orientation = DaVinciConfig.Orientation.Portrait;
                    orientationParamIdx = 3;
                    break;
                case "OPCODE_DRAG":
                    // default to Portrait for these opcode
                    orientation = DaVinciConfig.Orientation.Portrait;
                    orientationParamIdx = 4;
                    break;
                case "OPCODE_CLICK_MATCHED_TXT":
                case "OPCODE_CLICK_MATCHED_REGEX":
                    orientationParamIdx = 4;
                    break;
                case "OPCODE_CLICK_MATCHED_IMAGE_XY":
                case "OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY":
                case "OPCODE_TOUCHUP_MATCHED_IMAGE_XY":
                case "OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY":
                    orientationParamIdx = 5;
                    break;
            }

            if (0 <= orientationParamIdx && theAction.parameters != null && orientationParamIdx < theAction.parameters.Length)
            {
                try
                {
                    orientation = DaVinciConfig.IntergerToOrientation(int.Parse(theAction.parameters[orientationParamIdx]));
                }
                catch
                {
                    //TestManager.UpdateDebugMessage("GetActionOrientation exception!");
                }
            }
            if (orientation != DaVinciConfig.Orientation.Unknown)
            {
                return orientation;
            }
            else
            {
                orientation = DaVinciConfig.Orientation.Portrait;
                for (int index = selectedIndex - 1; index >= 0; --index)
                {
                    actionlinetype actionline = this.actions[index];
                    if (actionline.name == "OPCODE_SET_CLICKING_HORIZONTAL")
                    {
                        if (actionline.parameters[0] == "1")
                        {
                            orientation = DaVinciConfig.Orientation.Landscape;
                        }
                        break;
                    }
                }
                return orientation;
            }
        }

        private int frameIndexVal = 0;
        private void trackBar_ValueChanged(object sender, EventArgs e)
        {
            if (!File.Exists(videoFile))
                return;

            // change image frame
            try
            {
                frameIndexVal = this.trackBar.Value;
                DaVinciAPI.ImageEventHandler qScriptImageEventHandler = new DaVinciAPI.ImageEventHandler(DisplayImg);
                GCHandle gcImageEventHandler = GCHandle.Alloc(qScriptImageEventHandler);
                DaVinciAPI.NativeMethods.SetScriptVideoImgHandler(qScriptImageEventHandler);
                DaVinciAPI.NativeMethods.ShowVideoFrame(vidInst, this.trackBar.Value);

                if (this.actions != null && this.timeStampList != null && this.qs_trace != null)
                {
                    int timeIndex = this.trackBar.Value;
                    // Will also show the touch events right after this frame but before the next frame
                    if (timeIndex != this.trackBar.Maximum)
                        timeIndex++;

                    int traceIndex = getLastTraceNumberFromStamp(this.timeStampList[timeIndex]);
                    if (traceIndex >= 0)
                    {
                        int actionIndex = getActionsIndexFromLabel(this.qs_trace[traceIndex].label);
                        DaVinciConfig.Orientation actionOrientation;
                        if (actionIndex < 0)
                        {
                            //MessageBox.Show("Label " + this.qs_trace[traceIndex].label.ToString() + " is not found.");
                        }
                        else
                        {
                            actionOrientation = GetActionOrientation(actionIndex);
                            actionlinetype action = this.actions[actionIndex];
                            bool horizontalTouch = false;
                            if (action.name == "OPCODE_TOUCHDOWN" ||
                                action.name == "OPCODE_TOUCHUP" ||
                                action.name == "OPCODE_TOUCHMOVE" ||
                                action.name == "OPCODE_MULTI_TOUCHDOWN" ||
                                action.name == "OPCODE_MULTI_TOUCHUP" ||
                                action.name == "OPCODE_MULTI_TOUCHMOVE")
                            {
                                horizontalTouch = false;
                            }
                            else if (action.name == "OPCODE_TOUCHDOWN_HORIZONTAL" ||
                                action.name == "OPCODE_TOUCHUP_HORIZONTAL" ||
                                action.name == "OPCODE_TOUCHMOVE_HORIZONTAL" ||
                                action.name == "OPCODE_MULTI_TOUCHDOWN_HORIZONTAL" ||
                                action.name == "OPCODE_MULTI_TOUCHUP_HORIZONTAL" ||
                                action.name == "OPCODE_MULTI_TOUCHMOVE_HORIZONTAL")
                            {
                                horizontalTouch = true;
                            }
                            List<actionlinetype> operationList = new List<actionlinetype>();

                            var current = imageBox.Image;
                            Bitmap imgSave = DaVinciCommon.ConvertToBitmap(current.iplImage);
                            Graphics gImage = Graphics.FromImage(imgSave);

                            for (int i = traceIndex; i >= 0; i--)
                            {
                                actionIndex = getActionsIndexFromLabel(this.qs_trace[i].label);
                                if (actionIndex < 0)
                                {
                                    //MessageBox.Show("Label " + this.qs_trace[i].label.ToString() + " is not found.");
                                    //operationList.Clear();
                                    break;
                                }
                                action = this.actions[actionIndex];
                                if ((horizontalTouch &&
                                    (action.name == "OPCODE_TOUCHDOWN_HORIZONTAL" ||
                                    action.name == "OPCODE_TOUCHUP_HORIZONTAL" ||
                                    action.name == "OPCODE_TOUCHMOVE_HORIZONTAL" ||
                                    action.name == "OPCODE_MULTI_TOUCHDOWN_HORIZONTAL" ||
                                    action.name == "OPCODE_MULTI_TOUCHUP_HORIZONTAL" ||
                                    action.name == "OPCODE_MULTI_TOUCHMOVE_HORIZONTAL")) ||
                                    (!horizontalTouch &&
                                    (action.name == "OPCODE_TOUCHDOWN" ||
                                    action.name == "OPCODE_TOUCHUP" ||
                                    action.name == "OPCODE_TOUCHMOVE" ||
                                    action.name == "OPCODE_MULTI_TOUCHDOWN" ||
                                    action.name == "OPCODE_MULTI_TOUCHUP" ||
                                    action.name == "OPCODE_MULTI_TOUCHMOVE")))
                                {
                                    operationList.Add(action);
                                }
                                else
                                    break;
                            }

                            if (operationList != null && operationList.Count > 0)
                            {
                                operationList.Reverse();
                                List<actionlinetype>[] touches = new List<actionlinetype>[10];
                                int currentPointer = 0;
                                for (int i = 0; i < operationList.Count; i++)
                                {
                                    action = operationList[i];
                                    if (action.name == "OPCODE_TOUCHDOWN_HORIZONTAL" ||
                                        action.name == "OPCODE_TOUCHDOWN")
                                    {
                                    }
                                    else if (action.name == "OPCODE_MULTI_TOUCHDOWN_HORIZONTAL" ||
                                        action.name == "OPCODE_MULTI_TOUCHDOWN")
                                    {
                                        currentPointer = Int32.Parse(action.parameters[2]);
                                    }
                                    else if (action.name == "OPCODE_TOUCHMOVE_HORIZONTAL" ||
                                        action.name == "OPCODE_TOUCHMOVE")
                                    {
                                    }
                                    else if (action.name == "OPCODE_MULTI_TOUCHMOVE_HORIZONTAL" ||
                                        action.name == "OPCODE_MULTI_TOUCHMOVE")
                                    {
                                        currentPointer = Int32.Parse(action.parameters[2]);
                                    }
                                    else if (action.name == "OPCODE_TOUCHUP_HORIZONTAL" ||
                                        action.name == "OPCODE_TOUCHUP")
                                    {
                                        if (i != operationList.Count - 1)
                                        {
                                            touches[currentPointer] = null;
                                            continue;
                                        }
                                    }
                                    else if (action.name == "OPCODE_MULTI_TOUCHUP_HORIZONTAL" ||
                                        action.name == "OPCODE_MULTI_TOUCHUP")
                                    {
                                        currentPointer = Int32.Parse(action.parameters[2]);
                                        if (i != operationList.Count - 1)
                                        {
                                            touches[currentPointer] = null;
                                            continue;
                                        }
                                    }
                                    else
                                        continue;

                                    if (touches[currentPointer] == null)
                                        touches[currentPointer] = new List<actionlinetype>();
                                    touches[currentPointer].Add(action);
                                }

                                for (int i = 0; i < touches.Length; i++)
                                {
                                    if (touches[i] == null)
                                        continue;
                                    float last_X = -1, last_Y = -1;
                                    float curr_X = 0, curr_Y = 0;
                                    for (int j = 0; j < touches[i].Count; j++)
                                    {
                                        using (Pen myPen = new Pen(new SolidBrush(DaVinciCommon.IntelRed), 5))
                                        {
                                            action = touches[i][j];
                                            if (action.name == "OPCODE_TOUCHDOWN" || action.name == "OPCODE_MULTI_TOUCHDOWN" ||
                                                action.name == "OPCODE_TOUCHDOWN_HORIZONTAL" || action.name == "OPCODE_MULTI_TOUCHDOWN_HORIZONTAL")
                                            {
                                                if ((actionOrientation == DaVinciConfig.Orientation.Portrait) && (isImageBoxVertical == true) ||
                                                    (actionOrientation == DaVinciConfig.Orientation.Landscape) && (isImageBoxVertical == false))
                                                {
                                                    float X = Int32.Parse(action.parameters[0]);
                                                    float Y = Int32.Parse(action.parameters[1]);
                                                    last_X = X * imgSave.Size.Width / 4096;
                                                    last_Y = Y * imgSave.Size.Height / 4096;
                                                }
                                                else if ((actionOrientation == DaVinciConfig.Orientation.ReversePortrait) && (isImageBoxVertical == true) ||
                                                    (actionOrientation == DaVinciConfig.Orientation.ReverseLandscape) && (isImageBoxVertical == false))
                                                {
                                                    float X = Int32.Parse(action.parameters[0]);
                                                    float Y = Int32.Parse(action.parameters[1]);
                                                    last_X = (4096 - X) * imgSave.Size.Width / 4096;
                                                    last_Y = (4096 - Y) * imgSave.Size.Height / 4096;
                                                }
                                                else if ((actionOrientation == DaVinciConfig.Orientation.Landscape) && (isImageBoxVertical == true) ||
                                                    (actionOrientation == DaVinciConfig.Orientation.ReversePortrait) && (isImageBoxVertical == false))
                                                {
                                                    float X = Int32.Parse(action.parameters[0]);
                                                    float Y = Int32.Parse(action.parameters[1]);
                                                    last_X = (4096 - Y) * imgSave.Size.Width / 4096;
                                                    last_Y = X * imgSave.Size.Height / 4096;
                                                }
                                                else
                                                {
                                                    float X = Int32.Parse(action.parameters[0]);
                                                    float Y = Int32.Parse(action.parameters[1]);
                                                    last_X = Y * imgSave.Size.Width / 4096;
                                                    last_Y = (4096 - X) * imgSave.Size.Height / 4096;
                                                }
                                                this.DrawCircle(gImage, myPen, last_X, last_Y, 5);
                                            }
                                            else if (action.name == "OPCODE_TOUCHMOVE" || action.name == "OPCODE_MULTI_TOUCHMOVE" ||
                                                action.name == "OPCODE_TOUCHMOVE_HORIZONTAL" || action.name == "OPCODE_MULTI_TOUCHMOVE_HORIZONTAL")
                                            {
                                                if ((actionOrientation == DaVinciConfig.Orientation.Portrait) && (isImageBoxVertical == true) ||
                                                    (actionOrientation == DaVinciConfig.Orientation.Landscape) && (isImageBoxVertical == false))
                                                {
                                                    float X = Int32.Parse(action.parameters[0]);
                                                    float Y = Int32.Parse(action.parameters[1]);
                                                    curr_X = X * imgSave.Size.Width / 4096;
                                                    curr_Y = Y * imgSave.Size.Height / 4096;
                                                    if (last_X >= 0 && last_Y >= 0)
                                                    {
                                                        this.DrawLine(gImage, myPen, new PointF(last_X, last_Y), new PointF(curr_X, curr_Y));
                                                    }
                                                }
                                                else if ((actionOrientation == DaVinciConfig.Orientation.ReversePortrait) && (isImageBoxVertical == true) ||
                                                    (actionOrientation == DaVinciConfig.Orientation.ReverseLandscape) && (isImageBoxVertical == false))
                                                {
                                                    float X = Int32.Parse(action.parameters[0]);
                                                    float Y = Int32.Parse(action.parameters[1]);
                                                    curr_X = (4096 - X) * imgSave.Size.Width / 4096;
                                                    curr_Y = (4096 - Y) * imgSave.Size.Height / 4096;
                                                    if (last_X >= 0 && last_Y >= 0)
                                                    {
                                                        this.DrawLine(gImage, myPen, new PointF(last_X, last_Y), new PointF(curr_X, curr_Y));
                                                    }
                                                }
                                                else if ((actionOrientation == DaVinciConfig.Orientation.Landscape) && (isImageBoxVertical == true) ||
                                                    (actionOrientation == DaVinciConfig.Orientation.ReversePortrait) && (isImageBoxVertical == false))
                                                {
                                                    float X = Int32.Parse(action.parameters[0]);
                                                    float Y = Int32.Parse(action.parameters[1]);
                                                    curr_X = (4096 - Y) * imgSave.Size.Width / 4096;
                                                    curr_Y = X * imgSave.Size.Height / 4096;
                                                    if (last_X >= 0 && last_Y >= 0)
                                                    {
                                                        this.DrawLine(gImage, myPen, new PointF(last_X, last_Y), new PointF(curr_X, curr_Y));
                                                    }
                                                }
                                                else
                                                {
                                                    float X = Int32.Parse(action.parameters[0]);
                                                    float Y = Int32.Parse(action.parameters[1]);
                                                    curr_X = Y * imgSave.Size.Width / 4096;
                                                    curr_Y = (4096 - X) * imgSave.Size.Height / 4096;
                                                    if (last_X >= 0 && last_Y >= 0)
                                                    {
                                                        this.DrawLine(gImage, myPen, new PointF(last_X, last_Y), new PointF(curr_X, curr_Y));
                                                    }
                                                }
                                                last_X = curr_X;
                                                last_Y = curr_Y;
                                            }
                                            else if (action.name == "OPCODE_TOUCHUP" || action.name == "OPCODE_MULTI_TOUCHUP" ||
                                                action.name == "OPCODE_TOUCHUP_HORIZONTAL" || action.name == "OPCODE_MULTI_TOUCHUP_HORIZONTAL")
                                            {
                                                if ((actionOrientation == DaVinciConfig.Orientation.Portrait) && (isImageBoxVertical == true) ||
                                                    (actionOrientation == DaVinciConfig.Orientation.Landscape) && (isImageBoxVertical == false))
                                                {
                                                    float X = Int32.Parse(action.parameters[0]);
                                                    float Y = Int32.Parse(action.parameters[1]);
                                                    curr_X = X * imgSave.Width / 4096;
                                                    curr_Y = Y * imgSave.Height / 4096;
                                                    if (last_X >= 0 && last_Y >= 0)
                                                    {
                                                        this.DrawLine(gImage, myPen, new PointF(last_X, last_Y), new PointF(curr_X, curr_Y));
                                                    }
                                                    this.DrawCircle(gImage, myPen, last_X, last_Y, 20);
                                                }
                                                else if ((actionOrientation == DaVinciConfig.Orientation.ReversePortrait) && (isImageBoxVertical == true) ||
                                                    (actionOrientation == DaVinciConfig.Orientation.ReverseLandscape) && (isImageBoxVertical == false))
                                                {
                                                    float X = Int32.Parse(action.parameters[0]);
                                                    float Y = Int32.Parse(action.parameters[1]);
                                                    curr_X = (4096 - X) * imgSave.Width / 4096;
                                                    curr_Y = (4096 - Y) * imgSave.Height / 4096;
                                                    if (last_X >= 0 && last_Y >= 0)
                                                    {
                                                        this.DrawLine(gImage, myPen, new PointF(last_X, last_Y), new PointF(curr_X, curr_Y));
                                                    }
                                                    this.DrawCircle(gImage, myPen, last_X, last_Y, 20);
                                                }
                                                else if ((actionOrientation == DaVinciConfig.Orientation.Landscape) && (isImageBoxVertical == true) ||
                                                    (actionOrientation == DaVinciConfig.Orientation.ReversePortrait) && (isImageBoxVertical == false))
                                                {
                                                    float X = Int32.Parse(action.parameters[0]);
                                                    float Y = Int32.Parse(action.parameters[1]);
                                                    curr_X = (4096 - Y) * imgSave.Width / 4096;
                                                    curr_Y = X * imgSave.Height / 4096;
                                                    if (last_X >= 0 && last_Y >= 0)
                                                    {
                                                        this.DrawLine(gImage, myPen, new PointF(last_X, last_Y), new PointF(curr_X, curr_Y));
                                                    }
                                                    this.DrawCircle(gImage, myPen, last_X, last_Y, 20);
                                                }
                                                else
                                                {
                                                    float X = Int32.Parse(action.parameters[0]);
                                                    float Y = Int32.Parse(action.parameters[1]);
                                                    curr_X = Y * imgSave.Width / 4096;
                                                    curr_Y = (4096 - X) * imgSave.Height / 4096;
                                                    if (last_X >= 0 && last_Y >= 0)
                                                    {
                                                        this.DrawLine(gImage, myPen, new PointF(last_X, last_Y), new PointF(curr_X, curr_Y));
                                                    }
                                                    this.DrawCircle(gImage, myPen, last_X, last_Y, 20);
                                                }
                                                last_X = -1;
                                                last_Y = -1;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                double miniTime = 0;
                if (this.timeStampList != null)
                {
                    miniTime = ((double)(this.timeStampList[this.trackBar.Value]));
                }

                // change status bar time
                //this.toolStripStatusLabel2.Text = "Frame Time (ms): " + miniTime.ToString();

                // Enable the play or pause video
                if (this.trackBar.Value == this.trackBar.Maximum)
                {
                    stopVideo();
                    this.toolStripButtonPlayVideo.Enabled = false;
                }
                else
                {
                    this.toolStripButtonPlayVideo.Enabled = true;
                }
                // Change frame number on status bar
                //this.toolStripStatusLabel4.Text = "Frame: " + this.trackBar.Value.ToString() + "/" + this.trackBar.Maximum.ToString();
            }
            catch (Exception excep)
            {
                MessageBox.Show("Error happened!\n" + excep.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void saveToolStripMenuItem_Click(object sender, EventArgs e)
        {
            MessageBox.Show("Script change has been saved!", "Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
            saveToFiles();
        }

        private void saveAsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            while (this.saveFileDialog.ShowDialog() == DialogResult.OK)
            {
                string newScriptFile = Path.ChangeExtension(saveFileDialog.FileName, ".qs");
                string newVideoFile = Path.ChangeExtension(saveFileDialog.FileName, ".avi");
                string newTimeFile = Path.ChangeExtension(saveFileDialog.FileName, ".qts");
                saveToFiles(newScriptFile, newVideoFile, newTimeFile);
                return;
            }
        }

        private bool isImageBoxVertical = true;
        private void rotateToolStripMenuItem_Click(object sender, EventArgs e)
        {
            int padSize = 5;
            DaVinciAPI.ImageEventHandler qScriptImageEventHandler = new DaVinciAPI.ImageEventHandler(DisplayImg);
            GCHandle gcImageEventHandler = GCHandle.Alloc(qScriptImageEventHandler);
            DaVinciAPI.NativeMethods.SetScriptVideoImgHandler(qScriptImageEventHandler);
            if (isImageBoxVertical == true)
            {
                // Update image box layout to horizontal
                Point imageBoxLoc = this.imageBox.Location;
                int oldImageBoxHeight = this.imageBox.Height;
                this.groupBoxHelp.Location = imageBoxLoc;
                this.groupBoxHelp.Height = (this.groupBoxProperty.Height - padSize)/2;
                this.textBoxActionInfo.Height = this.groupBoxHelp.Height - padSize * 6;
                Point groupBoxLogNewLoc = new Point(imageBoxLoc.X, imageBoxLoc.Y + this.groupBoxHelp.Height + padSize);
                this.groupBoxLog.Location = groupBoxLogNewLoc;
                this.groupBoxLog.Height = this.groupBoxHelp.Height;
                this.textBoxLog.Height = this.groupBoxLog.Height - padSize * 6;
                this.toolStripVideo.Location = new Point(this.groupBoxProperty.Location.X, this.groupBoxProperty.Location.Y + this.groupBoxProperty.Height);
                this.imageBox.Location = new Point(this.toolStripVideo.Location.X, this.toolStripVideo.Location.Y + this.toolStripVideo.Height + padSize);
                this.imageBox.Height = (this.Height - (this.Icon.Height + this.menuStrip.Height + this.statusStrip.Height + this.toolStripVideo.Location.Y + this.toolStripVideo.Height));
                this.imageBox.Width = oldImageBoxHeight;
                this.trackBar.Location = new Point(this.imageBox.Location.X + this.imageBox.Width + padSize, this.imageBox.Location.Y);
                this.trackBar.Height = this.imageBox.Height;
                this.trackBar.Orientation = Orientation.Vertical;

                isImageBoxVertical = false;
                // Should refresh video frame
                if ((videoFile != null) && (File.Exists(videoFile)))
                {
                    DaVinciAPI.NativeMethods.ShowVideoFrame(vidInst, frameIndexVal);
                }
            }
            else
            {
                // Update image box layout to vertical
                int groupBoxHelpWidth = this.groupBoxHelp.Width;
                this.toolStripVideo.Location = new Point(this.groupBoxHelp.Location.X, this.comboBoxAction.Location.Y);
                this.groupBoxHelp.Location = new Point(this.groupBoxProperty.Location.X, this.groupBoxProperty.Location.Y + this.groupBoxProperty.Height + padSize);
                this.groupBoxHelp.Height = (this.Height - (this.Icon.Height + this.menuStrip.Height + this.statusStrip.Height + this.groupBoxProperty.Location.Y + this.groupBoxProperty.Height) + padSize) / 2;
                this.textBoxActionInfo.Height = this.groupBoxHelp.Height - padSize * 6;
                this.groupBoxLog.Location = new Point(this.groupBoxProperty.Location.X, this.groupBoxHelp.Location.Y + this.groupBoxHelp.Height + padSize);
                this.groupBoxLog.Height = this.groupBoxHelp.Height;
                this.textBoxLog.Height = this.groupBoxLog.Height - padSize * 6;
                this.imageBox.Location = new Point(this.toolStripVideo.Location.X, this.toolStripVideo.Location.Y + this.toolStripVideo.Height);
                this.imageBox.Width = groupBoxHelpWidth;
                this.imageBox.Height = this.groupBoxProperty.Height + this.groupBoxHelp.Height + this.groupBoxLog.Height - padSize * 6;
                this.trackBar.Location = new Point(this.toolStripVideo.Location.X, this.imageBox.Location.Y + this.imageBox.Height + padSize);
                this.trackBar.Orientation = Orientation.Horizontal;

                isImageBoxVertical = true;
                if ((videoFile != null) && (File.Exists(videoFile)))
                {
                    DaVinciAPI.NativeMethods.ShowVideoFrame(vidInst, frameIndexVal);
                }
            }

            //change tags
            string[] groupBoxListActionsTag = this.groupBoxListActions.Tag.ToString().Split(new char[] { ':' });
            string fontSize = groupBoxListActionsTag[4];
            this.imageBox.Tag = this.imageBox.Width / scaleWidth + ":" + this.imageBox.Height / scaleHeight + ":" + this.imageBox.Location.X / scaleWidth + ":" + this.imageBox.Location.Y / scaleHeight + ":" + fontSize;
            this.groupBoxHelp.Tag = this.groupBoxHelp.Width / scaleWidth + ":" + this.groupBoxHelp.Height / scaleHeight + ":" + this.groupBoxHelp.Location.X / scaleWidth + ":" + this.groupBoxHelp.Location.Y / scaleHeight + ":" + fontSize;
            this.groupBoxLog.Tag = this.groupBoxLog.Width / scaleWidth + ":" + this.groupBoxLog.Height / scaleHeight + ":" + this.groupBoxLog.Location.X / scaleWidth + ":" + this.groupBoxLog.Location.Y / scaleHeight + ":" + fontSize;
            this.trackBar.Tag = this.trackBar.Width / scaleWidth + ":" + this.trackBar.Height / scaleHeight + ":" + this.trackBar.Location.X / scaleWidth + ":" + this.trackBar.Location.Y / scaleHeight + ":" + fontSize;
            this.toolStripVideo.Tag = this.toolStripVideo.Width / scaleWidth + ":" + this.toolStripVideo.Height / scaleHeight + ":" + this.toolStripVideo.Location.X / scaleWidth + ":" + this.toolStripVideo.Location.Y / scaleHeight + ":" + fontSize;
            this.listBoxActions.Tag = this.listBoxActions.Width / scaleWidth + ":" + this.listBoxActions.Height / scaleHeight + ":" + this.listBoxActions.Location.X / scaleWidth + ":" + this.listBoxActions.Location.Y / scaleHeight + ":" + fontSize;
            this.dataGridViewActionProperty.Tag = this.dataGridViewActionProperty.Width / scaleWidth + ":" + this.dataGridViewActionProperty.Height / scaleHeight + ":" + this.dataGridViewActionProperty.Location.X / scaleWidth + ":" + this.dataGridViewActionProperty.Location.Y / scaleHeight + ":" + fontSize;
            this.groupBoxProperty.Tag = this.groupBoxProperty.Width / scaleWidth + ":" + this.groupBoxProperty.Height / scaleHeight + ":" + this.groupBoxProperty.Location.X / scaleWidth + ":" + this.groupBoxProperty.Location.Y / scaleHeight + ":" + fontSize;

            ImageBoxChangingHandler();
        }

        private void editConfigToolStripMenuItem_Click(object sender, EventArgs e)
        {
            using (QScriptConfigureForm qsConfig = new QScriptConfigureForm())
            {
                qsConfig.ShowDialog();
                if (qsConfig.needSaveConfig)
                {
                    try
                    {
                        this.saveToFiles();
                        qsConfig.needSaveConfig = false;
                        MessageBox.Show("Configure data has been saved!", "Info", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show("Configure data saving error!\n" + ex.Message, "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    }
                }
            }
        }

        private string GetPictureShotFileName(string dirname = "Video")
        {
            if (dirname == "Video")
                dirname = GetVideoFileDir();
            string picFileNamePre = dirname + "\\" + Path.GetFileNameWithoutExtension(this.GetQSFile()) + "_PictureShot";
            int picFileNameIndex = 1;
            string picFileName = picFileNamePre + picFileNameIndex.ToString() + ".png";
            while (File.Exists(picFileName))
            {
                picFileNameIndex++;
                picFileName = picFileNamePre + picFileNameIndex.ToString() + ".png";
            }
            return picFileName;
        }

        private string GetVideoFileDir()
        {
            return Path.GetDirectoryName(this.videoFile);
        }

        /// <summary>
        /// Get QS file
        /// </summary>
        /// <returns></returns>
        public String GetQSFile()
        {
            return this.scriptFile;
        }  

        private string GetScriptResourcePath(string resourceName)
        {
            string dir = Path.GetDirectoryName(this.scriptFile);
            if (dir == null)
            {
                return resourceName;
            }
            else
            {
                return Path.Combine(dir, resourceName);
            }
        }

        private void imageBox_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            string picFileName = GetPictureShotFileName();
            this.showLineLog("Picture shot is saved to \"" + picFileName + "\"");
            if (saveImg.iplImage == null || saveImg.mat == null)
            {
                return;
            }
            if (saveImg.iplImage != IntPtr.Zero)
            {
                Bitmap imgSave = DaVinciCommon.ConvertToBitmap(saveImg.iplImage);
                imgSave.Save(picFileName, ImageFormat.Png);
            }
        }


        // ImageBox operation
        private bool isMouseDown = false;
        private System.Drawing.Rectangle mouseRect = System.Drawing.Rectangle.Empty;
        private bool isCuttingPicture = false;

        // The image may not fill all the image boxes.
        int imageHeightInImageBox;
        int imageWidthInImageBox;
        bool isEditingPictureMode = false;
        bool isPictureEdited = false;

        Color editROIColor = DaVinciCommon.IntelGreen;
        Point matchXYCoord;
        int pictureEditEdge = -1;
        Rectangle editROI = new Rectangle(0,0,200,200);

        private void ResizeToRectangle(Point p)
        {
            DrawRectangleOnImageBox();
            mouseRect.Width = p.X - mouseRect.Left;
            mouseRect.Height = p.Y - mouseRect.Top;
            DrawRectangleOnImageBox();
        }

        private void DrawRectangleOnImageBox()
        {
            Rectangle rect = this.imageBox.RectangleToScreen(mouseRect);
            ControlPaint.DrawReversibleFrame(rect, DaVinciCommon.IntelWhite, FrameStyle.Dashed);
        }

        private void imageBox_MouseDown(object sender, MouseEventArgs e)
        {
            isMouseDown = true;
            if (this.isCuttingPicture)
            {
                //Cursor.Clip = this.RectangleToScreen(new Rectangle(0, 0, ClientSize.Width, ClientSize.Height));
                mouseRect = new Rectangle(e.X, e.Y, 0, 0);
            }
        }
        private void ImageBoxChangingHandler()
        {
            this.imageHeightInImageBox = this.imageBox.Height - 3;
            this.imageWidthInImageBox = this.imageBox.Width - 3;
        }

        private void normalizeCoordinateOnImageBox(int imageBox_X, int imageBox_Y, out int normalized_X, out int normalized_Y)
        {
            normalized_X = (int)(imageBox_X * 4096 / (double)this.imageWidthInImageBox);
            normalized_Y = (int)(imageBox_Y * 4096 / (double)this.imageHeightInImageBox);
        }
        private void imageBox_MouseMove(object sender, MouseEventArgs e)
        {
            double imageHeight, imageWidth;
            imageHeight = this.imageHeightInImageBox;
            imageWidth = this.imageWidthInImageBox;
            int x_pos, y_pos;
            normalizeCoordinateOnImageBox(e.X, e.Y, out x_pos, out y_pos);
            this.toolStripStatusLabelCoordinate.Text = "Coordinate X = " + x_pos.ToString() + " Y = " + y_pos.ToString();

            if (isMouseDown && isCuttingPicture)
            {
                ResizeToRectangle(e.Location);
            }

            if (saveBitmap == null)
            {
                return;
            }

            
            if (isEditingPictureMode) 
            {
                //Bitmap frame = (Bitmap)saveBitmap.Clone() ;
                Point where;
                if (isImageBoxVertical)
                {
                    double xRatio = imageHeight / saveBitmap.Width;
                    double yRatio = imageWidth / saveBitmap.Height;
                    where = new Point((int)(e.Y / xRatio), saveBitmap.Height - (int)(e.X / yRatio));
                }
                else
                {
                    double xRatio = imageWidth / saveBitmap.Width;
                    double yRatio = imageHeight / saveBitmap.Height;
                    where = new Point((int)(e.X / xRatio), (int)(e.Y / yRatio));
                }
                if (where.X < 0) where.X = 0;
                else if (where.X >= saveBitmap.Width) where.X = saveBitmap.Width - 1;
                if (where.Y < 0) where.Y = 0;
                else if (where.Y >= saveBitmap.Height) where.Y = saveBitmap.Height - 1;
                if (isMouseDown)
                {
                    if (0 <= pictureEditEdge && pictureEditEdge < 4)
                    {
                        editROIColor = DaVinciCommon.IntelBlue;
                        isPictureEdited = true;
                    }
                    else 
                    {
                        isPictureEdited = false;
                    }
                    if (pictureEditEdge == 0)
                    {
                        if (where.X < editROI.Right - 1)
                        {
                            editROI.Width = editROI.Right - where.X;
                            editROI.X = where.X;
                        }
                        else
                        {
                            editROI.X = editROI.Right - 1;
                            editROI.Width = 1;
                        }
                    }
                    else if (pictureEditEdge == 1)
                    {
                        if (where.X <= editROI.X + 1)
                        {
                            editROI.Width = 1;
                        }
                        else
                        {
                            editROI.Width = where.X - editROI.X;
                        }
                    }
                    else if (pictureEditEdge == 2)
                    {
                        if (where.Y < editROI.Bottom - 1)
                        {
                            editROI.Height = editROI.Bottom - where.Y;
                            editROI.Y = where.Y;
                        }
                        else
                        {
                            editROI.Y = editROI.Bottom - 1;
                            editROI.Height = 1;
                        }
                    }
                    else if (pictureEditEdge == 3)
                    {
                        if (where.Y <= editROI.Y + 1)
                        {
                            editROI.Height = 1;
                        }
                        else
                        {
                            editROI.Height = where.Y - editROI.Y;
                        }
                    }
                }
                else
                {
                    this.Cursor = Cursors.Cross;
                    pictureEditEdge = -1;
                    Cursor splitX, splitY;
                    if (isImageBoxVertical)
                    {
                        splitX = Cursors.HSplit;
                        splitY = Cursors.VSplit;
                    }
                    else
                    {
                        splitX = Cursors.VSplit;
                        splitY = Cursors.HSplit;
                    }
                    if (Math.Abs(where.X - editROI.X) < 2 && editROI.Y <= where.Y && where.Y < editROI.Bottom)
                    {
                        pictureEditEdge = 0;
                        this.Cursor = splitX;
                    }
                    else if (Math.Abs(where.X - editROI.Right + 1) < 2 && editROI.Y <= where.Y && where.Y < editROI.Bottom)
                    {
                        pictureEditEdge = 1;
                        this.Cursor = splitX;
                    }
                    else if (Math.Abs(where.Y - editROI.Y) < 2 && editROI.X <= where.X && where.X < editROI.Right)
                    {
                        pictureEditEdge = 2;
                        this.Cursor = splitY;
                    }
                    else if (Math.Abs(where.Y - editROI.Bottom + 1) < 2 && editROI.X <= where.X && where.X < editROI.Right)
                    {
                        pictureEditEdge = 3;
                        this.Cursor = splitY;
                    }
                    else
                    {
                        isPictureEdited = false;
                    }
                }

                if (isPictureEdited)
                {
                    Bitmap frame = (Bitmap)saveBitmap.Clone();
                    using (Graphics g = Graphics.FromImage(frame))
                    {
                        Pen p = new Pen(editROIColor, 4);
                        g.DrawRectangle(p, editROI);
                        p = new Pen(DaVinciCommon.IntelRed, 2);
                        g.DrawEllipse(p, matchXYCoord.X, matchXYCoord.Y, 5, 5);
                        if (isImageBoxVertical)
                        {
                            frame.RotateFlip(RotateFlipType.Rotate90FlipNone);
                        }
                    }
                    this.imageBox.BitmapToDraw = frame;
                    
                }
                
                return;
            }
        }

        private void imageBox_MouseUp(object sender, MouseEventArgs e)
        {
            this.imageBox.MouseMove -= this.imageBox_MouseMove;
            this.isMouseDown = false;
            if (isCuttingPicture)
            {
                DrawRectangleOnImageBox();
                if (saveBitmap != null)
                {
                    var current = imageBox.Image;
                    Bitmap fromImage = DaVinciCommon.ConvertToBitmap(current.iplImage);

                    int width, height, offsetX, offsetY;
                    if (mouseRect.Width > 0)
                    {
                        width = mouseRect.Width;
                        offsetX = mouseRect.Left;
                    }
                    else
                    {
                        width = -mouseRect.Width;
                        offsetX = mouseRect.Right;
                    }
                    if (mouseRect.Height > 0)
                    {
                        height = mouseRect.Height;
                        offsetY = mouseRect.Top;
                    }
                    else
                    {
                        height = -mouseRect.Height;
                        offsetY = mouseRect.Bottom;
                    }
                    width = (int)(width * fromImage.Width / (double)this.imageBox.Width);
                    height = (int)(height * fromImage.Height / (double)this.imageBox.Height);
                    offsetX = (int)(offsetX * fromImage.Width / (double)this.imageBox.Width);
                    offsetY = (int)(offsetY * fromImage.Height / (double)this.imageBox.Height);

                    if (width < 1 || height < 1) return;
                    
                    Rectangle rect = new Rectangle(offsetX, offsetY, width, height);
                    try
                    {
                        Bitmap toImage = fromImage.Clone(rect, fromImage.PixelFormat);
                        if (isImageBoxVertical)
                            toImage.RotateFlip(RotateFlipType.Rotate270FlipNone);
                        // Save the pictureshot
                        string picFileName = GetPictureShotFileName();
                        toImage.Save(picFileName);
                        this.showLineLog("Picture shot is saved to \"" + picFileName + "\"");
                    }
                    catch (System.OutOfMemoryException)
                    {
                        mouseRect = Rectangle.Empty;
                        this.isCuttingPicture = false;
                        this.Cursor = Cursors.Default;
                        MessageBox.Show("Snapshot area is outside of the source bitmap bounds!\nPlease re-do snapshot!", 
                            "Warning", 
                            MessageBoxButtons.OK, 
                            MessageBoxIcon.Exclamation);
                    }
                }
                mouseRect = Rectangle.Empty;
                this.isCuttingPicture = false;
                this.Cursor = Cursors.Default;
            }
            if (isEditingPictureMode)
            {
                if (isPictureEdited)
                {
                    Rectangle currentFrameEdit = editROI;
                    DaVinciAPI.Bounding b;
                    b.X = editROI.X;
                    b.Y = editROI.Y;
                    b.Width = editROI.Width;
                    b.Height = editROI.Height;
                    actionlinetype actionline = this.actions[this.listBoxActions.SelectedIndex];
                    DaVinciAPI.BoolType isValidRoi = DaVinciAPI.NativeMethods.IsROIMatch(ref saveImg, b, GetScriptResourcePath(actionline.parameters[0]));
                    if (isValidRoi == DaVinciAPI.BoolType.BoolTrue)
                    {
                        editROIColor = DaVinciCommon.IntelGreen;
                    }
                    else
                    {
                        editROIColor = DaVinciCommon.IntelRed;
                    }
                    Bitmap frameToDraw = (Bitmap)saveBitmap.Clone();
                    using (Graphics g = Graphics.FromImage(frameToDraw))
                    {
                        Pen p = new Pen(editROIColor, 4);
                        g.DrawRectangle(p, editROI);
                        p = new Pen(DaVinciCommon.IntelRed, 2);
                        g.DrawEllipse(p, matchXYCoord.X, matchXYCoord.Y, 5, 5);

                        if (isImageBoxVertical)
                        {
                            frameToDraw.RotateFlip(RotateFlipType.Rotate90FlipNone);
                        }
                    }
                    this.imageBox.BitmapToDraw = frameToDraw;
                }
            }
            this.imageBox.MouseMove += this.imageBox_MouseMove;
        }

        /// <summary>
        /// Get the absolute click point on the frame from the given action specified by the label.
        /// If the click point cannot be retrieved from the action, the function returns negative coordinates.
        /// </summary>
        /// <param name="label"></param>
        /// <param name="width"></param>
        /// <param name="height"></param>
        /// <returns></returns>
        private Point GetAbsoluteClickPointOnFrame(int label, int width, int height)
        {
            int actionIndex = getActionsIndexFromLabel(label);
            if (actionIndex != -1 &&
                (this.actions[actionIndex].name == "OPCODE_CLICK_MATCHED_IMAGE_XY" ||
                 this.actions[actionIndex].name == "OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY"))
            {
                actionlinetype theAction = this.actions[actionIndex];
                int x, y;
                int.TryParse(theAction.parameters[3], out x);
                int.TryParse(theAction.parameters[4], out y);
                return NormToFrameCoord(saveBitmap.Size, new Point(x, y), GetActionOrientation(actionIndex));
            }
            else
            {
                return new Point(-100, -100);
            }
        }

        /// <summary>
        /// Transform the normalized coordinate (4096x4096) into frame coordinate.
        /// Frame coordinate is always Landscape.
        /// </summary>
        /// <param name="frameSize">Size of the frame.</param>
        /// <param name="norm">Normalized point</param>
        /// <param name="orientation">Orientation of normalized coordinate</param>
        /// <returns>Point in frame coordinate</returns>
        public static Point NormToFrameCoord(Size frameSize, Point norm, DaVinciConfig.Orientation orientation)
        {
            Point result = new Point();
            double ratioX = (double)frameSize.Width / 4096;
            double ratioY = (double)frameSize.Height / 4096;
            if (DaVinciConfig.IsHorizontalOrientation(orientation))
            {
                result.X = (int)(norm.X * ratioX);
                result.Y = (int)(norm.Y * ratioY);
            }
            else
            {
                result.X = (int)(norm.Y * ratioX);
                result.Y = frameSize.Height - (int)(norm.X * ratioY);
            }
            if (DaVinciConfig.IsReverseOrientation(orientation))
            {
                result.X = frameSize.Width - result.X;
                result.Y = frameSize.Height - result.Y;
            }
            return result;
        }

        private void imageBox_MouseLeave(object sender, EventArgs e)
        {
            this.Cursor = Cursors.Default;
        }

        private void toolStripButtonShot_Click(object sender, EventArgs e)
        {
            if (this.isCuttingPicture)
            {
                this.isCuttingPicture = false;
                this.Cursor = Cursors.Default;
            }
            else
            {
                this.isCuttingPicture = true;
                this.Cursor = Cursors.Cross;
            }
        }

        private bool OpcodeNeedImageObject(string opcode)
        {
            return opcode == "OPCODE_IF_MATCH_IMAGE" || opcode == "OPCODE_IF_MATCH_IMAGE_WAIT" || opcode == "OPCODE_CONCURRENT_IF_MATCH";
        }

        private bool isOpCodeParaDependencyLegal()
        {
            List<string> variableNameList = new List<string>();
            for (int i = 0; i < listBoxActions.Items.Count; i++)
            {
                string listActionStr = listBoxActions.Items[i].ToString();
                System.Text.RegularExpressions.Regex replaceSpaces = new System.Text.RegularExpressions.Regex(@"\s{1,}");
                listActionStr = replaceSpaces.Replace(listActionStr, " ").Trim();
                if (listActionStr.Contains("OPCODE_SET_VARIABLE"))
                {
                    string variableNameSet = listActionStr.Split(' ')[3];
                    if (!variableNameList.Contains(variableNameSet))
                    {
                        variableNameList.Add(variableNameSet);
                    }
                    else
                    {
                        MessageBox.Show("Duplicate variable name set by OPCODE_SET_VARIABLE!\nPlease specify them different!",
                            "Warning",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Exclamation);
                        return false;
                    }
                }
                if (listActionStr.Contains("OPCODE_LOOP"))
                {
                    bool isSetineVar = false;
                    string variableNameLoop = listActionStr.Split(' ')[3];
                    foreach (string varName in variableNameList)
                    {
                        if (varName == variableNameLoop)
                        {
                            isSetineVar = true;
                            break;
                        }
                    }
                    if (!isSetineVar)
                    {
                        MessageBox.Show("Variable name has not been set!\nPlease set it via OPCODE_SET_VARIABLE!",
                            "Warning",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Exclamation);
                        return false;
                    }
                }
            }
            return true;
        }

        private bool isOpCodeInvalid(string opcodeCheck)
        {
            bool invalid = true;
            foreach (string opcode in this.comboBoxAction.Items)
            {
                if (opcode == opcodeCheck)
                {
                    invalid = false;
                    break;
                }
            }
            return invalid;
        }

        private actionlinetype getActionLineInput()
        {
            if (this.comboBoxAction.Text == "")
            {
                MessageBox.Show("Empty action!",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
                return null;
            }

            // Check whether input OPCODE is invalid
            if (isOpCodeInvalid(this.comboBoxAction.Text))
            {
                MessageBox.Show("Unknown OPCODE, please correct input!",
                        "Warning",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Exclamation);
                return null;
            }

            actionlinetype actionline = new actionlinetype(this.comboBoxAction.Text);
            if (!(int.TryParse(this.dataGridViewActionProperty.Rows[0].Cells[1].EditedFormattedValue.ToString(), out actionline.label)))
            {
                actionline.label = -1;
            }
            if (actionline.label < 0)
            {
                MessageBox.Show("Invalid Label!",
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return null;
            }

            if (!double.TryParse(this.dataGridViewActionProperty.Rows[1].Cells[1].EditedFormattedValue.ToString(), out actionline.time) ||
                actionline.time < 0)
            {
                MessageBox.Show("Invalid Time!",
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return null;
            }

            int numParams = this.dataGridViewActionProperty.Rows.Count - 2;
            if (OpcodeNeedImageObject(actionline.name))
            {
                numParams -= 3; // delete two pseudo params for Angle and AngleError
            }
            else if (actionline.name == "OPCODE_IMAGE_CHECK")
            {
                numParams --; // ignore Reference Match Ratio
            }
            if (numParams > 0)
                actionline.parameters = new string[numParams];

            int gridIndex = 2;
            int paramIndex = 0;

            char[] invalidChars = Path.GetInvalidFileNameChars();
            string touchDownLayoutPara = "";
            while (gridIndex < this.dataGridViewActionProperty.Rows.Count)
            {
                if (this.dataGridViewActionProperty.Rows[gridIndex].Cells[1].EditedFormattedValue == null ||
                    this.dataGridViewActionProperty.Rows[gridIndex].Cells[1].EditedFormattedValue.ToString() == "")
                {
                    string paraName = this.dataGridViewActionProperty.Rows[gridIndex].Cells[0].Value.ToString();
                    if (paraName.Length > 0 && paraName[0] != '[')
                    {
                        MessageBox.Show("Missing parameter \"" + paraName + "\"!",
                            "Warning",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Exclamation);
                        return null;
                    }
                }

                if (!System.Text.RegularExpressions.Regex.IsMatch(this.dataGridViewActionProperty.Rows[gridIndex].Cells[1].EditedFormattedValue.ToString(), "^(0|[1-9][0-9]*)$"))
                {
                    string paraName = this.dataGridViewActionProperty.Rows[gridIndex].Cells[0].Value.ToString();
                    if (paraName.Length > 0)
                    {
                        foreach (string name in paraIntCheckList)
                        {
                            if (paraName.Contains(name))
                            {
                                MessageBox.Show("Parameter " + paraName + " is invalid!",
                                        "Warning",
                                        MessageBoxButtons.OK,
                                        MessageBoxIcon.Exclamation);
                                return null;
                            }
                        }
                        if ((paraName.Contains("Unused Parameter") || (paraName.Contains("Tolerance"))) && (this.dataGridViewActionProperty.Rows[gridIndex].Cells[1].Value != null))
                        {
                            MessageBox.Show("Parameter " + paraName + " is invalid!",
                                "Warning",
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Exclamation);
                            return null;
                        }
                    }
                }

                if (this.dataGridViewActionProperty.Rows[gridIndex].Cells[1].EditedFormattedValue != null ||
                    this.dataGridViewActionProperty.Rows[gridIndex].Cells[1].EditedFormattedValue.ToString() != "")
                {
                    string value = this.dataGridViewActionProperty.Rows[gridIndex].Cells[1].EditedFormattedValue.ToString();
                    string paraName = this.dataGridViewActionProperty.Rows[gridIndex].Cells[0].EditedFormattedValue.ToString();
                    if (paraName.Length > 0 && (paraName.Equals("Degree")))
                    {
                        int paraValue = Convert.ToInt32(value);
                        if (paraValue < 0 || paraValue > 90)
                        {
                            MessageBox.Show("Parameter " + paraName + " is invalid! The range of Degree is: 0~90",
                                "Warning",
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Exclamation);
                            return null;
                        }
                    }
                    if (paraName.Length > 0 && (paraName.Contains("Arm") && (paraName.Contains("Degree"))))
                    {
                        int paraValue = Convert.ToInt32(value);
                        if (paraValue < 0 || paraValue > 180)
                        {
                            MessageBox.Show("Parameter " + paraName + " is invalid! The range of Arm* Degree is 0~180",
                                "Warning",
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Exclamation);
                            return null;
                        }
                    }

                    foreach (string pathName in paraFileNameCheckList)
                    {
                        if (paraName.ToLower().Contains(pathName.ToLower()))
                        {
                            foreach (char invalidChar in invalidChars)
                            {
                                if (value.Contains(invalidChar))
                                {
                                    MessageBox.Show("File name should not contain invalid characters!",
                                        "Warning",
                                        MessageBoxButtons.OK,
                                        MessageBoxIcon.Exclamation);
                                    return null;
                                }
                            }
                        }
                    }
                }

                if (this.dataGridViewActionProperty.Rows[gridIndex].Cells[1].EditedFormattedValue != null ||
                    this.dataGridViewActionProperty.Rows[gridIndex].Cells[1].EditedFormattedValue.ToString() != "")
                {
                    string paraName = this.dataGridViewActionProperty.Rows[gridIndex].Cells[0].Value.ToString();
                    string valString = this.dataGridViewActionProperty.Rows[gridIndex].Cells[1].EditedFormattedValue.ToString();
                    if ((paraName.Length > 0 && paraName.Contains("Rule")) &&
                        (valString != "1" && valString != "2" && valString != "4" && valString != "8" && valString != "12"))
                    {
                        MessageBox.Show("Parameter " + paraName + " is invalid!",
                                "Warning",
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Exclamation
                            );
                        return null;
                    }
                }


                if (gridIndex == 2 && OpcodeNeedImageObject(actionline.name))
                {
                    TestUtil.ImageObject imageObject = new TestUtil.ImageObject();
                    imageObject.ImageName = this.dataGridViewActionProperty.Rows[gridIndex].Cells[1].EditedFormattedValue.ToString();
                    try
                    {
                        imageObject.Angle = int.Parse(this.dataGridViewActionProperty.Rows[gridIndex + 1].Cells[1].EditedFormattedValue.ToString());
                        imageObject.AngleError = int.Parse(this.dataGridViewActionProperty.Rows[gridIndex + 2].Cells[1].EditedFormattedValue.ToString());
                        imageObject.RatioReferenceMatch = float.Parse(this.dataGridViewActionProperty.Rows[gridIndex + 3].Cells[1].EditedFormattedValue.ToString());
                    }
                    catch
                    {
                        Console.WriteLine("getActionLineInput exception!");
                    }
                    actionline.parameters[paramIndex] = imageObject.ToString();
                    gridIndex += 4;
                }
                else if (gridIndex == 2 && actionline.name == "OPCODE_IMAGE_CHECK")
                {
                    TestUtil.ImageObject imageObject = new TestUtil.ImageObject();
                    imageObject.ImageName = this.dataGridViewActionProperty.Rows[gridIndex].Cells[1].EditedFormattedValue.ToString();
                    try
                    {
                        imageObject.RatioReferenceMatch = float.Parse(this.dataGridViewActionProperty.Rows[gridIndex + 1].Cells[1].EditedFormattedValue.ToString());
                    }
                    catch
                    {
                        Console.WriteLine("getActionLineInput exception!");
                    }
                    actionline.parameters[paramIndex] = imageObject.ToString();
                    gridIndex += 2;
                }
                else
                {
                    actionline.parameters[paramIndex] = this.dataGridViewActionProperty.Rows[gridIndex].Cells[1].EditedFormattedValue.ToString();
                    // Process device UI layout parameter for OPCODE_TOUCHDOWN. UI layout parameter should be assembled by "name" and "value"
                    // in the form like: layout=record_1.tree&sb=(0,0,1200,50)&nb=(0,1824,1200,1920)&kb=0&fw=(0,50,1200,1824)
                    if ((paramIndex >= 3) && (actionline.name == "OPCODE_TOUCHDOWN"))
                    {
                        string paraTmp = this.dataGridViewActionProperty.Rows[gridIndex].Cells[0].EditedFormattedValue.ToString() + '='
                            + this.dataGridViewActionProperty.Rows[gridIndex].Cells[1].EditedFormattedValue.ToString() + '&';
                        touchDownLayoutPara += paraTmp;
                        if ((paramIndex == actionline.parameters.Length - 1) && (touchDownLayoutPara.EndsWith("&")))
                        {
                            actionline.parameters[3] = "";
                            touchDownLayoutPara = touchDownLayoutPara.Remove(touchDownLayoutPara.Length - 1);
                            actionline.parameters[3] = touchDownLayoutPara;
                            ArrayList actionLinePara = new ArrayList(actionline.parameters);
                            for (int i = actionline.parameters.Length - 1; i >= 4; i--)
                            {
                                actionLinePara.RemoveAt(i);
                            }
                            actionline.parameters = (string[])actionLinePara.ToArray(typeof(string));
                        }
                    }
                    gridIndex++;
                }
                paramIndex++;
            }
            return actionline;
        }

        private System.Collections.Generic.List<actionlinetype> oldActions;
        // Video playing
        private System.Timers.Timer videoTimer;
        private bool videoRunning = false;
        private int playUntil = 0;

        // ImageBox operation
        //private bool isMouseDown = false;
        //private bool isCuttingPicture = false;
        //private System.Drawing.Rectangle mouseRect = System.Drawing.Rectangle.Empty;

        private void toolStripButtonInsertAction_Click(object sender, EventArgs e)
        {
            int oldSelectedIndex = this.listBoxActions.SelectedIndex;
            // Do not support to new a script file now
            if (this.actions == null)
                return;
            if (oldSelectedIndex < 0 && this.actions.Count != 0)
            {
                MessageBox.Show("Please choose where to insert.",
                    "Information",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                return;
            }
            actionlinetype actionline = getActionLineInput();
            if (actionline == null)
                return;
            if (isLabelInScript(actionline.label))
            {
                MessageBox.Show("The label is already used.",
                    "Information",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                return;
            }

            this.oldActions = new List<actionlinetype>(this.actions);
            int insertIndex = oldSelectedIndex;
            if (oldSelectedIndex < 0)
                insertIndex = 0;
            this.actions.Insert(insertIndex, actionline);
            this.flashListBoxActions();
            this.listBoxActionsSelect(insertIndex);

            this.isScriptFileChanged = true;
            isOpCodeParaDependencyLegal();
        }

        private void toolStripButtonDeleteAction_Click(object sender, EventArgs e)
        {
            int oldSelectedIndex = this.listBoxActions.SelectedIndex;

            // Do not support to new a script file now
            if (this.actions == null)
                return;
            if (this.listBoxActions.SelectedIndices.Count <= 0)
            {
                MessageBox.Show("Please choose which action to delete.",
                    "Information",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                return;
            }

            this.oldActions = new List<actionlinetype>(this.actions);
            List<actionlinetype> removedActions = new List<actionlinetype>();
            foreach (int index in this.listBoxActions.SelectedIndices)
                removedActions.Add(this.actions[index]);
            this.actions.RemoveAll(action => removedActions.IndexOf(action) >= 0);
            this.flashListBoxActions();
            if (oldSelectedIndex < this.actions.Count)
                listBoxActionsSelect(oldSelectedIndex);
            else if (this.actions.Count > 0)
                listBoxActionsSelect(this.actions.Count - 1);

            this.isScriptFileChanged = true;
            isOpCodeParaDependencyLegal();
        }

        private void toolStripButtonChangeAction_Click(object sender, EventArgs e)
        {
            int oldSelectedIndex = this.listBoxActions.SelectedIndex;
            // Do not support to new a script file now
            if (this.actions == null)
                return;
            if (oldSelectedIndex < 0 || this.listBoxActions.SelectedIndices.Count > 1)
            {
                MessageBox.Show("Please choose which action to edit.",
                    "Information",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                return;
            }
            actionlinetype actionline = getActionLineInput();
            if (actionline == null)
                return;
            if (oldSelectedIndex < this.actions.Count)
            {
                if (actionline.label != this.actions[oldSelectedIndex].label && isLabelInScript(actionline.label))
                {
                    MessageBox.Show("The label is already used.",
                        "Information",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Information);
                    return;
                }
            }
            else if (isLabelInScript(actionline.label))
            {
                MessageBox.Show("The label is already used.",
                    "Information",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                return;
            }

            this.oldActions = new List<actionlinetype>(this.actions);
            if (oldSelectedIndex >= this.actions.Count)
                this.actions.Add(actionline);
            else
                this.actions[oldSelectedIndex] = actionline;
            this.flashListBoxActions();
            if (oldSelectedIndex < this.actions.Count)
                this.listBoxActionsSelect(oldSelectedIndex);
            else if (this.actions.Count > 0)
                this.listBoxActionsSelect(this.actions.Count - 1);
            this.isScriptFileChanged = true;
            isOpCodeParaDependencyLegal();
        }

        private void toolStripButtonUndo_Click(object sender, EventArgs e)
        {
            if (this.oldActions == null)
                return;
            this.actions = this.oldActions;
            this.oldActions = null;

            int oldSelectedIndex = this.listBoxActions.SelectedIndex;
            if (oldSelectedIndex == -1)
            {
                oldSelectedIndex = 0;
            }
            this.flashListBoxActions();
            if (oldSelectedIndex < this.actions.Count)
                this.listBoxActionsSelect(oldSelectedIndex);
            else if (this.actions.Count > 0)
                this.listBoxActionsSelect(this.actions.Count - 1);
        }

        private void toolStripButtonEditReferenceImage_Click(object sender, EventArgs e)
        {
            if (saveBitmap == null || this.actions == null ||
                this.listBoxActions.SelectedIndex >= this.actions.Count ||
                this.listBoxActions.SelectedIndex < 0)
            {
                return;
            }

            if (this.listBoxActions.SelectedIndices.Count > 1)
            {
                MessageBox.Show("Please choose only one line!",
                    "Information",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                return;
            }

            double ratioX = (double)saveBitmap.Width / 4096;
            double ratioY = (double)saveBitmap.Height / 4096;

            if (isEditingPictureMode)
            {
                DaVinci.TestUtil.EnableDisableControls(this, true);
                //if (saveBitmap != null) saveBitmap.Dispose();
                this.imageBox.IsEditingPicture = false;
                if (isPictureEdited)
                {
                    flashListBoxActions();
                   // UpdateImageBox(currentFrame);
                    isEditingPictureMode = false;
                    this.Cursor = Cursors.Default;
                    return;
                }
            }
            else
            {
                actionlinetype actionline = this.actions[this.listBoxActions.SelectedIndex];
                isPictureEdited = false;
                if (actionline.name == "OPCODE_IF_MATCH_IMAGE" ||
                    actionline.name == "OPCODE_IF_MATCH_IMAGE_WAIT" ||
                    actionline.name == "OPCODE_CLICK_MATCHED_IMAGE_XY" ||
                    actionline.name == "OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY" ||
                    actionline.name == "OPCODE_TOUCHUP_MATCHED_IMAGE_XY" ||
                    actionline.name == "OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY" ||
                    actionline.name == "OPCODE_IMAGE_CHECK")
                {
                    string trueLabelStr = actionline.parameters[1];
                    int trueLabel;
                    try
                    {
                        trueLabel = int.Parse(trueLabelStr);
                    }
                    catch
                    {
                        trueLabel = -1;
                    }
                    Bitmap currentFrame = (Bitmap)saveBitmap.Clone();
                    string modelImageName = GetScriptResourcePath(actionline.parameters[0]);
                    if (modelImageName.IndexOf("://") == -1)
                    {
                        modelImageName = modelImageName.Insert(0, "file:///");
                    }

                    matchXYCoord = GetAbsoluteClickPointOnFrame(trueLabel, currentFrame.Width, currentFrame.Height);
                    var uri = new Uri(modelImageName);
                    if (!File.Exists(uri.LocalPath))
                    {
                        MessageBox.Show("Model image " + uri.LocalPath + " cannot be found!",
                            "Warning",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Exclamation);
                        return;
                    }
                    this.imageBox.IsEditingPicture = true;
                    isEditingPictureMode = true;
                    DaVinciAPI.Bounding bounding = DaVinciAPI.NativeMethods.GetImageBounding(ref saveImg, uri.LocalPath);
                    int centerX = matchXYCoord.X - 10;
                    int centerY = matchXYCoord.Y - 10;
                    if (bounding.Width > 0 && bounding.Height > 0)
                    {
                        editROI = new Rectangle(bounding.X, bounding.Y, bounding.Width, bounding.Height);
                        editROIColor = DaVinciCommon.IntelGreen;
                    }
                    else
                    {
                        if (matchXYCoord.X < 0 || matchXYCoord.Y < 0)
                        {
                            centerX = currentFrame.Width / 2;
                            centerY = currentFrame.Height / 2;
                        }
                        editROI = new Rectangle(new Point(centerX, centerY), new Size(20, 20));
                        if (editROI.X < 0) editROI.X = 0;
                        if (editROI.Y < 0) editROI.Y = 0;
                        if (editROI.Right > currentFrame.Width) editROI.Width = currentFrame.Width - editROI.X;
                        if (editROI.Bottom > currentFrame.Height) editROI.Height = currentFrame.Height - editROI.Y;
                        editROIColor = DaVinciCommon.IntelRed;
                        this.showLineLog("Model image " + uri.LocalPath + " cannot be found on the current frame!");
                    }
                    using (Graphics g = Graphics.FromImage(currentFrame))
                    {
                        Pen p = new Pen(editROIColor, 4);
                        g.DrawRectangle(p, editROI);
                        p = new Pen(DaVinciCommon.IntelRed, 2);
                        g.DrawEllipse(p, centerX, centerY, 5, 5);
                        if (isImageBoxVertical)
                        {
                            currentFrame.RotateFlip(RotateFlipType.Rotate90FlipNone);
                        }
                    }
                    this.imageBox.BitmapToDraw = currentFrame;
                    this.showLineLog("The clicking coordinate has been marked on the current frame.");
                    isEditingPictureMode = true;
                    this.Cursor = Cursors.Default;
                    DaVinci.TestUtil.EnableDisableControls(this, false);
                    this.toolStripButtonEditReferenceImage.Enabled = true;
                    this.imageBox.Enabled = true;
                    EnableDisableControlChain(this.toolStripButtonEditReferenceImage.GetCurrentParent(), true);
                    //UpdateImageBox(currentFrame, currentFrameClone);
                }
            }
        }

        private void toolStripButtonPosition_Click(object sender, EventArgs e)
        {
            if (this.imageBox.Image.iplImage == null || this.imageBox.Image.mat == null || 
                this.timeStampList == null || this.qs_trace == null || this.actions == null)
                return;
            int traceIndex = getLastTraceNumberFromStamp(this.timeStampList[this.trackBar.Value]);
            if (traceIndex < 0)
            {
                listBoxActionsSelect(0);
            }
            else
            {
                int actionsIndex = getActionsIndexFromLabel(this.qs_trace[traceIndex].label);
                if (actionsIndex < 0)
                {
                    MessageBox.Show("label " + this.qs_trace[traceIndex].label.ToString() + " is not found! Maybe you have changed the qscript.",
                        "Information",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Information);
                }
                else
                {
                    listBoxActionsSelect(actionsIndex);
                }
            }
        }

        private void stopVideo()
        {
            if (this.videoRunning)
            {
                this.videoRunning = false;
                this.toolStripButtonPlayVideo.Image = global::DaVinci.Properties.Resources.button_play;
            }
        }

        private void timerBackToControl(object sender, System.Timers.ElapsedEventArgs e)
        {
            if (!this.videoRunning) return;

            if (TestUtil.AddControlPropertyValueUntil(this.trackBar, "Value", this.playUntil))
            {
                videoTimer.Interval = 1000.0 / 30.0;
                videoTimer.AutoReset = false;
                videoTimer.Enabled = true;
            }
            else
            {
                videoTimer.Enabled = false;
                this.videoRunning = false;
                this.toolStripButtonPlayVideo.Image = global::DaVinci.Properties.Resources.button_play;
            }
        }

        private void playVideo()
        {
            if (this.videoRunning)
            {
                this.videoRunning = false;
                this.toolStripButtonPlayVideo.Image = global::DaVinci.Properties.Resources.button_play;
            }
            else if (this.trackBar.Value < this.playUntil)
            {
                videoTimer = new System.Timers.Timer();
                videoTimer.Elapsed += new ElapsedEventHandler(timerBackToControl);
                videoTimer.Interval = 1000.0 / 30.0;
                videoTimer.AutoReset = false;
                videoTimer.Enabled = true;

                this.videoRunning = true;
                this.toolStripButtonPlayVideo.Image = global::DaVinci.Properties.Resources.button_pause;
            }
        }

        /// <summary>
        /// Find those actions in the trace to be executed immediately before the next frame. 
        /// </summary>
        /// <param name="frameIndex"> current frame index </param>
        /// <returns> All the trace indexes of those found actions </returns>
        private List<int> getTraceNumberAtFrame(int frameIndex)
        {
            List<int> result = new List<int>();
            if (this.qs_trace == null || this.qs_trace.Count == 0 || frameIndex >= this.trackBar.Maximum)
                return result;

            double stamp1 = this.timeStampList[frameIndex], stamp2 = this.timeStampList[frameIndex + 1];
            int i;
            for (i = 0; i < this.qs_trace.Count; i++)
            {
                if (this.qs_trace[i].time < stamp1)
                    continue;
                if (this.qs_trace[i].time >= stamp2)
                    return result;
                result.Add(i);
            }
            return result;
        }

        /// <summary>
        /// Get the line number of the opcode(s) which saved in qs file
        /// </summary>
        /// <param name="label"></param>
        /// <returns></returns>
        private int getLineNumberFromLabel(int label)
        {
            int lineNumber = 0;
            if (this.actions == null)
            {
                return -1;
            }
            for (int i = 0; i < actions.Count; i++)
            {
                if (actions[i].label != label)
                {
                    lineNumber++;
                }
                else
                {
                    break;
                }
            }
            return lineNumber;
        }

        /// <summary>
        /// If the previous/next action is new added manually or through script transform,
        /// then return true, otherwise return false
        /// </summary>
        /// <param name="label"></param>
        /// <param name="nextStep">If true, then move to next line, otherwise move to previous line</param>
        /// <returns></returns>
        private bool isNextActionNewAdded(int label, bool nextStep = true)
        {
            int nextLine = 0;
            int curLineNumber = getLineNumberFromLabel(label);
            if (nextStep)
            {
                if (curLineNumber + 1 < actions.Count)
                {
                    nextLine = curLineNumber + 1;
                }
                else
                {
                    nextLine = curLineNumber;
                }
            }
            else
            {
                if (curLineNumber - 1 > 0)
                {
                    nextLine = curLineNumber - 1;
                }
                else
                {
                    nextLine = 0;
                }
            }
            int nextLabel = actions[nextLine].label;
            for (int i = 0; i < qs_trace.Count; i++)
            {
                if (nextLabel == qs_trace[i].label)
                {
                    return false;
                }
            }
            return true;
        }

        private void toolStripButtonPrevStep_Click(object sender, EventArgs e)
        {
            stopVideo();

            if (this.actions == null || this.listBoxActions.SelectedIndex < 0 || this.listBoxActions.SelectedIndex >= this.actions.Count ||
                this.timeStampList == null || this.qs_trace == null || this.videoFile == null)
                return;

            if (this.listBoxActions.SelectedIndices.Count > 1)
            {
                MessageBox.Show("Please choose only one line.",
                    "Information",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                return;
            }

            actionlinetype action = this.actions[this.listBoxActions.SelectedIndex];
            List<int> traceIndexes = getTraceNumberAtFrame(this.trackBar.Value);
            List<int> labelList = new List<int>();
            for (int i = 0; i < traceIndexes.Count; i++)
            {
                labelList.Add(this.qs_trace[traceIndexes[i]].label);
            }
            int labelIndex = labelList.IndexOf(action.label);
            if (traceIndexes.Count == 0 || labelIndex < 0)
            {
                int frameIndex = getFrameNumberFromLabel(action.label, this.trackBar.Value);
                if ((frameIndex >= 0) && (frameIndex < this.trackBar.Maximum))
                {
                    this.trackBar.Value = frameIndex;
                }
                else if (frameIndex >= this.trackBar.Maximum)
                {
                    // If already at the last OPCODE
                    this.trackBar.Value = frameIndex;
                    int lineNumber = getLineNumberFromLabel(action.label);
                    if (lineNumber > 0)
                    {
                        listBoxActionsSelect(lineNumber - 1);
                    }
                }
                else
                {
                    this.trackBar.Value = 0;
                    int lineNumber = getLineNumberFromLabel(action.label);
                    if (lineNumber > 0)
                    {
                        listBoxActionsSelect(lineNumber - 1);
                    }
                    else
                    {
                        MessageBox.Show("Already the beginning of the execution.",
                            "Information",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Information);
                    }
                }
                return;
            }
            else if (traceIndexes[labelIndex] > 0)
            {
                if (!isNextActionNewAdded(action.label, false))
                {
                    TimeStampFileReader.traceInfo ti = this.qs_trace[traceIndexes[labelIndex] - 1];
                    int stampIndex = getStampNumberBeforeStamp(ti.time);
                    if (stampIndex >= 0)
                    {
                        int actionsIndex = getActionsIndexFromLabel(ti.label);
                        if (actionsIndex < 0)
                        {
                            MessageBox.Show("label " + ti.label.ToString() + " is not found! Maybe you have changed the qscript.",
                                "Information",
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Error);
                            return;
                        }
                        listBoxActionsSelect(actionsIndex);
                        this.trackBar.Value = stampIndex;
                    }
                    else
                    {
                        int lineNumber = getLineNumberFromLabel(action.label);
                        if (lineNumber > 0)
                        {
                            listBoxActionsSelect(lineNumber - 1);
                        }
                    }
                }
                else
                {
                    int lineNumber = getLineNumberFromLabel(action.label);
                    if (lineNumber > 0)
                    {
                        listBoxActionsSelect(lineNumber - 1);
                    }
                }
            }
            else
            {
                MessageBox.Show("Already the beginning of the execution.",
                    "Information",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
            }
        }

        private void toolStripButtonNextStep_Click(object sender, EventArgs e)
        {
            stopVideo();

            if (this.actions == null || this.listBoxActions.SelectedIndex < 0 || this.listBoxActions.SelectedIndex >= this.actions.Count ||
                this.timeStampList == null || this.qs_trace == null || this.videoFile == null)
                return;

            if (this.listBoxActions.SelectedIndices.Count > 1)
            {
                MessageBox.Show("Please choose only one line.",
                    "Information",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                return;
            }
            actionlinetype action = this.actions[this.listBoxActions.SelectedIndex];
            List<int> traceIndexes = getTraceNumberAtFrame(this.trackBar.Value);
            List<int> labelList = new List<int>();
            for (int i = 0; i < traceIndexes.Count; i++)
            {
                labelList.Add(this.qs_trace[traceIndexes[i]].label);
            }
            int labelIndex = labelList.IndexOf(action.label);
            if (traceIndexes.Count == 0 || labelIndex < 0)
            {
                int frameIndex = getFrameNumberFromLabel(action.label, this.trackBar.Value);
                if ((frameIndex >= 0) && (frameIndex < this.trackBar.Maximum))
                {
                    this.trackBar.Value = frameIndex;
                }
                else if (frameIndex >= this.trackBar.Maximum)
                {
                    //If already at the last OPCODE
                    MessageBox.Show("Already the end of the execution.",
                    "Information",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                }
                else
                {
                    this.trackBar.Value = 0;
                    int lineNumber = getLineNumberFromLabel(action.label);
                    listBoxActionsSelect(lineNumber + 1);
                }
                return;
            }
            else if (traceIndexes[labelIndex] < this.qs_trace.Count - 1)
            {
                if (!isNextActionNewAdded(action.label))
                {
                    TimeStampFileReader.traceInfo ti = this.qs_trace[traceIndexes[labelIndex] + 1];
                    int stampIndex = getStampNumberBeforeStamp(ti.time);
                    if (stampIndex >= 0)
                    {
                        int actionsIndex = getActionsIndexFromLabel(ti.label);
                        if (actionsIndex < 0)
                        {
                            MessageBox.Show("label " + ti.label.ToString() + " is not found! Maybe you have changed the qscript.",
                                "Information",
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Information);
                            return;
                        }
                        this.playUntil = stampIndex;
                        playVideo();
                        listBoxActionsSelect(actionsIndex);
                    }
                    else
                    {
                        MessageBox.Show("Impossible path. Contact developer please.",
                            "Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                    }
                }
                else
                {
                    int lineNumber = getLineNumberFromLabel(action.label);
                    listBoxActionsSelect(lineNumber + 1);
                }
            }
            else
            {
                MessageBox.Show("Already the end of the execution.",
                    "Information",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
            }
        }


        private void toolStripButtonPrevFrame_Click(object sender, EventArgs e)
        {
            if (this.trackBar.Value > 0)
                this.trackBar.Value--;
        }

        private void toolStripButtonNextFrame_Click(object sender, EventArgs e)
        {
            if (this.trackBar.Value < this.trackBar.Maximum)
                this.trackBar.Value++;
        }

        private void toolStripButtonPlayVideo_Click(object sender, EventArgs e)
        {
            if (this.videoFile == null)
                return;
            this.playUntil = this.trackBar.Maximum;
            playVideo();
        }

        private void QScriptEditor_Load(object sender, EventArgs e)
        {
            this.setTag(this);
        }

        private void QScriptEditor_FormClosing(object sender, FormClosingEventArgs e)
        {
            stopVideo();
            DaVinciAPI.NativeMethods.CloseVideo(vidInst);
            if (this.isScriptFileChanged)
            {
                DialogResult r = MessageBox.Show("QScript has been changed.\n Store to " + this.Text + "?", "QScriptEditor", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);
                if (r == DialogResult.Cancel)
                    e.Cancel = true;
                else if (r == DialogResult.Yes)
                {
                    saveToFiles();
                }
            }
        }

        private void loadAVIToolStripMenuItem_Click(object sender, EventArgs e)
        {
            // Check whether a qs file has been selected or not
            if (configList == null)
            {
                MessageBox.Show("Please select a QS file first!",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
                return;
            }
            if (aviOpenFileDialog.ShowDialog() == DialogResult.OK)
            {
                this.videoFile = aviOpenFileDialog.FileName;
                this.timeFile = Path.ChangeExtension(videoFile, ".qts");
                this.loadAVIToolStripMenuItem.ToolTipText = "Load Avi File Only. Current: " + this.videoFile;

                // Load the video file
                this.videoLoad();
            }
        }

        // resize each components based on the scale
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

        float scaleWidth = 1.0f;
        float scaleHeight = 1.0f;
        private void QScriptEditor_Resize(object sender, EventArgs e)
        {
            scaleWidth = (float)this.Width / (float)this.initialWidth;
            scaleHeight = (float)this.Height / (float)this.initialHeight;

            setControls(scaleWidth, scaleHeight, this);

            this.ImageBoxChangingHandler();
        }
    }
}
