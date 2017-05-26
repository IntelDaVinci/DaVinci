namespace DaVinci
{
    partial class CreateTestSelect
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
            this.radioButtonRnRButton = new System.Windows.Forms.RadioButton();
            this.radioButtonFPS = new System.Windows.Forms.RadioButton();
            this.radioButtonLaunchTime = new System.Windows.Forms.RadioButton();
            this.label1 = new System.Windows.Forms.Label();
            this.SeTestTypeCancelButton = new System.Windows.Forms.Button();
            this.SeTestTypeSelectButton = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // radioButtonRnRButton
            // 
            this.radioButtonRnRButton.AutoSize = true;
            this.radioButtonRnRButton.Location = new System.Drawing.Point(67, 65);
            this.radioButtonRnRButton.Name = "radioButtonRnRButton";
            this.radioButtonRnRButton.Size = new System.Drawing.Size(96, 17);
            this.radioButtonRnRButton.TabIndex = 0;
            this.radioButtonRnRButton.TabStop = true;
            this.radioButtonRnRButton.Text = "Record Replay";
            this.radioButtonRnRButton.UseVisualStyleBackColor = true;
            // 
            // radioButtonFPS
            // 
            this.radioButtonFPS.AutoSize = true;
            this.radioButtonFPS.Location = new System.Drawing.Point(67, 89);
            this.radioButtonFPS.Name = "radioButtonFPS";
            this.radioButtonFPS.Size = new System.Drawing.Size(45, 17);
            this.radioButtonFPS.TabIndex = 1;
            this.radioButtonFPS.TabStop = true;
            this.radioButtonFPS.Text = "FPS";
            this.radioButtonFPS.UseVisualStyleBackColor = true;
            // 
            // radioButtonLaunchTime
            // 
            this.radioButtonLaunchTime.AutoSize = true;
            this.radioButtonLaunchTime.Location = new System.Drawing.Point(67, 113);
            this.radioButtonLaunchTime.Name = "radioButtonLaunchTime";
            this.radioButtonLaunchTime.Size = new System.Drawing.Size(87, 17);
            this.radioButtonLaunchTime.TabIndex = 2;
            this.radioButtonLaunchTime.TabStop = true;
            this.radioButtonLaunchTime.Text = "Launch Time";
            this.radioButtonLaunchTime.UseVisualStyleBackColor = true;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(64, 36);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(127, 16);
            this.label1.TabIndex = 3;
            this.label1.Text = "Select Test Type";
            // 
            // SeTestTypeCancelButton
            // 
            this.SeTestTypeCancelButton.Location = new System.Drawing.Point(159, 145);
            this.SeTestTypeCancelButton.Name = "SeTestTypeCancelButton";
            this.SeTestTypeCancelButton.Size = new System.Drawing.Size(75, 23);
            this.SeTestTypeCancelButton.TabIndex = 4;
            this.SeTestTypeCancelButton.Text = "Cancel";
            this.SeTestTypeCancelButton.UseVisualStyleBackColor = true;
            this.SeTestTypeCancelButton.Click += new System.EventHandler(this.CancelButton_Click);
            // 
            // SeTestTypeSelectButton
            // 
            this.SeTestTypeSelectButton.Location = new System.Drawing.Point(37, 145);
            this.SeTestTypeSelectButton.Name = "SeTestTypeSelectButton";
            this.SeTestTypeSelectButton.Size = new System.Drawing.Size(75, 23);
            this.SeTestTypeSelectButton.TabIndex = 5;
            this.SeTestTypeSelectButton.Text = "Select";
            this.SeTestTypeSelectButton.UseVisualStyleBackColor = true;
            this.SeTestTypeSelectButton.Click += new System.EventHandler(this.SeTestTypeSelectButton_Click);
            // 
            // CreateTestSelect
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(261, 192);
            this.Controls.Add(this.SeTestTypeSelectButton);
            this.Controls.Add(this.SeTestTypeCancelButton);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.radioButtonLaunchTime);
            this.Controls.Add(this.radioButtonFPS);
            this.Controls.Add(this.radioButtonRnRButton);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.None;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "CreateTestSelect";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "CreateTestSelect";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.RadioButton radioButtonRnRButton;
        private System.Windows.Forms.RadioButton radioButtonFPS;
        private System.Windows.Forms.RadioButton radioButtonLaunchTime;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button SeTestTypeCancelButton;
        private System.Windows.Forms.Button SeTestTypeSelectButton;
    }
}