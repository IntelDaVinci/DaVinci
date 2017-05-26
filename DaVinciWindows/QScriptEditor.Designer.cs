namespace DaVinci
{
    partial class QScriptEditor
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Flag to check whether QS file is changed
        /// </summary>
        public bool isScriptFileChanged = false;

        // Time stamp list
        private System.Collections.Generic.List<double> timeStampList;
        private System.Collections.Generic.List<TimeStampFileReader.traceInfo> qs_trace;

        private string videoFile;
        private string timeFile;

        private string scriptFile;

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
            /// <param name="actionName"></param>
            public actionlinetype(string actionName)
            {
                label = -1;
                time = 0;
                name = actionName;
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

        private System.Collections.Generic.List<actionlinetype> actions;

        //private System.Collections.Generic.List<actionlinetype> oldActions;

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
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle2 = new System.Windows.Forms.DataGridViewCellStyle();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(QScriptEditor));
            this.listBoxActions = new System.Windows.Forms.ListBox();
            this.dataGridViewActionProperty = new System.Windows.Forms.DataGridView();
            this.ColumnProperty = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.ColumnValue = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.textBoxLog = new System.Windows.Forms.TextBox();
            this.groupBoxListActions = new System.Windows.Forms.GroupBox();
            this.toolStripAction = new System.Windows.Forms.ToolStrip();
            this.toolStripButtonInsertAction = new System.Windows.Forms.ToolStripButton();
            this.toolStripButtonDeleteAction = new System.Windows.Forms.ToolStripButton();
            this.toolStripButtonChangeAction = new System.Windows.Forms.ToolStripButton();
            this.toolStripButtonUndo = new System.Windows.Forms.ToolStripButton();
            this.toolStripButtonEditReferenceImage = new System.Windows.Forms.ToolStripButton();
            this.toolStripButtonPosition = new System.Windows.Forms.ToolStripButton();
            this.toolStripButtonPrevStep = new System.Windows.Forms.ToolStripButton();
            this.toolStripButtonNextStep = new System.Windows.Forms.ToolStripButton();
            this.groupBoxLog = new System.Windows.Forms.GroupBox();
            this.trackBar = new System.Windows.Forms.TrackBar();
            this.toolStripVideo = new System.Windows.Forms.ToolStrip();
            this.toolStripButtonPrevFrame = new System.Windows.Forms.ToolStripButton();
            this.toolStripButtonNextFrame = new System.Windows.Forms.ToolStripButton();
            this.toolStripButtonPlayVideo = new System.Windows.Forms.ToolStripButton();
            this.toolStripButtonShot = new System.Windows.Forms.ToolStripButton();
            this.saveFileDialog = new System.Windows.Forms.SaveFileDialog();
            this.textBoxActionInfo = new System.Windows.Forms.TextBox();
            this.comboBoxAction = new System.Windows.Forms.ComboBox();
            this.menuStrip = new System.Windows.Forms.MenuStrip();
            this.saveToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.saveAsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.rotateToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.loadAVIToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.editConfigToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.groupBoxHelp = new System.Windows.Forms.GroupBox();
            this.groupBoxProperty = new System.Windows.Forms.GroupBox();
            this.imageBox = new DaVinci.QScriptImageBox(this.components);
            this.aviOpenFileDialog = new System.Windows.Forms.OpenFileDialog();
            this.statusStrip = new System.Windows.Forms.StatusStrip();
            this.toolStripStatusLabelCoordinate = new System.Windows.Forms.ToolStripStatusLabel();
            ((System.ComponentModel.ISupportInitialize)(this.dataGridViewActionProperty)).BeginInit();
            this.groupBoxListActions.SuspendLayout();
            this.toolStripAction.SuspendLayout();
            this.groupBoxLog.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar)).BeginInit();
            this.toolStripVideo.SuspendLayout();
            this.menuStrip.SuspendLayout();
            this.groupBoxHelp.SuspendLayout();
            this.groupBoxProperty.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.imageBox)).BeginInit();
            this.statusStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // listBoxActions
            // 
            this.listBoxActions.Font = new System.Drawing.Font("Segoe UI", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.listBoxActions.FormattingEnabled = true;
            this.listBoxActions.HorizontalScrollbar = true;
            this.listBoxActions.ItemHeight = 17;
            this.listBoxActions.Location = new System.Drawing.Point(6, 55);
            this.listBoxActions.Name = "listBoxActions";
            this.listBoxActions.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended;
            this.listBoxActions.Size = new System.Drawing.Size(415, 531);
            this.listBoxActions.TabIndex = 30;
            this.listBoxActions.SelectedIndexChanged += new System.EventHandler(this.listBoxActions_SelectedIndexChanged);
            this.listBoxActions.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.listBoxActions_MouseDoubleClick);
            // 
            // dataGridViewActionProperty
            // 
            this.dataGridViewActionProperty.AllowUserToAddRows = false;
            this.dataGridViewActionProperty.AllowUserToDeleteRows = false;
            this.dataGridViewActionProperty.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.Fill;
            this.dataGridViewActionProperty.BackgroundColor = System.Drawing.SystemColors.Control;
            this.dataGridViewActionProperty.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.dataGridViewActionProperty.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.dataGridViewActionProperty.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.ColumnProperty,
            this.ColumnValue});
            this.dataGridViewActionProperty.EditMode = System.Windows.Forms.DataGridViewEditMode.EditOnKeystroke;
            this.dataGridViewActionProperty.Location = new System.Drawing.Point(6, 21);
            this.dataGridViewActionProperty.MultiSelect = false;
            this.dataGridViewActionProperty.Name = "dataGridViewActionProperty";
            this.dataGridViewActionProperty.RowHeadersVisible = false;
            this.dataGridViewActionProperty.Size = new System.Drawing.Size(346, 214);
            this.dataGridViewActionProperty.TabIndex = 31;
            // 
            // ColumnProperty
            // 
            dataGridViewCellStyle2.BackColor = System.Drawing.Color.White;
            dataGridViewCellStyle2.Font = new System.Drawing.Font("Segoe UI", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle2.ForeColor = System.Drawing.Color.Black;
            dataGridViewCellStyle2.SelectionBackColor = System.Drawing.Color.White;
            dataGridViewCellStyle2.SelectionForeColor = System.Drawing.Color.Black;
            this.ColumnProperty.DefaultCellStyle = dataGridViewCellStyle2;
            this.ColumnProperty.FillWeight = 71.06599F;
            this.ColumnProperty.HeaderText = "Property";
            this.ColumnProperty.Name = "ColumnProperty";
            this.ColumnProperty.ReadOnly = true;
            this.ColumnProperty.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            // 
            // ColumnValue
            // 
            this.ColumnValue.FillWeight = 128.934F;
            this.ColumnValue.HeaderText = "Value";
            this.ColumnValue.Name = "ColumnValue";
            this.ColumnValue.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.ColumnValue.ToolTipText = "Note: Text editing is  not finished if the curse is flashing.";
            // 
            // textBoxLog
            // 
            this.textBoxLog.BackColor = System.Drawing.SystemColors.ControlLight;
            this.textBoxLog.Location = new System.Drawing.Point(6, 21);
            this.textBoxLog.Multiline = true;
            this.textBoxLog.Name = "textBoxLog";
            this.textBoxLog.ReadOnly = true;
            this.textBoxLog.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.textBoxLog.Size = new System.Drawing.Size(346, 116);
            this.textBoxLog.TabIndex = 34;
            // 
            // groupBoxListActions
            // 
            this.groupBoxListActions.Controls.Add(this.toolStripAction);
            this.groupBoxListActions.Controls.Add(this.listBoxActions);
            this.groupBoxListActions.Location = new System.Drawing.Point(6, 28);
            this.groupBoxListActions.Name = "groupBoxListActions";
            this.groupBoxListActions.Size = new System.Drawing.Size(427, 591);
            this.groupBoxListActions.TabIndex = 33;
            this.groupBoxListActions.TabStop = false;
            this.groupBoxListActions.Text = "QScript";
            // 
            // toolStripAction
            // 
            this.toolStripAction.Dock = System.Windows.Forms.DockStyle.None;
            this.toolStripAction.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripButtonInsertAction,
            this.toolStripButtonDeleteAction,
            this.toolStripButtonChangeAction,
            this.toolStripButtonUndo,
            this.toolStripButtonEditReferenceImage,
            this.toolStripButtonPosition,
            this.toolStripButtonPrevStep,
            this.toolStripButtonNextStep});
            this.toolStripAction.Location = new System.Drawing.Point(6, 18);
            this.toolStripAction.Name = "toolStripAction";
            this.toolStripAction.Size = new System.Drawing.Size(196, 25);
            this.toolStripAction.Stretch = true;
            this.toolStripAction.TabIndex = 31;
            this.toolStripAction.Text = "toolStrip1";
            // 
            // toolStripButtonInsertAction
            // 
            this.toolStripButtonInsertAction.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolStripButtonInsertAction.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButtonInsertAction.Image")));
            this.toolStripButtonInsertAction.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripButtonInsertAction.Name = "toolStripButtonInsertAction";
            this.toolStripButtonInsertAction.Size = new System.Drawing.Size(23, 22);
            this.toolStripButtonInsertAction.Text = "Ins";
            this.toolStripButtonInsertAction.ToolTipText = "Insert an action";
            this.toolStripButtonInsertAction.Click += new System.EventHandler(this.toolStripButtonInsertAction_Click);
            // 
            // toolStripButtonDeleteAction
            // 
            this.toolStripButtonDeleteAction.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolStripButtonDeleteAction.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButtonDeleteAction.Image")));
            this.toolStripButtonDeleteAction.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripButtonDeleteAction.Name = "toolStripButtonDeleteAction";
            this.toolStripButtonDeleteAction.Size = new System.Drawing.Size(23, 22);
            this.toolStripButtonDeleteAction.Text = "toolStripButtonDeleteAction";
            this.toolStripButtonDeleteAction.ToolTipText = "Delete selected lines";
            this.toolStripButtonDeleteAction.Click += new System.EventHandler(this.toolStripButtonDeleteAction_Click);
            // 
            // toolStripButtonChangeAction
            // 
            this.toolStripButtonChangeAction.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolStripButtonChangeAction.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButtonChangeAction.Image")));
            this.toolStripButtonChangeAction.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripButtonChangeAction.Name = "toolStripButtonChangeAction";
            this.toolStripButtonChangeAction.Size = new System.Drawing.Size(23, 22);
            this.toolStripButtonChangeAction.Text = "Chg";
            this.toolStripButtonChangeAction.ToolTipText = "Change current line";
            this.toolStripButtonChangeAction.Click += new System.EventHandler(this.toolStripButtonChangeAction_Click);
            // 
            // toolStripButtonUndo
            // 
            this.toolStripButtonUndo.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolStripButtonUndo.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButtonUndo.Image")));
            this.toolStripButtonUndo.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripButtonUndo.Name = "toolStripButtonUndo";
            this.toolStripButtonUndo.Size = new System.Drawing.Size(23, 22);
            this.toolStripButtonUndo.Text = "Undo";
            this.toolStripButtonUndo.ToolTipText = "Undo the last operation";
            this.toolStripButtonUndo.Click += new System.EventHandler(this.toolStripButtonUndo_Click);
            // 
            // toolStripButtonEditReferenceImage
            // 
            this.toolStripButtonEditReferenceImage.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolStripButtonEditReferenceImage.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButtonEditReferenceImage.Image")));
            this.toolStripButtonEditReferenceImage.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripButtonEditReferenceImage.Name = "toolStripButtonEditReferenceImage";
            this.toolStripButtonEditReferenceImage.Size = new System.Drawing.Size(23, 22);
            this.toolStripButtonEditReferenceImage.Text = "Edit Reference Image";
            this.toolStripButtonEditReferenceImage.Click += new System.EventHandler(this.toolStripButtonEditReferenceImage_Click);
            // 
            // toolStripButtonPosition
            // 
            this.toolStripButtonPosition.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolStripButtonPosition.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButtonPosition.Image")));
            this.toolStripButtonPosition.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripButtonPosition.Name = "toolStripButtonPosition";
            this.toolStripButtonPosition.Size = new System.Drawing.Size(23, 22);
            this.toolStripButtonPosition.Text = "Position";
            this.toolStripButtonPosition.ToolTipText = "Go to the corresponding action of the current frame";
            this.toolStripButtonPosition.Click += new System.EventHandler(this.toolStripButtonPosition_Click);
            // 
            // toolStripButtonPrevStep
            // 
            this.toolStripButtonPrevStep.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolStripButtonPrevStep.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButtonPrevStep.Image")));
            this.toolStripButtonPrevStep.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripButtonPrevStep.Name = "toolStripButtonPrevStep";
            this.toolStripButtonPrevStep.Size = new System.Drawing.Size(23, 22);
            this.toolStripButtonPrevStep.Text = "PrevStep";
            this.toolStripButtonPrevStep.ToolTipText = "Previous Execution Step";
            this.toolStripButtonPrevStep.Click += new System.EventHandler(this.toolStripButtonPrevStep_Click);
            // 
            // toolStripButtonNextStep
            // 
            this.toolStripButtonNextStep.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolStripButtonNextStep.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButtonNextStep.Image")));
            this.toolStripButtonNextStep.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripButtonNextStep.Name = "toolStripButtonNextStep";
            this.toolStripButtonNextStep.Size = new System.Drawing.Size(23, 22);
            this.toolStripButtonNextStep.Text = "NextStep";
            this.toolStripButtonNextStep.ToolTipText = "Next Execution Step";
            this.toolStripButtonNextStep.Click += new System.EventHandler(this.toolStripButtonNextStep_Click);
            // 
            // groupBoxLog
            // 
            this.groupBoxLog.Controls.Add(this.textBoxLog);
            this.groupBoxLog.Location = new System.Drawing.Point(445, 476);
            this.groupBoxLog.Name = "groupBoxLog";
            this.groupBoxLog.Size = new System.Drawing.Size(358, 143);
            this.groupBoxLog.TabIndex = 37;
            this.groupBoxLog.TabStop = false;
            this.groupBoxLog.Text = "Messages";
            // 
            // trackBar
            // 
            this.trackBar.AutoSize = false;
            this.trackBar.Location = new System.Drawing.Point(814, 589);
            this.trackBar.Name = "trackBar";
            this.trackBar.Size = new System.Drawing.Size(257, 30);
            this.trackBar.TabIndex = 38;
            this.trackBar.ValueChanged += new System.EventHandler(this.trackBar_ValueChanged);
            // 
            // toolStripVideo
            // 
            this.toolStripVideo.Dock = System.Windows.Forms.DockStyle.None;
            this.toolStripVideo.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripButtonPrevFrame,
            this.toolStripButtonNextFrame,
            this.toolStripButtonPlayVideo,
            this.toolStripButtonShot});
            this.toolStripVideo.Location = new System.Drawing.Point(814, 37);
            this.toolStripVideo.Name = "toolStripVideo";
            this.toolStripVideo.Size = new System.Drawing.Size(135, 25);
            this.toolStripVideo.Stretch = true;
            this.toolStripVideo.TabIndex = 39;
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
            // saveFileDialog
            // 
            this.saveFileDialog.DefaultExt = "qs";
            this.saveFileDialog.Filter = "QScript files|*.qs";
            // 
            // textBoxActionInfo
            // 
            this.textBoxActionInfo.BackColor = System.Drawing.SystemColors.Control;
            this.textBoxActionInfo.Location = new System.Drawing.Point(6, 21);
            this.textBoxActionInfo.Multiline = true;
            this.textBoxActionInfo.Name = "textBoxActionInfo";
            this.textBoxActionInfo.ReadOnly = true;
            this.textBoxActionInfo.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.textBoxActionInfo.Size = new System.Drawing.Size(346, 132);
            this.textBoxActionInfo.TabIndex = 33;
            // 
            // comboBoxAction
            // 
            this.comboBoxAction.AutoCompleteMode = System.Windows.Forms.AutoCompleteMode.SuggestAppend;
            this.comboBoxAction.AutoCompleteSource = System.Windows.Forms.AutoCompleteSource.ListItems;
            this.comboBoxAction.FormattingEnabled = true;
            this.comboBoxAction.Location = new System.Drawing.Point(445, 37);
            this.comboBoxAction.Name = "comboBoxAction";
            this.comboBoxAction.Size = new System.Drawing.Size(265, 21);
            this.comboBoxAction.Sorted = true;
            this.comboBoxAction.TabIndex = 40;
            this.comboBoxAction.TextChanged += new System.EventHandler(this.comboBoxAction_TextChanged);
            // 
            // menuStrip
            // 
            this.menuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.saveToolStripMenuItem,
            this.saveAsToolStripMenuItem,
            this.rotateToolStripMenuItem,
            this.loadAVIToolStripMenuItem,
            this.editConfigToolStripMenuItem});
            this.menuStrip.Location = new System.Drawing.Point(0, 0);
            this.menuStrip.Name = "menuStrip";
            this.menuStrip.Size = new System.Drawing.Size(1184, 24);
            this.menuStrip.TabIndex = 41;
            this.menuStrip.Text = "menuStrip1";
            // 
            // saveToolStripMenuItem
            // 
            this.saveToolStripMenuItem.Name = "saveToolStripMenuItem";
            this.saveToolStripMenuItem.Size = new System.Drawing.Size(43, 20);
            this.saveToolStripMenuItem.Text = "&Save";
            this.saveToolStripMenuItem.Click += new System.EventHandler(this.saveToolStripMenuItem_Click);
            // 
            // saveAsToolStripMenuItem
            // 
            this.saveAsToolStripMenuItem.Name = "saveAsToolStripMenuItem";
            this.saveAsToolStripMenuItem.Size = new System.Drawing.Size(59, 20);
            this.saveAsToolStripMenuItem.Text = "Save &As";
            this.saveAsToolStripMenuItem.Click += new System.EventHandler(this.saveAsToolStripMenuItem_Click);
            // 
            // rotateToolStripMenuItem
            // 
            this.rotateToolStripMenuItem.Name = "rotateToolStripMenuItem";
            this.rotateToolStripMenuItem.Size = new System.Drawing.Size(53, 20);
            this.rotateToolStripMenuItem.Text = "&Rotate";
            this.rotateToolStripMenuItem.Click += new System.EventHandler(this.rotateToolStripMenuItem_Click);
            // 
            // loadAVIToolStripMenuItem
            // 
            this.loadAVIToolStripMenuItem.Name = "loadAVIToolStripMenuItem";
            this.loadAVIToolStripMenuItem.Size = new System.Drawing.Size(66, 20);
            this.loadAVIToolStripMenuItem.Text = "&Load AVI";
            this.loadAVIToolStripMenuItem.Click += new System.EventHandler(this.loadAVIToolStripMenuItem_Click);
            // 
            // editConfigToolStripMenuItem
            // 
            this.editConfigToolStripMenuItem.Name = "editConfigToolStripMenuItem";
            this.editConfigToolStripMenuItem.Size = new System.Drawing.Size(78, 20);
            this.editConfigToolStripMenuItem.Text = "&Edit Config";
            this.editConfigToolStripMenuItem.Click += new System.EventHandler(this.editConfigToolStripMenuItem_Click);
            // 
            // groupBoxHelp
            // 
            this.groupBoxHelp.Controls.Add(this.textBoxActionInfo);
            this.groupBoxHelp.Location = new System.Drawing.Point(445, 311);
            this.groupBoxHelp.Name = "groupBoxHelp";
            this.groupBoxHelp.Size = new System.Drawing.Size(358, 159);
            this.groupBoxHelp.TabIndex = 42;
            this.groupBoxHelp.TabStop = false;
            this.groupBoxHelp.Text = "Help";
            // 
            // groupBoxProperty
            // 
            this.groupBoxProperty.Controls.Add(this.dataGridViewActionProperty);
            this.groupBoxProperty.Location = new System.Drawing.Point(445, 64);
            this.groupBoxProperty.Name = "groupBoxProperty";
            this.groupBoxProperty.Size = new System.Drawing.Size(358, 241);
            this.groupBoxProperty.TabIndex = 43;
            this.groupBoxProperty.TabStop = false;
            this.groupBoxProperty.Text = "OPCODE Properties/Values";
            // 
            // imageBox
            // 
            this.imageBox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.imageBox.IsEditingPicture = false;
            this.imageBox.Location = new System.Drawing.Point(814, 64);
            this.imageBox.Name = "imageBox";
            this.imageBox.Size = new System.Drawing.Size(358, 513);
            this.imageBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
            this.imageBox.TabIndex = 35;
            this.imageBox.TabStop = false;
            this.imageBox.Text = "ImageBox";
            this.imageBox.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.imageBox_MouseDoubleClick);
            this.imageBox.MouseDown += new System.Windows.Forms.MouseEventHandler(this.imageBox_MouseDown);
            this.imageBox.MouseLeave += new System.EventHandler(this.imageBox_MouseLeave);
            this.imageBox.MouseMove += new System.Windows.Forms.MouseEventHandler(this.imageBox_MouseMove);
            this.imageBox.MouseUp += new System.Windows.Forms.MouseEventHandler(this.imageBox_MouseUp);
            // 
            // aviOpenFileDialog
            // 
            this.aviOpenFileDialog.Filter = "Video files|*.avi";
            // 
            // statusStrip
            // 
            this.statusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripStatusLabelCoordinate});
            this.statusStrip.Location = new System.Drawing.Point(0, 622);
            this.statusStrip.Name = "statusStrip";
            this.statusStrip.Size = new System.Drawing.Size(1184, 22);
            this.statusStrip.TabIndex = 44;
            this.statusStrip.Text = "statusStrip1";
            // 
            // toolStripStatusLabelCoordinate
            // 
            this.toolStripStatusLabelCoordinate.Name = "toolStripStatusLabelCoordinate";
            this.toolStripStatusLabelCoordinate.Size = new System.Drawing.Size(117, 17);
            this.toolStripStatusLabelCoordinate.Text = "Coordiate X = ? Y = ?";
            // 
            // QScriptEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.White;
            this.ClientSize = new System.Drawing.Size(1184, 644);
            this.Controls.Add(this.statusStrip);
            this.Controls.Add(this.groupBoxProperty);
            this.Controls.Add(this.groupBoxHelp);
            this.Controls.Add(this.comboBoxAction);
            this.Controls.Add(this.toolStripVideo);
            this.Controls.Add(this.menuStrip);
            this.Controls.Add(this.trackBar);
            this.Controls.Add(this.groupBoxLog);
            this.Controls.Add(this.groupBoxListActions);
            this.Controls.Add(this.imageBox);
            this.Font = new System.Drawing.Font("Segoe UI", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.MainMenuStrip = this.menuStrip;
            this.Name = "QScriptEditor";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "QScriptEditor";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.QScriptEditor_FormClosing);
            this.Load += new System.EventHandler(this.QScriptEditor_Load);
            this.Resize += new System.EventHandler(this.QScriptEditor_Resize);
            ((System.ComponentModel.ISupportInitialize)(this.dataGridViewActionProperty)).EndInit();
            this.groupBoxListActions.ResumeLayout(false);
            this.groupBoxListActions.PerformLayout();
            this.toolStripAction.ResumeLayout(false);
            this.toolStripAction.PerformLayout();
            this.groupBoxLog.ResumeLayout(false);
            this.groupBoxLog.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar)).EndInit();
            this.toolStripVideo.ResumeLayout(false);
            this.toolStripVideo.PerformLayout();
            this.menuStrip.ResumeLayout(false);
            this.menuStrip.PerformLayout();
            this.groupBoxHelp.ResumeLayout(false);
            this.groupBoxHelp.PerformLayout();
            this.groupBoxProperty.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.imageBox)).EndInit();
            this.statusStrip.ResumeLayout(false);
            this.statusStrip.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ListBox listBoxActions;
        private System.Windows.Forms.DataGridView dataGridViewActionProperty;
        private System.Windows.Forms.DataGridViewTextBoxColumn ColumnProperty;
        private System.Windows.Forms.DataGridViewTextBoxColumn ColumnValue;
        private System.Windows.Forms.TextBox textBoxLog;
        //private System.Windows.Forms.PictureBox imageBox;
        private System.Windows.Forms.GroupBox groupBoxListActions;
        private System.Windows.Forms.GroupBox groupBoxLog;
        private System.Windows.Forms.TrackBar trackBar;
        private System.Windows.Forms.ToolStrip toolStripVideo;
        private System.Windows.Forms.ToolStripButton toolStripButtonPrevFrame;
        private System.Windows.Forms.ToolStripButton toolStripButtonNextFrame;
        private System.Windows.Forms.ToolStripButton toolStripButtonPlayVideo;
        private System.Windows.Forms.ToolStripButton toolStripButtonShot;
        private System.Windows.Forms.SaveFileDialog saveFileDialog;
        private QScriptImageBox imageBox;
        private System.Windows.Forms.TextBox textBoxActionInfo;
        private System.Windows.Forms.ComboBox comboBoxAction;
        private System.Windows.Forms.MenuStrip menuStrip;
        private System.Windows.Forms.ToolStripMenuItem saveToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem saveAsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem rotateToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem loadAVIToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem editConfigToolStripMenuItem;
        private System.Windows.Forms.GroupBox groupBoxHelp;
        private System.Windows.Forms.GroupBox groupBoxProperty;

        private System.Windows.Forms.ToolStrip toolStripAction;
        private System.Windows.Forms.ToolStripButton toolStripButtonInsertAction;
        private System.Windows.Forms.ToolStripButton toolStripButtonDeleteAction;
        private System.Windows.Forms.ToolStripButton toolStripButtonChangeAction;
        private System.Windows.Forms.ToolStripButton toolStripButtonUndo;
        private System.Windows.Forms.ToolStripButton toolStripButtonEditReferenceImage;
        private System.Windows.Forms.ToolStripButton toolStripButtonPosition;
        private System.Windows.Forms.ToolStripButton toolStripButtonPrevStep;
        private System.Windows.Forms.ToolStripButton toolStripButtonNextStep;
        private System.Windows.Forms.OpenFileDialog aviOpenFileDialog;
        private System.Windows.Forms.StatusStrip statusStrip;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabelCoordinate;

    }
}