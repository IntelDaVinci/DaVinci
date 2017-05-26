namespace DaVinci
{
    partial class FPSTestForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(FPSTestForm));
            this.groupBoxListActions = new System.Windows.Forms.GroupBox();
            this.button2 = new System.Windows.Forms.Button();
            this.button3 = new System.Windows.Forms.Button();
            this.label9 = new System.Windows.Forms.Label();
            this.label16 = new System.Windows.Forms.Label();
            this.button1 = new System.Windows.Forms.Button();
            this.label8 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.label15 = new System.Windows.Forms.Label();
            this.label14 = new System.Windows.Forms.Label();
            this.buttonSaveAs = new System.Windows.Forms.Button();
            this.label13 = new System.Windows.Forms.Label();
            this.label12 = new System.Windows.Forms.Label();
            this.button4 = new System.Windows.Forms.Button();
            this.label11 = new System.Windows.Forms.Label();
            this.label10 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.buttonBrowse = new System.Windows.Forms.Button();
            this.label2 = new System.Windows.Forms.Label();
            this.textBoxQSFileName = new System.Windows.Forms.TextBox();
            this.launchTimeImageBox = new DaVinci.QScriptImageBox(this.components);
            this.trackBar = new System.Windows.Forms.TrackBar();
            this.toolStripVideo = new System.Windows.Forms.ToolStrip();
            this.toolStripButtonPrevFrame = new System.Windows.Forms.ToolStripButton();
            this.toolStripButtonNextFrame = new System.Windows.Forms.ToolStripButton();
            this.toolStripButtonPlayVideo = new System.Windows.Forms.ToolStripButton();
            this.toolStripButtonShot = new System.Windows.Forms.ToolStripButton();
            this.groupBoxListActions.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.launchTimeImageBox)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar)).BeginInit();
            this.toolStripVideo.SuspendLayout();
            this.SuspendLayout();
            // 
            // groupBoxListActions
            // 
            this.groupBoxListActions.Controls.Add(this.button2);
            this.groupBoxListActions.Controls.Add(this.button3);
            this.groupBoxListActions.Controls.Add(this.label9);
            this.groupBoxListActions.Controls.Add(this.label16);
            this.groupBoxListActions.Controls.Add(this.button1);
            this.groupBoxListActions.Controls.Add(this.label8);
            this.groupBoxListActions.Controls.Add(this.label6);
            this.groupBoxListActions.Controls.Add(this.label7);
            this.groupBoxListActions.Controls.Add(this.textBox1);
            this.groupBoxListActions.Controls.Add(this.label15);
            this.groupBoxListActions.Controls.Add(this.label14);
            this.groupBoxListActions.Controls.Add(this.buttonSaveAs);
            this.groupBoxListActions.Controls.Add(this.label13);
            this.groupBoxListActions.Controls.Add(this.label12);
            this.groupBoxListActions.Controls.Add(this.button4);
            this.groupBoxListActions.Controls.Add(this.label11);
            this.groupBoxListActions.Controls.Add(this.label10);
            this.groupBoxListActions.Controls.Add(this.label5);
            this.groupBoxListActions.Controls.Add(this.label4);
            this.groupBoxListActions.Controls.Add(this.label1);
            this.groupBoxListActions.Controls.Add(this.label3);
            this.groupBoxListActions.Controls.Add(this.buttonBrowse);
            this.groupBoxListActions.Controls.Add(this.label2);
            this.groupBoxListActions.Controls.Add(this.textBoxQSFileName);
            this.groupBoxListActions.Font = new System.Drawing.Font("Segoe UI", 15.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBoxListActions.Location = new System.Drawing.Point(12, 21);
            this.groupBoxListActions.Name = "groupBoxListActions";
            this.groupBoxListActions.Size = new System.Drawing.Size(787, 604);
            this.groupBoxListActions.TabIndex = 35;
            this.groupBoxListActions.TabStop = false;
            this.groupBoxListActions.Text = "Step by Step Instructions";
            // 
            // button2
            // 
            this.button2.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.button2.Location = new System.Drawing.Point(347, 344);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(145, 31);
            this.button2.TabIndex = 29;
            this.button2.Text = "Select as image";
            this.button2.UseVisualStyleBackColor = true;
            this.button2.Click += new System.EventHandler(this.buttonSelectFrame_Click);
            // 
            // button3
            // 
            this.button3.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.button3.Location = new System.Drawing.Point(75, 344);
            this.button3.Name = "button3";
            this.button3.Size = new System.Drawing.Size(162, 31);
            this.button3.TabIndex = 28;
            this.button3.Text = "Select as timestamp";
            this.button3.UseVisualStyleBackColor = true;
            this.button3.Click += new System.EventHandler(this.buttonSelectFrame_Click_1);
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Font = new System.Drawing.Font("Segoe UI", 9.75F, System.Drawing.FontStyle.Italic, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label9.Location = new System.Drawing.Point(85, 315);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(340, 17);
            this.label9.TabIndex = 27;
            this.label9.Text = "Select a frame from right video control when measure stops";
            // 
            // label16
            // 
            this.label16.AutoSize = true;
            this.label16.Font = new System.Drawing.Font("Segoe UI", 11.25F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Italic))), System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label16.Location = new System.Drawing.Point(22, 312);
            this.label16.Name = "label16";
            this.label16.Size = new System.Drawing.Size(57, 20);
            this.label16.TabIndex = 26;
            this.label16.Text = "Step3. ";
            // 
            // button1
            // 
            this.button1.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.button1.Location = new System.Drawing.Point(347, 246);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(145, 31);
            this.button1.TabIndex = 25;
            this.button1.Text = "Select as image";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.buttonSelectFrame_Click);
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Font = new System.Drawing.Font("Segoe UI", 11.25F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Italic))), System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label8.Location = new System.Drawing.Point(330, 157);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(29, 20);
            this.label8.TabIndex = 24;
            this.label8.Text = ".qs";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Font = new System.Drawing.Font("Segoe UI", 11.25F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Italic))), System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label6.Location = new System.Drawing.Point(330, 470);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(29, 20);
            this.label6.TabIndex = 23;
            this.label6.Text = ".qs";
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Font = new System.Drawing.Font("Segoe UI", 11.25F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Italic))), System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label7.Location = new System.Drawing.Point(71, 466);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(83, 20);
            this.label7.TabIndex = 22;
            this.label7.Text = "File name:";
            // 
            // textBox1
            // 
            this.textBox1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.textBox1.Location = new System.Drawing.Point(161, 455);
            this.textBox1.Name = "textBox1";
            this.textBox1.Size = new System.Drawing.Size(163, 35);
            this.textBox1.TabIndex = 21;
            // 
            // label15
            // 
            this.label15.AutoSize = true;
            this.label15.Font = new System.Drawing.Font("Segoe UI", 9.75F, System.Drawing.FontStyle.Italic, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label15.Location = new System.Drawing.Point(85, 524);
            this.label15.Name = "label15";
            this.label15.Size = new System.Drawing.Size(634, 17);
            this.label15.TabIndex = 20;
            this.label15.Text = "Please go back to Main Window and then select the saved script, and then click \"R" +
    "un\" button to start the FPS test";
            // 
            // label14
            // 
            this.label14.AutoSize = true;
            this.label14.Font = new System.Drawing.Font("Segoe UI", 11.25F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Italic))), System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label14.Location = new System.Drawing.Point(22, 521);
            this.label14.Name = "label14";
            this.label14.Size = new System.Drawing.Size(57, 20);
            this.label14.TabIndex = 19;
            this.label14.Text = "Step5. ";
            // 
            // buttonSaveAs
            // 
            this.buttonSaveAs.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonSaveAs.Location = new System.Drawing.Point(378, 456);
            this.buttonSaveAs.Name = "buttonSaveAs";
            this.buttonSaveAs.Size = new System.Drawing.Size(80, 38);
            this.buttonSaveAs.TabIndex = 18;
            this.buttonSaveAs.Text = "Save";
            this.buttonSaveAs.UseVisualStyleBackColor = true;
            this.buttonSaveAs.Click += new System.EventHandler(this.buttonSaveAs_Click);
            // 
            // label13
            // 
            this.label13.AutoSize = true;
            this.label13.Font = new System.Drawing.Font("Segoe UI", 9.75F, System.Drawing.FontStyle.Italic, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label13.Location = new System.Drawing.Point(85, 413);
            this.label13.Name = "label13";
            this.label13.Size = new System.Drawing.Size(247, 17);
            this.label13.TabIndex = 17;
            this.label13.Text = "Save script with selected frame information";
            // 
            // label12
            // 
            this.label12.AutoSize = true;
            this.label12.Font = new System.Drawing.Font("Segoe UI", 11.25F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Italic))), System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label12.Location = new System.Drawing.Point(22, 410);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(57, 20);
            this.label12.TabIndex = 16;
            this.label12.Text = "Step4. ";
            // 
            // button4
            // 
            this.button4.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.button4.Location = new System.Drawing.Point(75, 246);
            this.button4.Name = "button4";
            this.button4.Size = new System.Drawing.Size(162, 31);
            this.button4.TabIndex = 15;
            this.button4.Text = "Select as timestamp";
            this.button4.UseVisualStyleBackColor = true;
            this.button4.Click += new System.EventHandler(this.buttonSelectFrame_Click_1);
            // 
            // label11
            // 
            this.label11.AutoSize = true;
            this.label11.Font = new System.Drawing.Font("Segoe UI", 9.75F, System.Drawing.FontStyle.Italic, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label11.Location = new System.Drawing.Point(85, 217);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(342, 17);
            this.label11.TabIndex = 14;
            this.label11.Text = "Select a frame from right video control when measure starts";
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Font = new System.Drawing.Font("Segoe UI", 11.25F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Italic))), System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label10.Location = new System.Drawing.Point(22, 214);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(57, 20);
            this.label10.TabIndex = 13;
            this.label10.Text = "Step2. ";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Font = new System.Drawing.Font("Segoe UI", 11.25F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Italic))), System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label5.Location = new System.Drawing.Point(71, 153);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(83, 20);
            this.label5.TabIndex = 8;
            this.label5.Text = "File name:";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Font = new System.Drawing.Font("Segoe UI", 9.75F, System.Drawing.FontStyle.Italic, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label4.Location = new System.Drawing.Point(73, 112);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(241, 17);
            this.label4.TabIndex = 7;
            this.label4.Text = "Please select a test file (.qs) from RnR Test";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Segoe UI", 11.25F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Italic))), System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(22, 110);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(57, 20);
            this.label1.TabIndex = 6;
            this.label1.Text = "Step1. ";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Segoe UI", 11.25F, System.Drawing.FontStyle.Italic, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(6, 75);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(331, 20);
            this.label3.TabIndex = 5;
            this.label3.Text = "Embed FPS measure test into a comman R&&R test ";
            // 
            // buttonBrowse
            // 
            this.buttonBrowse.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonBrowse.Location = new System.Drawing.Point(378, 143);
            this.buttonBrowse.Name = "buttonBrowse";
            this.buttonBrowse.Size = new System.Drawing.Size(80, 38);
            this.buttonBrowse.TabIndex = 4;
            this.buttonBrowse.Text = "Browse";
            this.buttonBrowse.UseVisualStyleBackColor = true;
            this.buttonBrowse.Click += new System.EventHandler(this.buttonBrowse_Click);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Font = new System.Drawing.Font("Segoe UI", 11.25F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Italic))), System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label2.Location = new System.Drawing.Point(6, 55);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(183, 20);
            this.label2.TabIndex = 2;
            this.label2.Text = "Create FPS Measure Test";
            // 
            // textBoxQSFileName
            // 
            this.textBoxQSFileName.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.textBoxQSFileName.Location = new System.Drawing.Point(161, 142);
            this.textBoxQSFileName.Name = "textBoxQSFileName";
            this.textBoxQSFileName.ReadOnly = true;
            this.textBoxQSFileName.Size = new System.Drawing.Size(163, 35);
            this.textBoxQSFileName.TabIndex = 1;
            // 
            // launchTimeImageBox
            // 
            this.launchTimeImageBox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.launchTimeImageBox.IsEditingPicture = false;
            this.launchTimeImageBox.Location = new System.Drawing.Point(812, 21);
            this.launchTimeImageBox.Name = "launchTimeImageBox";
            this.launchTimeImageBox.Size = new System.Drawing.Size(301, 506);
            this.launchTimeImageBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
            this.launchTimeImageBox.TabIndex = 36;
            this.launchTimeImageBox.TabStop = false;
            this.launchTimeImageBox.Text = "ImageBox";
            this.launchTimeImageBox.MouseDown += new System.Windows.Forms.MouseEventHandler(this.launchTimeImageBox_MouseDown);
            this.launchTimeImageBox.MouseMove += new System.Windows.Forms.MouseEventHandler(this.launchTimeImageBox_MouseMove);
            this.launchTimeImageBox.MouseUp += new System.Windows.Forms.MouseEventHandler(this.launchTimeImageBox_MouseUp);
            // 
            // trackBar
            // 
            this.trackBar.AutoSize = false;
            this.trackBar.Location = new System.Drawing.Point(812, 544);
            this.trackBar.Name = "trackBar";
            this.trackBar.Size = new System.Drawing.Size(301, 30);
            this.trackBar.TabIndex = 39;
            this.trackBar.ValueChanged += new System.EventHandler(this.launchTimeTrackBar_ValueChanged);
            // 
            // toolStripVideo
            // 
            this.toolStripVideo.Dock = System.Windows.Forms.DockStyle.None;
            this.toolStripVideo.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripButtonPrevFrame,
            this.toolStripButtonNextFrame,
            this.toolStripButtonPlayVideo,
            this.toolStripButtonShot});
            this.toolStripVideo.Location = new System.Drawing.Point(812, 590);
            this.toolStripVideo.Name = "toolStripVideo";
            this.toolStripVideo.Size = new System.Drawing.Size(104, 25);
            this.toolStripVideo.TabIndex = 24;
            this.toolStripVideo.Text = "toolStrip";
            // 
            // toolStripButtonPrevFrame
            // 
            this.toolStripButtonPrevFrame.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolStripButtonPrevFrame.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButtonPrevFrame.Image")));
            this.toolStripButtonPrevFrame.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripButtonPrevFrame.Name = "toolStripButtonPrevFrame";
            this.toolStripButtonPrevFrame.Size = new System.Drawing.Size(23, 22);
            this.toolStripButtonPrevFrame.Text = "Prev";
            this.toolStripButtonPrevFrame.ToolTipText = "Previous Frame";
            this.toolStripButtonPrevFrame.Click += new System.EventHandler(this.toolStripButtonPrevFrame_Click);
            // 
            // toolStripButtonNextFrame
            // 
            this.toolStripButtonNextFrame.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolStripButtonNextFrame.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButtonNextFrame.Image")));
            this.toolStripButtonNextFrame.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripButtonNextFrame.Name = "toolStripButtonNextFrame";
            this.toolStripButtonNextFrame.Size = new System.Drawing.Size(23, 22);
            this.toolStripButtonNextFrame.Text = "Next";
            this.toolStripButtonNextFrame.ToolTipText = "Next Frame";
            this.toolStripButtonNextFrame.Click += new System.EventHandler(this.toolStripButtonNextFrame_Click);
            // 
            // toolStripButtonPlayVideo
            // 
            this.toolStripButtonPlayVideo.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolStripButtonPlayVideo.Image = global::DaVinci.Properties.Resources.button_play;
            this.toolStripButtonPlayVideo.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripButtonPlayVideo.Name = "toolStripButtonPlayVideo";
            this.toolStripButtonPlayVideo.Size = new System.Drawing.Size(23, 22);
            this.toolStripButtonPlayVideo.Text = "Play";
            this.toolStripButtonPlayVideo.ToolTipText = "Play QScript Video";
            this.toolStripButtonPlayVideo.Click += new System.EventHandler(this.toolStripButtonPlayVideo_Click);
            // 
            // toolStripButtonShot
            // 
            this.toolStripButtonShot.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolStripButtonShot.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButtonShot.Image")));
            this.toolStripButtonShot.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripButtonShot.Name = "toolStripButtonShot";
            this.toolStripButtonShot.Size = new System.Drawing.Size(23, 22);
            this.toolStripButtonShot.Text = "toolStripButtonPictureShot";
            this.toolStripButtonShot.ToolTipText = "Picture Shot";
            this.toolStripButtonShot.Click += new System.EventHandler(this.toolStripButtonShot_Click);
            // 
            // FPSTestForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1125, 664);
            this.Controls.Add(this.toolStripVideo);
            this.Controls.Add(this.trackBar);
            this.Controls.Add(this.launchTimeImageBox);
            this.Controls.Add(this.groupBoxListActions);
            this.MaximizeBox = false;
            this.Name = "FPSTestForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "FPS Test Wizard";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.FPSTestForm_FormClosing);
            this.groupBoxListActions.ResumeLayout(false);
            this.groupBoxListActions.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.launchTimeImageBox)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar)).EndInit();
            this.toolStripVideo.ResumeLayout(false);
            this.toolStripVideo.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBoxListActions;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Button buttonBrowse;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox textBoxQSFileName;
        private QScriptImageBox launchTimeImageBox;
        private System.Windows.Forms.TrackBar trackBar;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.Label label11;
        private System.Windows.Forms.Button button4;
        private System.Windows.Forms.Label label12;
        private System.Windows.Forms.Label label13;
        private System.Windows.Forms.Button buttonSaveAs;
        private System.Windows.Forms.Label label14;
        private System.Windows.Forms.Label label15;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.TextBox textBox1;
        private System.Windows.Forms.ToolStrip toolStripVideo;
        private System.Windows.Forms.ToolStripButton toolStripButtonPrevFrame;
        private System.Windows.Forms.ToolStripButton toolStripButtonNextFrame;
        private System.Windows.Forms.ToolStripButton toolStripButtonPlayVideo;
        private System.Windows.Forms.ToolStripButton toolStripButtonShot;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.Button button2;
        private System.Windows.Forms.Button button3;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.Label label16;
        private System.Windows.Forms.Button button1;
    }
}