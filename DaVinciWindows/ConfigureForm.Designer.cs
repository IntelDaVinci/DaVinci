namespace DaVinci
{
    partial class ConfigureForm
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
            System.Windows.Forms.TreeNode treeNode1 = new System.Windows.Forms.TreeNode("Android");
            this.buttonDevicesRefresh = new System.Windows.Forms.Button();
            this.buttonDevicesDelete = new System.Windows.Forms.Button();
            this.buttonDevicesAdd = new System.Windows.Forms.Button();
            this.labelDeviceList = new System.Windows.Forms.Label();
            this.treeViewDevices = new System.Windows.Forms.TreeView();
            this.textBoxAgentStatus = new System.Windows.Forms.TextBox();
            this.labelAgentStatus = new System.Windows.Forms.Label();
            this.comboBoxDeviceCapture = new System.Windows.Forms.ComboBox();
            this.comboBoxDeviceRecordAudio = new System.Windows.Forms.ComboBox();
            this.buttonDevicesConnectAgent = new System.Windows.Forms.Button();
            this.labelDeviceRecordAudio = new System.Windows.Forms.Label();
            this.comboBoxDevicePlayAudio = new System.Windows.Forms.ComboBox();
            this.comboBoxMultiLayerSupport = new System.Windows.Forms.ComboBox();
            this.labelDeviceCapture = new System.Windows.Forms.Label();
            this.labelDevicePlayAudio = new System.Windows.Forms.Label();
            this.textBoxConnectedBy = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.multiLayerSupport = new System.Windows.Forms.Label();
            this.comboBoxRecordFromUser = new System.Windows.Forms.ComboBox();
            this.comboBoxPlayToUser = new System.Windows.Forms.ComboBox();
            this.labelFromUser = new System.Windows.Forms.Label();
            this.tabPageHostDeviceSetting = new System.Windows.Forms.TabPage();
            this.labelPlayToUser = new System.Windows.Forms.Label();
            this.checkBoxOthersGPU = new System.Windows.Forms.CheckBox();
            this.textBoxDeviceWidth = new System.Windows.Forms.TextBox();
            this.labelDeviceWidth = new System.Windows.Forms.Label();
            this.textBoxDeviceHeight = new System.Windows.Forms.TextBox();
            this.labelDeviceHeight = new System.Windows.Forms.Label();
            this.textBoxDeviceIp = new System.Windows.Forms.TextBox();
            this.labelFfrdController = new System.Windows.Forms.Label();
            this.comboBoxDeviceFfrdController = new System.Windows.Forms.ComboBox();
            this.labelWait = new System.Windows.Forms.Label();
            this.buttonMakeCurrent = new System.Windows.Forms.Button();
            this.groupBoxDeviceProperties = new System.Windows.Forms.GroupBox();
            this.labelDeviceIp = new System.Windows.Forms.Label();
            this.tabPageTargetDeviceSetting = new System.Windows.Forms.TabPage();
            this.tabControlConfigure = new System.Windows.Forms.TabControl();
            this.tabPageHostDeviceSetting.SuspendLayout();
            this.groupBoxDeviceProperties.SuspendLayout();
            this.tabPageTargetDeviceSetting.SuspendLayout();
            this.tabControlConfigure.SuspendLayout();
            this.SuspendLayout();
            // 
            // buttonDevicesRefresh
            // 
            this.buttonDevicesRefresh.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonDevicesRefresh.Location = new System.Drawing.Point(116, 257);
            this.buttonDevicesRefresh.Name = "buttonDevicesRefresh";
            this.buttonDevicesRefresh.Size = new System.Drawing.Size(147, 33);
            this.buttonDevicesRefresh.TabIndex = 6;
            this.buttonDevicesRefresh.Text = "Refresh";
            this.buttonDevicesRefresh.UseVisualStyleBackColor = true;
            this.buttonDevicesRefresh.Click += new System.EventHandler(this.buttonDevicesRefresh_Click);
            // 
            // buttonDevicesDelete
            // 
            this.buttonDevicesDelete.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonDevicesDelete.Location = new System.Drawing.Point(8, 317);
            this.buttonDevicesDelete.Name = "buttonDevicesDelete";
            this.buttonDevicesDelete.Size = new System.Drawing.Size(91, 37);
            this.buttonDevicesDelete.TabIndex = 4;
            this.buttonDevicesDelete.Text = "Delete";
            this.buttonDevicesDelete.UseVisualStyleBackColor = true;
            this.buttonDevicesDelete.Click += new System.EventHandler(this.buttonDevicesDelete_Click);
            // 
            // buttonDevicesAdd
            // 
            this.buttonDevicesAdd.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonDevicesAdd.Location = new System.Drawing.Point(8, 257);
            this.buttonDevicesAdd.Name = "buttonDevicesAdd";
            this.buttonDevicesAdd.Size = new System.Drawing.Size(91, 33);
            this.buttonDevicesAdd.TabIndex = 3;
            this.buttonDevicesAdd.Text = "Add";
            this.buttonDevicesAdd.UseVisualStyleBackColor = true;
            this.buttonDevicesAdd.Click += new System.EventHandler(this.buttonDevicesAdd_Click);
            // 
            // labelDeviceList
            // 
            this.labelDeviceList.AutoSize = true;
            this.labelDeviceList.Font = new System.Drawing.Font("Segoe UI", 15.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelDeviceList.Location = new System.Drawing.Point(3, 3);
            this.labelDeviceList.Name = "labelDeviceList";
            this.labelDeviceList.Size = new System.Drawing.Size(84, 30);
            this.labelDeviceList.TabIndex = 1;
            this.labelDeviceList.Text = "Devices";
            // 
            // treeViewDevices
            // 
            this.treeViewDevices.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.treeViewDevices.Location = new System.Drawing.Point(3, 32);
            this.treeViewDevices.Name = "treeViewDevices";
            treeNode1.Name = "NodeAndroid";
            treeNode1.Text = "Android";
            this.treeViewDevices.Nodes.AddRange(new System.Windows.Forms.TreeNode[] {
            treeNode1});
            this.treeViewDevices.Size = new System.Drawing.Size(260, 195);
            this.treeViewDevices.TabIndex = 0;
            this.treeViewDevices.BeforeSelect += new System.Windows.Forms.TreeViewCancelEventHandler(this.treeViewDevices_BeforeSelect);
            this.treeViewDevices.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeViewDevices_AfterSelect);
            // 
            // textBoxAgentStatus
            // 
            this.textBoxAgentStatus.Location = new System.Drawing.Point(134, 340);
            this.textBoxAgentStatus.Name = "textBoxAgentStatus";
            this.textBoxAgentStatus.ReadOnly = true;
            this.textBoxAgentStatus.Size = new System.Drawing.Size(110, 25);
            this.textBoxAgentStatus.TabIndex = 23;
            this.textBoxAgentStatus.TextChanged += new System.EventHandler(this.textBoxAgentStatus_TextChanged);
            // 
            // labelAgentStatus
            // 
            this.labelAgentStatus.AutoSize = true;
            this.labelAgentStatus.Location = new System.Drawing.Point(10, 340);
            this.labelAgentStatus.Name = "labelAgentStatus";
            this.labelAgentStatus.Size = new System.Drawing.Size(81, 17);
            this.labelAgentStatus.TabIndex = 22;
            this.labelAgentStatus.Text = "Agent Status";
            // 
            // comboBoxDeviceCapture
            // 
            this.comboBoxDeviceCapture.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxDeviceCapture.FormattingEnabled = true;
            this.comboBoxDeviceCapture.Location = new System.Drawing.Point(134, 199);
            this.comboBoxDeviceCapture.Name = "comboBoxDeviceCapture";
            this.comboBoxDeviceCapture.Size = new System.Drawing.Size(194, 25);
            this.comboBoxDeviceCapture.TabIndex = 20;
            this.comboBoxDeviceCapture.SelectedIndexChanged += new System.EventHandler(this.comboBoxDeviceCapture_SelectedIndexChanged);
            // 
            // comboBoxDeviceRecordAudio
            // 
            this.comboBoxDeviceRecordAudio.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxDeviceRecordAudio.FormattingEnabled = true;
            this.comboBoxDeviceRecordAudio.Location = new System.Drawing.Point(134, 269);
            this.comboBoxDeviceRecordAudio.Name = "comboBoxDeviceRecordAudio";
            this.comboBoxDeviceRecordAudio.Size = new System.Drawing.Size(194, 25);
            this.comboBoxDeviceRecordAudio.TabIndex = 19;
            this.comboBoxDeviceRecordAudio.SelectedIndexChanged += new System.EventHandler(this.comboBoxDeviceRecordAudio_SelectedIndexChanged);
            // 
            // buttonDevicesConnectAgent
            // 
            this.buttonDevicesConnectAgent.Enabled = false;
            this.buttonDevicesConnectAgent.Font = new System.Drawing.Font("Segoe UI", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonDevicesConnectAgent.Location = new System.Drawing.Point(250, 340);
            this.buttonDevicesConnectAgent.Name = "buttonDevicesConnectAgent";
            this.buttonDevicesConnectAgent.Size = new System.Drawing.Size(78, 27);
            this.buttonDevicesConnectAgent.TabIndex = 5;
            this.buttonDevicesConnectAgent.Text = "Connect";
            this.buttonDevicesConnectAgent.UseVisualStyleBackColor = true;
            this.buttonDevicesConnectAgent.Click += new System.EventHandler(this.buttonDevicesConnectAgent_Click);
            // 
            // labelDeviceRecordAudio
            // 
            this.labelDeviceRecordAudio.AutoSize = true;
            this.labelDeviceRecordAudio.Location = new System.Drawing.Point(10, 269);
            this.labelDeviceRecordAudio.Name = "labelDeviceRecordAudio";
            this.labelDeviceRecordAudio.Size = new System.Drawing.Size(88, 17);
            this.labelDeviceRecordAudio.TabIndex = 18;
            this.labelDeviceRecordAudio.Text = "Record Audio";
            // 
            // comboBoxDevicePlayAudio
            // 
            this.comboBoxDevicePlayAudio.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxDevicePlayAudio.FormattingEnabled = true;
            this.comboBoxDevicePlayAudio.Location = new System.Drawing.Point(134, 234);
            this.comboBoxDevicePlayAudio.Name = "comboBoxDevicePlayAudio";
            this.comboBoxDevicePlayAudio.Size = new System.Drawing.Size(194, 25);
            this.comboBoxDevicePlayAudio.TabIndex = 17;
            this.comboBoxDevicePlayAudio.SelectedIndexChanged += new System.EventHandler(this.comboBoxDevicePlayAudio_SelectedIndexChanged);
            // 
            // comboBoxMultiLayerSupport
            // 
            this.comboBoxMultiLayerSupport.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxMultiLayerSupport.FormattingEnabled = true;
            this.comboBoxMultiLayerSupport.Location = new System.Drawing.Point(134, 130);
            this.comboBoxMultiLayerSupport.Name = "comboBoxMultiLayerSupport";
            this.comboBoxMultiLayerSupport.Size = new System.Drawing.Size(194, 25);
            this.comboBoxMultiLayerSupport.TabIndex = 16;
            this.comboBoxMultiLayerSupport.SelectedIndexChanged += new System.EventHandler(this.comboBoxMultiLayerSupport_SelectedIndexChanged);
            // 
            // labelDeviceCapture
            // 
            this.labelDeviceCapture.AutoSize = true;
            this.labelDeviceCapture.Location = new System.Drawing.Point(10, 199);
            this.labelDeviceCapture.Name = "labelDeviceCapture";
            this.labelDeviceCapture.Size = new System.Drawing.Size(91, 17);
            this.labelDeviceCapture.TabIndex = 12;
            this.labelDeviceCapture.Text = "Screen Source";
            // 
            // labelDevicePlayAudio
            // 
            this.labelDevicePlayAudio.AutoSize = true;
            this.labelDevicePlayAudio.Location = new System.Drawing.Point(10, 234);
            this.labelDevicePlayAudio.Name = "labelDevicePlayAudio";
            this.labelDevicePlayAudio.Size = new System.Drawing.Size(69, 17);
            this.labelDevicePlayAudio.TabIndex = 10;
            this.labelDevicePlayAudio.Text = "Play Audio";
            // 
            // textBoxConnectedBy
            // 
            this.textBoxConnectedBy.Location = new System.Drawing.Point(134, 165);
            this.textBoxConnectedBy.Name = "textBoxConnectedBy";
            this.textBoxConnectedBy.ReadOnly = true;
            this.textBoxConnectedBy.Size = new System.Drawing.Size(194, 25);
            this.textBoxConnectedBy.TabIndex = 9;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(10, 165);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(87, 17);
            this.label1.TabIndex = 8;
            this.label1.Text = "Connected By";
            // 
            // multiLayerSupport
            // 
            this.multiLayerSupport.AutoSize = true;
            this.multiLayerSupport.Location = new System.Drawing.Point(10, 130);
            this.multiLayerSupport.Name = "multiLayerSupport";
            this.multiLayerSupport.Size = new System.Drawing.Size(124, 17);
            this.multiLayerSupport.TabIndex = 6;
            this.multiLayerSupport.Text = "Multi-Layer Support";
            // 
            // comboBoxRecordFromUser
            // 
            this.comboBoxRecordFromUser.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxRecordFromUser.Font = new System.Drawing.Font("Segoe UI", 15.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.comboBoxRecordFromUser.FormattingEnabled = true;
            this.comboBoxRecordFromUser.Location = new System.Drawing.Point(207, 142);
            this.comboBoxRecordFromUser.Name = "comboBoxRecordFromUser";
            this.comboBoxRecordFromUser.Size = new System.Drawing.Size(204, 38);
            this.comboBoxRecordFromUser.TabIndex = 14;
            this.comboBoxRecordFromUser.SelectedIndexChanged += new System.EventHandler(this.comboBoxRecordFromUser_SelectedIndexChanged);
            // 
            // comboBoxPlayToUser
            // 
            this.comboBoxPlayToUser.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxPlayToUser.Font = new System.Drawing.Font("Segoe UI", 15.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.comboBoxPlayToUser.FormattingEnabled = true;
            this.comboBoxPlayToUser.Location = new System.Drawing.Point(207, 88);
            this.comboBoxPlayToUser.Name = "comboBoxPlayToUser";
            this.comboBoxPlayToUser.Size = new System.Drawing.Size(204, 38);
            this.comboBoxPlayToUser.TabIndex = 13;
            this.comboBoxPlayToUser.SelectedIndexChanged += new System.EventHandler(this.comboBoxPlayToUser_SelectedIndexChanged);
            // 
            // labelFromUser
            // 
            this.labelFromUser.AutoSize = true;
            this.labelFromUser.Font = new System.Drawing.Font("Segoe UI", 15.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelFromUser.Location = new System.Drawing.Point(17, 145);
            this.labelFromUser.Name = "labelFromUser";
            this.labelFromUser.Size = new System.Drawing.Size(175, 30);
            this.labelFromUser.TabIndex = 10;
            this.labelFromUser.Text = "Record from User";
            // 
            // tabPageHostDeviceSetting
            // 
            this.tabPageHostDeviceSetting.Controls.Add(this.comboBoxRecordFromUser);
            this.tabPageHostDeviceSetting.Controls.Add(this.comboBoxPlayToUser);
            this.tabPageHostDeviceSetting.Controls.Add(this.labelFromUser);
            this.tabPageHostDeviceSetting.Controls.Add(this.labelPlayToUser);
            this.tabPageHostDeviceSetting.Controls.Add(this.checkBoxOthersGPU);
            this.tabPageHostDeviceSetting.Location = new System.Drawing.Point(4, 22);
            this.tabPageHostDeviceSetting.Name = "tabPageHostDeviceSetting";
            this.tabPageHostDeviceSetting.Size = new System.Drawing.Size(620, 414);
            this.tabPageHostDeviceSetting.TabIndex = 3;
            this.tabPageHostDeviceSetting.Text = "Devices on Host";
            this.tabPageHostDeviceSetting.UseVisualStyleBackColor = true;
            // 
            // labelPlayToUser
            // 
            this.labelPlayToUser.AutoSize = true;
            this.labelPlayToUser.Font = new System.Drawing.Font("Segoe UI", 15.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelPlayToUser.Location = new System.Drawing.Point(17, 91);
            this.labelPlayToUser.Name = "labelPlayToUser";
            this.labelPlayToUser.Size = new System.Drawing.Size(123, 30);
            this.labelPlayToUser.TabIndex = 9;
            this.labelPlayToUser.Text = "Play to User";
            // 
            // checkBoxOthersGPU
            // 
            this.checkBoxOthersGPU.AutoSize = true;
            this.checkBoxOthersGPU.Font = new System.Drawing.Font("Segoe UI", 15.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.checkBoxOthersGPU.Location = new System.Drawing.Point(22, 39);
            this.checkBoxOthersGPU.Name = "checkBoxOthersGPU";
            this.checkBoxOthersGPU.Size = new System.Drawing.Size(233, 34);
            this.checkBoxOthersGPU.TabIndex = 0;
            this.checkBoxOthersGPU.Text = "Use GPU Acceleration";
            this.checkBoxOthersGPU.UseVisualStyleBackColor = true;
            this.checkBoxOthersGPU.Click += new System.EventHandler(this.checkBoxOthersGPU_Click);
            // 
            // textBoxDeviceWidth
            // 
            this.textBoxDeviceWidth.Location = new System.Drawing.Point(134, 95);
            this.textBoxDeviceWidth.Name = "textBoxDeviceWidth";
            this.textBoxDeviceWidth.Size = new System.Drawing.Size(194, 25);
            this.textBoxDeviceWidth.TabIndex = 5;
            // 
            // labelDeviceWidth
            // 
            this.labelDeviceWidth.AutoSize = true;
            this.labelDeviceWidth.Location = new System.Drawing.Point(10, 95);
            this.labelDeviceWidth.Name = "labelDeviceWidth";
            this.labelDeviceWidth.Size = new System.Drawing.Size(42, 17);
            this.labelDeviceWidth.TabIndex = 4;
            this.labelDeviceWidth.Text = "Width";
            // 
            // textBoxDeviceHeight
            // 
            this.textBoxDeviceHeight.Location = new System.Drawing.Point(134, 60);
            this.textBoxDeviceHeight.Name = "textBoxDeviceHeight";
            this.textBoxDeviceHeight.Size = new System.Drawing.Size(194, 25);
            this.textBoxDeviceHeight.TabIndex = 3;
            // 
            // labelDeviceHeight
            // 
            this.labelDeviceHeight.AutoSize = true;
            this.labelDeviceHeight.Location = new System.Drawing.Point(10, 60);
            this.labelDeviceHeight.Name = "labelDeviceHeight";
            this.labelDeviceHeight.Size = new System.Drawing.Size(46, 17);
            this.labelDeviceHeight.TabIndex = 2;
            this.labelDeviceHeight.Text = "Height";
            // 
            // textBoxDeviceIp
            // 
            this.textBoxDeviceIp.Location = new System.Drawing.Point(134, 25);
            this.textBoxDeviceIp.Name = "textBoxDeviceIp";
            this.textBoxDeviceIp.ReadOnly = true;
            this.textBoxDeviceIp.Size = new System.Drawing.Size(194, 25);
            this.textBoxDeviceIp.TabIndex = 1;
            // 
            // labelFfrdController
            // 
            this.labelFfrdController.AutoSize = true;
            this.labelFfrdController.Location = new System.Drawing.Point(10, 305);
            this.labelFfrdController.Name = "labelFfrdController";
            this.labelFfrdController.Size = new System.Drawing.Size(97, 17);
            this.labelFfrdController.TabIndex = 14;
            this.labelFfrdController.Text = "FFRD controller";
            // 
            // comboBoxDeviceFfrdController
            // 
            this.comboBoxDeviceFfrdController.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxDeviceFfrdController.FormattingEnabled = true;
            this.comboBoxDeviceFfrdController.Location = new System.Drawing.Point(134, 305);
            this.comboBoxDeviceFfrdController.Name = "comboBoxDeviceFfrdController";
            this.comboBoxDeviceFfrdController.Size = new System.Drawing.Size(194, 25);
            this.comboBoxDeviceFfrdController.TabIndex = 21;
            this.comboBoxDeviceFfrdController.SelectedIndexChanged += new System.EventHandler(this.comboBoxDeviceFfrdController_SelectedIndexChanged);
            // 
            // labelWait
            // 
            this.labelWait.AutoSize = true;
            this.labelWait.Font = new System.Drawing.Font("Segoe UI", 38.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelWait.Location = new System.Drawing.Point(6, 355);
            this.labelWait.Name = "labelWait";
            this.labelWait.Size = new System.Drawing.Size(311, 68);
            this.labelWait.TabIndex = 11;
            this.labelWait.Text = "Please wait...";
            this.labelWait.Visible = false;
            // 
            // buttonMakeCurrent
            // 
            this.buttonMakeCurrent.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonMakeCurrent.Location = new System.Drawing.Point(116, 317);
            this.buttonMakeCurrent.Name = "buttonMakeCurrent";
            this.buttonMakeCurrent.Size = new System.Drawing.Size(147, 37);
            this.buttonMakeCurrent.TabIndex = 9;
            this.buttonMakeCurrent.Text = "Set As Current";
            this.buttonMakeCurrent.UseVisualStyleBackColor = true;
            this.buttonMakeCurrent.Click += new System.EventHandler(this.buttonMakeCurrent_Click);
            // 
            // groupBoxDeviceProperties
            // 
            this.groupBoxDeviceProperties.Controls.Add(this.textBoxAgentStatus);
            this.groupBoxDeviceProperties.Controls.Add(this.labelAgentStatus);
            this.groupBoxDeviceProperties.Controls.Add(this.comboBoxDeviceCapture);
            this.groupBoxDeviceProperties.Controls.Add(this.comboBoxDeviceRecordAudio);
            this.groupBoxDeviceProperties.Controls.Add(this.buttonDevicesConnectAgent);
            this.groupBoxDeviceProperties.Controls.Add(this.labelDeviceRecordAudio);
            this.groupBoxDeviceProperties.Controls.Add(this.comboBoxDevicePlayAudio);
            this.groupBoxDeviceProperties.Controls.Add(this.comboBoxMultiLayerSupport);
            this.groupBoxDeviceProperties.Controls.Add(this.labelDeviceCapture);
            this.groupBoxDeviceProperties.Controls.Add(this.labelDevicePlayAudio);
            this.groupBoxDeviceProperties.Controls.Add(this.textBoxConnectedBy);
            this.groupBoxDeviceProperties.Controls.Add(this.label1);
            this.groupBoxDeviceProperties.Controls.Add(this.multiLayerSupport);
            this.groupBoxDeviceProperties.Controls.Add(this.textBoxDeviceWidth);
            this.groupBoxDeviceProperties.Controls.Add(this.labelDeviceWidth);
            this.groupBoxDeviceProperties.Controls.Add(this.textBoxDeviceHeight);
            this.groupBoxDeviceProperties.Controls.Add(this.labelDeviceHeight);
            this.groupBoxDeviceProperties.Controls.Add(this.textBoxDeviceIp);
            this.groupBoxDeviceProperties.Controls.Add(this.labelDeviceIp);
            this.groupBoxDeviceProperties.Controls.Add(this.labelFfrdController);
            this.groupBoxDeviceProperties.Controls.Add(this.comboBoxDeviceFfrdController);
            this.groupBoxDeviceProperties.Font = new System.Drawing.Font("Segoe UI", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBoxDeviceProperties.Location = new System.Drawing.Point(269, 3);
            this.groupBoxDeviceProperties.Name = "groupBoxDeviceProperties";
            this.groupBoxDeviceProperties.Size = new System.Drawing.Size(345, 402);
            this.groupBoxDeviceProperties.TabIndex = 8;
            this.groupBoxDeviceProperties.TabStop = false;
            this.groupBoxDeviceProperties.Text = "Device Properties";
            // 
            // labelDeviceIp
            // 
            this.labelDeviceIp.AutoSize = true;
            this.labelDeviceIp.Location = new System.Drawing.Point(10, 25);
            this.labelDeviceIp.Name = "labelDeviceIp";
            this.labelDeviceIp.Size = new System.Drawing.Size(18, 17);
            this.labelDeviceIp.TabIndex = 0;
            this.labelDeviceIp.Text = "IP";
            // 
            // tabPageTargetDeviceSetting
            // 
            this.tabPageTargetDeviceSetting.Controls.Add(this.labelWait);
            this.tabPageTargetDeviceSetting.Controls.Add(this.buttonMakeCurrent);
            this.tabPageTargetDeviceSetting.Controls.Add(this.groupBoxDeviceProperties);
            this.tabPageTargetDeviceSetting.Controls.Add(this.buttonDevicesRefresh);
            this.tabPageTargetDeviceSetting.Controls.Add(this.buttonDevicesDelete);
            this.tabPageTargetDeviceSetting.Controls.Add(this.buttonDevicesAdd);
            this.tabPageTargetDeviceSetting.Controls.Add(this.labelDeviceList);
            this.tabPageTargetDeviceSetting.Controls.Add(this.treeViewDevices);
            this.tabPageTargetDeviceSetting.Location = new System.Drawing.Point(4, 22);
            this.tabPageTargetDeviceSetting.Name = "tabPageTargetDeviceSetting";
            this.tabPageTargetDeviceSetting.Padding = new System.Windows.Forms.Padding(3);
            this.tabPageTargetDeviceSetting.Size = new System.Drawing.Size(620, 414);
            this.tabPageTargetDeviceSetting.TabIndex = 0;
            this.tabPageTargetDeviceSetting.Text = "Target Devices";
            this.tabPageTargetDeviceSetting.UseVisualStyleBackColor = true;
            // 
            // tabControlConfigure
            // 
            this.tabControlConfigure.Controls.Add(this.tabPageTargetDeviceSetting);
            this.tabControlConfigure.Controls.Add(this.tabPageHostDeviceSetting);
            this.tabControlConfigure.Font = new System.Drawing.Font("Segoe UI", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.tabControlConfigure.Location = new System.Drawing.Point(3, 7);
            this.tabControlConfigure.Name = "tabControlConfigure";
            this.tabControlConfigure.SelectedIndex = 0;
            this.tabControlConfigure.Size = new System.Drawing.Size(628, 440);
            this.tabControlConfigure.TabIndex = 1;
            // 
            // ConfigureForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(635, 455);
            this.Controls.Add(this.tabControlConfigure);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ConfigureForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "DaVinci Configuration";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.ConfigureForm_FormClosed);
            this.Load += new System.EventHandler(this.ConfigureForm_Load);
            this.tabPageHostDeviceSetting.ResumeLayout(false);
            this.tabPageHostDeviceSetting.PerformLayout();
            this.groupBoxDeviceProperties.ResumeLayout(false);
            this.groupBoxDeviceProperties.PerformLayout();
            this.tabPageTargetDeviceSetting.ResumeLayout(false);
            this.tabPageTargetDeviceSetting.PerformLayout();
            this.tabControlConfigure.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button buttonDevicesRefresh;
        private System.Windows.Forms.Button buttonDevicesDelete;
        private System.Windows.Forms.Button buttonDevicesAdd;
        private System.Windows.Forms.Label labelDeviceList;
        private System.Windows.Forms.TreeView treeViewDevices;
        private System.Windows.Forms.TextBox textBoxAgentStatus;
        private System.Windows.Forms.Label labelAgentStatus;
        private System.Windows.Forms.ComboBox comboBoxDeviceCapture;
        private System.Windows.Forms.ComboBox comboBoxDeviceRecordAudio;
        private System.Windows.Forms.Button buttonDevicesConnectAgent;
        private System.Windows.Forms.Label labelDeviceRecordAudio;
        private System.Windows.Forms.ComboBox comboBoxDevicePlayAudio;
        private System.Windows.Forms.ComboBox comboBoxMultiLayerSupport;
        private System.Windows.Forms.Label labelDeviceCapture;
        private System.Windows.Forms.Label labelDevicePlayAudio;
        private System.Windows.Forms.TextBox textBoxConnectedBy;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label multiLayerSupport;
        private System.Windows.Forms.ComboBox comboBoxRecordFromUser;
        private System.Windows.Forms.ComboBox comboBoxPlayToUser;
        private System.Windows.Forms.Label labelFromUser;
        private System.Windows.Forms.TabPage tabPageHostDeviceSetting;
        private System.Windows.Forms.Label labelPlayToUser;
        private System.Windows.Forms.CheckBox checkBoxOthersGPU;
        private System.Windows.Forms.TextBox textBoxDeviceWidth;
        private System.Windows.Forms.Label labelDeviceWidth;
        private System.Windows.Forms.TextBox textBoxDeviceHeight;
        private System.Windows.Forms.Label labelDeviceHeight;
        private System.Windows.Forms.TextBox textBoxDeviceIp;
        private System.Windows.Forms.Label labelFfrdController;
        private System.Windows.Forms.ComboBox comboBoxDeviceFfrdController;
        private System.Windows.Forms.Label labelWait;
        private System.Windows.Forms.Button buttonMakeCurrent;
        private System.Windows.Forms.GroupBox groupBoxDeviceProperties;
        private System.Windows.Forms.Label labelDeviceIp;
        private System.Windows.Forms.TabPage tabPageTargetDeviceSetting;
        private System.Windows.Forms.TabControl tabControlConfigure;
    }
}