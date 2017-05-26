namespace DaVinci
{
    partial class PushDataConfigure
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
            this.labelSource = new System.Windows.Forms.Label();
            this.labelTarget = new System.Windows.Forms.Label();
            this.buttonOK = new System.Windows.Forms.Button();
            this.buttonClose = new System.Windows.Forms.Button();
            this.textBoxSource = new System.Windows.Forms.TextBox();
            this.textBoxTarget = new System.Windows.Forms.TextBox();
            this.buttonBrowser = new System.Windows.Forms.Button();
            this.buttonReset = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // labelSource
            // 
            this.labelSource.AutoSize = true;
            this.labelSource.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelSource.Location = new System.Drawing.Point(47, 48);
            this.labelSource.Name = "labelSource";
            this.labelSource.Size = new System.Drawing.Size(70, 21);
            this.labelSource.TabIndex = 0;
            this.labelSource.Text = "Source: ";
            // 
            // labelTarget
            // 
            this.labelTarget.AutoSize = true;
            this.labelTarget.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelTarget.Location = new System.Drawing.Point(47, 88);
            this.labelTarget.Name = "labelTarget";
            this.labelTarget.Size = new System.Drawing.Size(67, 21);
            this.labelTarget.TabIndex = 1;
            this.labelTarget.Text = "Target: ";
            // 
            // buttonOK
            // 
            this.buttonOK.Font = new System.Drawing.Font("Segoe UI", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonOK.Location = new System.Drawing.Point(51, 138);
            this.buttonOK.Name = "buttonOK";
            this.buttonOK.Size = new System.Drawing.Size(75, 23);
            this.buttonOK.TabIndex = 2;
            this.buttonOK.Text = "OK";
            this.buttonOK.UseVisualStyleBackColor = true;
            this.buttonOK.Click += new System.EventHandler(this.buttonOK_Click);
            // 
            // buttonClose
            // 
            this.buttonClose.Font = new System.Drawing.Font("Segoe UI", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonClose.Location = new System.Drawing.Point(328, 138);
            this.buttonClose.Name = "buttonClose";
            this.buttonClose.Size = new System.Drawing.Size(75, 23);
            this.buttonClose.TabIndex = 3;
            this.buttonClose.Text = "Close";
            this.buttonClose.UseVisualStyleBackColor = true;
            this.buttonClose.Click += new System.EventHandler(this.buttonClose_Click);
            // 
            // textBoxSource
            // 
            this.textBoxSource.Location = new System.Drawing.Point(133, 51);
            this.textBoxSource.Name = "textBoxSource";
            this.textBoxSource.Size = new System.Drawing.Size(199, 20);
            this.textBoxSource.TabIndex = 4;
            // 
            // textBoxTarget
            // 
            this.textBoxTarget.Location = new System.Drawing.Point(133, 91);
            this.textBoxTarget.Name = "textBoxTarget";
            this.textBoxTarget.Size = new System.Drawing.Size(199, 20);
            this.textBoxTarget.TabIndex = 5;
            // 
            // buttonBrowser
            // 
            this.buttonBrowser.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonBrowser.Location = new System.Drawing.Point(350, 51);
            this.buttonBrowser.Name = "buttonBrowser";
            this.buttonBrowser.Size = new System.Drawing.Size(53, 23);
            this.buttonBrowser.TabIndex = 6;
            this.buttonBrowser.Text = "...";
            this.buttonBrowser.UseVisualStyleBackColor = true;
            this.buttonBrowser.Click += new System.EventHandler(this.buttonBrowser_Click);
            // 
            // buttonReset
            // 
            this.buttonReset.Location = new System.Drawing.Point(191, 138);
            this.buttonReset.Name = "buttonReset";
            this.buttonReset.Size = new System.Drawing.Size(75, 23);
            this.buttonReset.TabIndex = 7;
            this.buttonReset.Text = "Reset";
            this.buttonReset.UseVisualStyleBackColor = true;
            this.buttonReset.Click += new System.EventHandler(this.buttonReset_Click);
            // 
            // PushDataConfigure
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(450, 196);
            this.Controls.Add(this.buttonReset);
            this.Controls.Add(this.buttonBrowser);
            this.Controls.Add(this.textBoxTarget);
            this.Controls.Add(this.textBoxSource);
            this.Controls.Add(this.buttonClose);
            this.Controls.Add(this.buttonOK);
            this.Controls.Add(this.labelTarget);
            this.Controls.Add(this.labelSource);
            this.MaximizeBox = false;
            this.Name = "PushDataConfigure";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Push Data Configuration";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label labelSource;
        private System.Windows.Forms.Label labelTarget;
        private System.Windows.Forms.Button buttonOK;
        private System.Windows.Forms.Button buttonClose;
        private System.Windows.Forms.TextBox textBoxSource;
        private System.Windows.Forms.TextBox textBoxTarget;
        private System.Windows.Forms.Button buttonBrowser;
        private System.Windows.Forms.Button buttonReset;
    }
}