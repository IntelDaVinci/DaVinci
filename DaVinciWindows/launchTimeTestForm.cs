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
using System.Drawing.Imaging;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using System.Runtime.InteropServices;
using System.Timers;

namespace DaVinci
{
    /// <summary>
    /// 
    /// </summary>
    public partial class launchTimeTestForm : Form
    {
        /// <summary>
        /// 
        /// </summary>
        public launchTimeTestForm()
        {
            InitializeComponent();
            this.ImageBoxChangingHandler();
        }

        /// <summary>
        /// 
        /// </summary>
        public int displayVideoImageLock = 0;
        private System.IntPtr vidInst;

        private void ShowImage(DaVinciAPI.ImageInfo img)
        {
            if (CreateTestSelect.launchTimeForm != null)
            {
                if (CreateTestSelect.launchTimeForm.launchTimeImageBox != null
                    && CreateTestSelect.launchTimeForm.launchTimeImageBox.InvokeRequired
                    && !CreateTestSelect.launchTimeForm.launchTimeImageBox.IsDisposed)
                {
                    if (Interlocked.CompareExchange(ref displayVideoImageLock, 1, 0) == 0)
                    {
                        CreateTestSelect.launchTimeForm.launchTimeImageBox.BeginInvoke((Action<DaVinciAPI.ImageInfo>)ShowImage, img);
                    }
                    else
                    {
                        DaVinciAPI.NativeMethods.ReleaseImage(ref img);
                    }
                }
                else
                {
                    if (CreateTestSelect.launchTimeForm.launchTimeImageBox != null && CreateTestSelect.launchTimeForm.launchTimeImageBox.IsDisposed == false)
                    {
                        CreateTestSelect.launchTimeForm.launchTimeImageBox.Image = img;
                    }
                    Interlocked.Exchange(ref displayVideoImageLock, 0);
                }
            }
        }

        private bool isImgBoxVertical = true;
        private void DisplayImgLaunchTimeVideo(ref DaVinciAPI.ImageInfo img)
        {
            DaVinciAPI.ImageInfo showImg;
            if (isImgBoxVertical)
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
                //this.timeStampList = null;
                //this.qs_trace = null;
                int initFrameIndex = 0;
                int frameCount = 0;
                vidInst = DaVinciAPI.NativeMethods.OpenVideo(videoFile, ref frameCount);
                try
                {
                    DaVinciAPI.ImageEventHandler qScriptImageEventHandler = new DaVinciAPI.ImageEventHandler(DisplayImgLaunchTimeVideo);
                    GCHandle gcImageEventHandler = GCHandle.Alloc(qScriptImageEventHandler);
                    DaVinciAPI.NativeMethods.SetScriptVideoImgHandler(qScriptImageEventHandler);
                    DaVinciAPI.NativeMethods.ShowVideoFrame(vidInst, initFrameIndex);

                    if (frameCount >= 1)
                    {
                        this.launchTimeTrackBar.Maximum = frameCount - 1;
                    }
                    else
                    {
                        this.launchTimeTrackBar.Maximum = 0;
                    }
                    this.launchTimeTrackBar.Value = 0;
                    this.launchTimeTrackBar.Enabled = true;
                    this.toolStripVideo.Enabled = true;
                    this.buttonSelectFrame.Enabled = true;
                }
                catch
                {
                    MessageBox.Show("Video file \"" + videoFile + "\" cannot be loaded!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    this.launchTimeTrackBar.Enabled = false;
                    this.toolStripVideo.Enabled = false;
                    this.buttonSelectFrame.Enabled = false;
                    return;
                }
            }
            else
            {
                MessageBox.Show("Video file \"" + videoFile + "\" doesn't exist!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                this.launchTimeTrackBar.Enabled = false;
                this.toolStripVideo.Enabled = false;
                this.buttonSelectFrame.Enabled = false;
                return;
            }
        }
     
        private void buttonBrowse_Click(object sender, EventArgs e)
        {
            using (OpenFileDialog dialog = new OpenFileDialog())
            {
                dialog.Filter = "DaVinci Test File|*.qs";
                dialog.FilterIndex = 1;
                dialog.Title = "Open a DaVinci Test File and Video";
                dialog.RestoreDirectory = true;
                if (dialog.ShowDialog() == DialogResult.OK && dialog.FileName != "")
                {
                    try
                    {
                        // Load Script file
                        FileInfo scriptFileInfo = new FileInfo(dialog.FileName);
                        textBoxQSFileName.Text = Path.GetFileNameWithoutExtension(scriptFileInfo.FullName);
                        loadQScript(scriptFileInfo.FullName);
                    }
                    catch (Exception)
                    {
                        MessageBox.Show("Unable to load the test file or the Video!", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
            }
        }

        private string scriptFile;
        private string videoFile;
        private string timeFile;

        private string GetAVIFile()
        {
            return videoFile;
        }

        private string GetQSFile()
        {
            return scriptFile;
        }

        // Script configuration
        private class conftype
        {
            public string name;
            public string value;
            public conftype(string s)
            {
                this.name = s;
                this.value = "";
            }
            public override string ToString()
            {
                return (this.name + "=" + this.value);
            }
        };

        /// <summary>
        /// Script actions lines
        /// </summary>
        public class actionlinetype
        {
            /// <summary>
            /// 
            /// </summary>
            public int label;
            /// <summary>
            /// 
            /// </summary>
            public double time;
            /// <summary>
            /// 
            /// </summary>
            public string name;
            /// <summary>
            /// 
            /// </summary>
            public string[] parameters;
            /// <summary>
            /// 
            /// </summary>
            public actionlinetype()
            {
                label = -1;
                time = 0;
                name = "";
            }
            /// <summary>
            /// 
            /// </summary>
            /// <returns></returns>
            public override string ToString()
            {
                string result = "";
                if (this.label != -1)
                    result += this.label.ToString();
                string timeString = ((int)this.time).ToString();
                for (int i = result.Length; i < 12 - timeString.Length; i++)
                    result += " ";
                result += " ";
                result += timeString;
                result += ": ";
                result += this.name;
                if (this.parameters != null)
                {
                    for (int i = 0; i < this.parameters.Length; i++)
                    {
                        result += " ";
                        result += this.parameters[i];
                    }
                }
                return result;
            }
        }
        private System.Collections.Generic.List<conftype> configList;
        private System.Collections.Generic.List<actionlinetype> actions;

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
                        Console.WriteLine("Warning: Label " + label.ToString() + " is duplicated.");
                        result = false;
                    }
            }
            return result;
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
                this.configList = new List<conftype>();
                while (line < qs_lines.Length && qs_lines[line].IndexOf("[Events and Actions]") == -1)
                {
                    conftype conf = parseConfigLine(qs_lines[line]);
                    if (conf != null)
                        this.configList.Add(conf);
                    else if (qs_lines[line].Length > 0)
                        Console.WriteLine("Unrecognized config line: " + qs_lines[line]);

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
                        Console.WriteLine("Unrecognized action in line " + (line + 1).ToString() + ", which will be removed if you save the current scripts!");
                    }
                    line++;
                }

                checkQScript(this.configList, this.actions);
                //flashListBoxActions();
                //if (this.actions.Count > 0)
                //    listBoxActionsSelect(0);
            }
        }

        /// <summary>
        /// Flag to check whether QS file is changed
        /// </summary>
        public bool isScriptFileChanged = false;

        /// <summary>
        /// Load QS file
        /// </summary>
        /// <param name="scriptName">QS file name</param>
        private void loadQScript(string scriptName)
        {
            this.scriptFile = scriptName;
            this.videoFile = Path.ChangeExtension(scriptFile, ".avi");
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

        // Check whether a label is already used in the current scripts.
        private bool isLabelInScript(int l)
        {
            for (int i = 0; i < this.actions.Count; i++)
                if (this.actions[i].label == l)
                    return true;
            return false;
        }

        /// <summary>
        /// Get N-lines unused label
        /// </summary>
        /// <param name="n"></param>
        /// <returns></returns>
        public int get_consecutive_UnusedLabel(int n)
        {
            if (this.actions == null)
                return 0;
            int label = 1, j;
            while (isLabelInScript(label))
            {
                for (j = 1; j < n + 2; j++)
                {
                    if (isLabelInScript(label + j))
                    {
                        label += j;
                        break;
                    }
                }

                if (j == n + 2)
                {
                    label++;
                    break;
                }
            }
            return label;
        }

        private void buttonSaveAs_Click(object sender, EventArgs e)
        {
            String avifilename = GetAVIFile();
            String qsfilename = GetQSFile();
            if (String.IsNullOrEmpty(avifilename) && String.IsNullOrEmpty(qsfilename))
            {
                MessageBox.Show("Cannot find QS file or AVI file.",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
                return;
            }

            String filename1 = "", filename2 = "";

            if (!String.IsNullOrEmpty(avifilename))
            {
                filename1 = Path.GetFileNameWithoutExtension(avifilename);
            }

            if (!String.IsNullOrEmpty(qsfilename))
            {
                filename2 = Path.GetFileNameWithoutExtension(qsfilename);
            }

            if (filename1 != "" && filename2 != "" && filename1 != filename2)
            {
                MessageBox.Show("Inconsistent filename for QS and AVI file.",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
                return;
            }

            qsfilename = filename1 + ".qs";
            String qsfilepath = Path.GetDirectoryName(avifilename) + "\\" + filename1 + ".qs";
            String qsLaunchTimeFilePath = "";
            if ((textBoxQSSaveName.Text != null) && (textBoxQSSaveName.Text != ""))
            {
                qsLaunchTimeFilePath = Path.GetDirectoryName(avifilename) + "\\" + textBoxQSSaveName.Text + ".qs";
            }
            else
            {
                MessageBox.Show("Script Name cannot be empty!",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Warning);
                return;
            }

            if (File.Exists(qsfilepath))
            {
                loadQScript(qsfilepath);
            }
            else
            {
                MessageBox.Show(qsfilename + " does not exist!",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
                return;
            }

            if (CreateTestSelect.launchTimeForm.isScriptFileChanged)
            {
                MessageBox.Show(qsfilename + " has been changed before launch time wizard; please save or cancel wizard first!",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
                return;
            }

            int lablenumber = 500;
            lablenumber = get_consecutive_UnusedLabel(3);

            String[] lines = System.IO.File.ReadAllLines(qsfilepath);
            String changed_lines = "";
            bool flag1 = false;
            String image_file_name = Path.GetFileName(picFileName);
            
            foreach (String line in lines)
            {
                if (line == "[Events and Actions]")
                {
                    changed_lines += line + System.Environment.NewLine;
                    flag1 = true;
                    continue;
                }

                if (flag1)
                {
                    //if (line.Contains("OPCODE_START_APP"))
                    //{
                        changed_lines += (lablenumber) + "        0.0000: OPCODE_CONCURRENT_IF_MATCH " + image_file_name + " 2 1 " + System.Environment.NewLine;
                        changed_lines += (++lablenumber) + "        0.0000: OPCODE_MESSAGE Measuring app launch time" + System.Environment.NewLine;
                        changed_lines += (++lablenumber) + "        0.0000: OPCODE_IMAGE_MATCHED_TIME " + image_file_name + " " + System.Environment.NewLine;
                        changed_lines += line + System.Environment.NewLine;
                        flag1 = false;
                    //}
                    
                }
                else
                {
                    changed_lines += line + System.Environment.NewLine;
                }
            }

            try
            {
                //Check the qs file name validity
                if (TestUtil.isValidFileName(textBoxQSSaveName.Text))
                {
                    using (FileStream qs_fs = new FileStream(qsLaunchTimeFilePath, FileMode.Create))
                    using (StreamWriter qs_sw = new StreamWriter(qs_fs))
                    {
                        qs_sw.Write(changed_lines);
                        qs_sw.Flush();
                        MessageBox.Show("Script has been saved!",
                            "Info",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Information);
                        MainWindow_V.form.RefreshTestProject();
                    }
                }
            }
            catch (IOException)
            {
                MessageBox.Show("Cannot save QScript for launch time.",
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
            CreateTestSelect.launchTimeForm.loadQScript(qsfilepath);
        }

        private void launchTimeTrackBar_ValueChanged(object sender, EventArgs e)
        {
            if (!File.Exists(CreateTestSelect.launchTimeForm.GetAVIFile()))
                return;

            // change image frame
            DaVinciAPI.ImageEventHandler qScriptImageEventHandler = new DaVinciAPI.ImageEventHandler(DisplayImgLaunchTimeVideo);
            GCHandle gcImageEventHandler = GCHandle.Alloc(qScriptImageEventHandler);
            DaVinciAPI.NativeMethods.SetScriptVideoImgHandler(qScriptImageEventHandler);
            DaVinciAPI.NativeMethods.ShowVideoFrame(vidInst, this.launchTimeTrackBar.Value);
        }

        // ImageBox operation
        private bool isMouseDown = false;
        private bool isCuttingPicture = false;
        private System.Drawing.Rectangle mouseRect = System.Drawing.Rectangle.Empty;

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

        private void launchTimeImageBox_MouseDown(object sender, MouseEventArgs e)
        {
            isMouseDown = true;
            if (this.isCuttingPicture)
            {
                //Cursor.Clip = this.RectangleToScreen(new Rectangle(0, 0, ClientSize.Width, ClientSize.Height));
                mouseRect = new Rectangle(e.X, e.Y, 0, 0);
            }
        }

        private void DrawRectangleOnImageBox()
        {
            Rectangle rect = this.launchTimeImageBox.RectangleToScreen(mouseRect);
            ControlPaint.DrawReversibleFrame(rect, DaVinciCommon.IntelWhite, FrameStyle.Dashed);
        }

        private string GetVideoFileDir()
        {
            return Path.GetDirectoryName(GetAVIFile());
        }

        private string GetPictureShotFileName(string dirname = "Video")
        {
            if (dirname == "Video")
                dirname = GetVideoFileDir();
            string picFileNamePre = dirname + "\\" + Path.GetFileNameWithoutExtension(GetQSFile()) + "_PictureShot";
            int picFileNameIndex = 1;
            string picFilePath = picFileNamePre + picFileNameIndex.ToString() + ".png";
            while (File.Exists(picFilePath))
            {
                picFileNameIndex++;
                picFilePath = picFileNamePre + picFileNameIndex.ToString() + ".png";
            }
            return picFilePath;
        }

        DaVinciAPI.ImageInfo queryFrame;
        private void GetImgData(ref DaVinciAPI.ImageInfo img)
        {
            queryFrame = img;
        }

        private DaVinciAPI.ImageInfo getCurrentFrame()
        {
            if (this.videoFile == null)
            {
                MessageBox.Show("No video file loaded!", 
                    "Warning", 
                    MessageBoxButtons.OK, 
                    MessageBoxIcon.Exclamation);
                return queryFrame;
            }
            DaVinciAPI.ImageEventHandler queryFrameHandler = new DaVinciAPI.ImageEventHandler(GetImgData);
            GCHandle gcImageEventHandler = GCHandle.Alloc(queryFrameHandler);
            DaVinciAPI.NativeMethods.SetScriptVideoImgHandler(queryFrameHandler);
            DaVinciAPI.NativeMethods.ShowVideoFrame(vidInst, this.launchTimeTrackBar.Value);

            if (queryFrame.iplImage == null || queryFrame.mat == null)
            {
                MessageBox.Show("Invalid frame",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
                return (queryFrame);
            }
            else
            {
                return (queryFrame);
            }
        }

        private String picFileName = null;
        private void launchTimeImageBox_MouseUp(object sender, MouseEventArgs e)
        {
            //Cursor.Clip = Rectangle.Empty;
            this.isMouseDown = false;
            if (this.isCuttingPicture)
            {
                DrawRectangleOnImageBox();

                // Save the picture.
                if ((this.launchTimeImageBox.Image.iplImage) != null && (this.launchTimeImageBox.Image.mat != null))
                {
                    DaVinciAPI.ImageInfo current = getCurrentFrame();
                    if (current.iplImage == null || current.mat == null)
                    {
                        return;
                    }

                    picFileName = GetPictureShotFileName();
                    if (current.iplImage != IntPtr.Zero)
                    {
                        Bitmap imgSave = DaVinciCommon.ConvertToBitmap(current.iplImage);
                        imgSave.Save(picFileName, ImageFormat.Png);
                        MessageBox.Show(picFileName + " has been saved!", "Information",MessageBoxButtons.OK, MessageBoxIcon.Information);
                    }
                }
                mouseRect = Rectangle.Empty;
                this.isCuttingPicture = false;
                this.Cursor = Cursors.Default;
            }
        }

        private void buttonSelectFrame_Click(object sender, EventArgs e)
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

        // The image may not fill all the image boxes.
        int imageHeightInImageBox;
        int imageWidthInImageBox;

        private void ImageBoxChangingHandler()
        {
            this.imageHeightInImageBox = this.launchTimeImageBox.Height - 3;
            this.imageWidthInImageBox = this.launchTimeImageBox.Width - 3;
        }

        private void normalizeCoordinateOnImageBox(int imageBox_X, int imageBox_Y, out int normalized_X, out int normalized_Y)
        {
            normalized_X = (int)(imageBox_X * 4096 / (double)this.imageWidthInImageBox);
            normalized_Y = (int)(imageBox_Y * 4096 / (double)this.imageHeightInImageBox);
        }

        private void ResizeToRectangle(Point p)
        {
            DrawRectangleOnImageBox();
            mouseRect.Width = p.X - mouseRect.Left;
            mouseRect.Height = p.Y - mouseRect.Top;
            DrawRectangleOnImageBox();
        }

        private void launchTimeImageBox_MouseMove(object sender, MouseEventArgs e)
        {
            int x_pos, y_pos;
            normalizeCoordinateOnImageBox(e.X, e.Y, out x_pos, out y_pos);

            if (isMouseDown && isCuttingPicture)
            {
                ResizeToRectangle(e.Location);
            }
        }

        private void toolStripButtonPrevFrame_Click(object sender, EventArgs e)
        {
            if (this.launchTimeTrackBar.Value > 0)
                this.launchTimeTrackBar.Value--;
        }

        private void toolStripButtonNextFrame_Click(object sender, EventArgs e)
        {
            if (this.launchTimeTrackBar.Value < this.launchTimeTrackBar.Maximum)
                this.launchTimeTrackBar.Value++;
        }

        // Video playing
        private System.Timers.Timer videoTimer;
        private bool videoRunning = false;
        private int playUntil = 0;

        private void toolStripButtonPlayVideo_Click(object sender, EventArgs e)
        {
            if (this.videoFile == null)
                return;
            this.playUntil = this.launchTimeTrackBar.Maximum;
            playVideo();
        }


        private void timerBackToControl(object sender, System.Timers.ElapsedEventArgs e)
        {
            if (!this.videoRunning) return;

            if (TestUtil.AddControlPropertyValueUntil(this.launchTimeTrackBar, "Value", this.playUntil))
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
            else if (this.launchTimeTrackBar.Value < this.playUntil)
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

        private void stopVideo()
        {
            if (this.videoRunning)
            {
                this.videoRunning = false;
                this.toolStripButtonPlayVideo.Image = global::DaVinci.Properties.Resources.button_play;
            }
        }

        private void launchTimeTestForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            stopVideo();
            DaVinciAPI.NativeMethods.CloseVideo(vidInst);
        }
    }
}
