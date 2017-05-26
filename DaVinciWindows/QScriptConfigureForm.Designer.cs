namespace DaVinci
{
    partial class QScriptConfigureForm
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
            this.labelConfiguration = new System.Windows.Forms.Label();
            this.labelConfigValue = new System.Windows.Forms.Label();
            this.configComboBox = new System.Windows.Forms.ComboBox();
            this.confValueBox = new System.Windows.Forms.TextBox();
            this.SuspendLayout();
            // 
            // labelConfiguration
            // 
            this.labelConfiguration.AutoSize = true;
            this.labelConfiguration.Font = new System.Drawing.Font("Segoe UI", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelConfiguration.Location = new System.Drawing.Point(12, 30);
            this.labelConfiguration.Name = "labelConfiguration";
            this.labelConfiguration.Size = new System.Drawing.Size(130, 17);
            this.labelConfiguration.TabIndex = 12;
            this.labelConfiguration.Text = "[Configuration Items]";
            // 
            // labelConfigValue
            // 
            this.labelConfigValue.AutoSize = true;
            this.labelConfigValue.Font = new System.Drawing.Font("Segoe UI", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelConfigValue.Location = new System.Drawing.Point(204, 30);
            this.labelConfigValue.Name = "labelConfigValue";
            this.labelConfigValue.Size = new System.Drawing.Size(131, 17);
            this.labelConfigValue.TabIndex = 13;
            this.labelConfigValue.Text = "[Configuration Value]";
            // 
            // configComboBox
            // 
            this.configComboBox.FormattingEnabled = true;
            this.configComboBox.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.configComboBox.Location = new System.Drawing.Point(15, 58);
            this.configComboBox.Name = "configComboBox";
            this.configComboBox.Size = new System.Drawing.Size(164, 21);
            this.configComboBox.TabIndex = 14;
            this.configComboBox.SelectedIndexChanged += new System.EventHandler(this.configComboBox_SelectedIndexChanged);
            // 
            // confValueBox
            // 
            this.confValueBox.Location = new System.Drawing.Point(207, 59);
            this.confValueBox.Name = "confValueBox";
            this.confValueBox.Size = new System.Drawing.Size(188, 20);
            this.confValueBox.TabIndex = 15;
            this.confValueBox.TextChanged += new System.EventHandler(this.confValueBox_TextChanged);
            // 
            // QScriptConfigureForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(407, 127);
            this.Controls.Add(this.confValueBox);
            this.Controls.Add(this.configComboBox);
            this.Controls.Add(this.labelConfigValue);
            this.Controls.Add(this.labelConfiguration);
            this.Name = "QScriptConfigureForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "QScript Configure";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.QScriptConfigureForm_FormClosing);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label labelConfiguration;
        private System.Windows.Forms.Label labelConfigValue;
        private System.Windows.Forms.ComboBox configComboBox;
        private System.Windows.Forms.TextBox confValueBox;

    }
}