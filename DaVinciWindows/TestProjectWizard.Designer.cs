namespace DaVinci
{
    partial class TestProjectWizard
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
            this.buttonProjectLocation = new System.Windows.Forms.Button();
            this.buttonCancel = new System.Windows.Forms.Button();
            this.buttonCreate = new System.Windows.Forms.Button();
            this.textBoxProjectLocation = new System.Windows.Forms.TextBox();
            this.label4 = new System.Windows.Forms.Label();
            this.textBoxProjectName = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // buttonProjectLocation
            // 
            this.buttonProjectLocation.Font = new System.Drawing.Font("Segoe UI", 14.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonProjectLocation.Location = new System.Drawing.Point(480, 25);
            this.buttonProjectLocation.Name = "buttonProjectLocation";
            this.buttonProjectLocation.Size = new System.Drawing.Size(43, 29);
            this.buttonProjectLocation.TabIndex = 24;
            this.buttonProjectLocation.Text = "...";
            this.buttonProjectLocation.UseVisualStyleBackColor = true;
            this.buttonProjectLocation.Click += new System.EventHandler(this.buttonProjectLocation_Click);
            // 
            // buttonCancel
            // 
            this.buttonCancel.Font = new System.Drawing.Font("Segoe UI", 14.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonCancel.Location = new System.Drawing.Point(304, 150);
            this.buttonCancel.Name = "buttonCancel";
            this.buttonCancel.Size = new System.Drawing.Size(101, 35);
            this.buttonCancel.TabIndex = 22;
            this.buttonCancel.Text = "Cancel";
            this.buttonCancel.UseVisualStyleBackColor = true;
            this.buttonCancel.Click += new System.EventHandler(this.buttonCancel_Click);
            // 
            // buttonCreate
            // 
            this.buttonCreate.Font = new System.Drawing.Font("Segoe UI", 14.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonCreate.Location = new System.Drawing.Point(103, 150);
            this.buttonCreate.Name = "buttonCreate";
            this.buttonCreate.Size = new System.Drawing.Size(101, 35);
            this.buttonCreate.TabIndex = 21;
            this.buttonCreate.Text = "Create";
            this.buttonCreate.UseVisualStyleBackColor = true;
            this.buttonCreate.Click += new System.EventHandler(this.buttonCreate_Click);
            // 
            // textBoxProjectLocation
            // 
            this.textBoxProjectLocation.Font = new System.Drawing.Font("Segoe UI", 14.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.textBoxProjectLocation.Location = new System.Drawing.Point(194, 22);
            this.textBoxProjectLocation.Name = "textBoxProjectLocation";
            this.textBoxProjectLocation.Size = new System.Drawing.Size(266, 33);
            this.textBoxProjectLocation.TabIndex = 20;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Font = new System.Drawing.Font("Segoe UI", 14.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label4.Location = new System.Drawing.Point(24, 25);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(156, 25);
            this.label4.TabIndex = 19;
            this.label4.Text = "Project Location*";
            // 
            // textBoxProjectName
            // 
            this.textBoxProjectName.Font = new System.Drawing.Font("Segoe UI", 14.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.textBoxProjectName.Location = new System.Drawing.Point(194, 72);
            this.textBoxProjectName.Name = "textBoxProjectName";
            this.textBoxProjectName.Size = new System.Drawing.Size(266, 33);
            this.textBoxProjectName.TabIndex = 18;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Segoe UI", 14.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(24, 72);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(134, 25);
            this.label3.TabIndex = 17;
            this.label3.Text = "Project Name*";
            // 
            // TestProjectWizard
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(556, 213);
            this.Controls.Add(this.buttonProjectLocation);
            this.Controls.Add(this.buttonCancel);
            this.Controls.Add(this.buttonCreate);
            this.Controls.Add(this.textBoxProjectLocation);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.textBoxProjectName);
            this.Controls.Add(this.label3);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "TestProjectWizard";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "New Test Project Wizard";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button buttonProjectLocation;
        private System.Windows.Forms.Button buttonCancel;
        private System.Windows.Forms.Button buttonCreate;
        private System.Windows.Forms.TextBox textBoxProjectLocation;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TextBox textBoxProjectName;
        private System.Windows.Forms.Label label3;
    }
}