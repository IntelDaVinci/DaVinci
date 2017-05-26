namespace DaVinci
{
    partial class SplashScreen
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
            this.backgroundWorker = new System.ComponentModel.BackgroundWorker();
            this.labelShowVer = new System.Windows.Forms.Label();
            this.ProgressBar = new DaVinci.CustomizedProgressBar();
            this.SuspendLayout();
            // 
            // backgroundWorker
            // 
            this.backgroundWorker.WorkerReportsProgress = true;
            this.backgroundWorker.WorkerSupportsCancellation = true;
            this.backgroundWorker.DoWork += new System.ComponentModel.DoWorkEventHandler(this.backgroundWorker_DoWork);
            this.backgroundWorker.ProgressChanged += new System.ComponentModel.ProgressChangedEventHandler(this.backgroundWorker_ProgressChanged);
            // 
            // labelShowVer
            // 
            this.labelShowVer.AutoSize = true;
            this.labelShowVer.BackColor = System.Drawing.Color.Transparent;
            this.labelShowVer.Font = new System.Drawing.Font("Segoe UI", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelShowVer.Location = new System.Drawing.Point(532, 9);
            this.labelShowVer.Name = "labelShowVer";
            this.labelShowVer.Size = new System.Drawing.Size(14, 15);
            this.labelShowVer.TabIndex = 1;
            this.labelShowVer.Text = "v";
            // 
            // ProgressBar
            // 
            this.ProgressBar.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(0)))), ((int)(((byte)(255)))), ((int)(((byte)(255)))), ((int)(((byte)(255)))));
            this.ProgressBar.Location = new System.Drawing.Point(0, 614);
            this.ProgressBar.Name = "ProgressBar";
            this.ProgressBar.Size = new System.Drawing.Size(624, 10);
            this.ProgressBar.Style = System.Windows.Forms.ProgressBarStyle.Continuous;
            this.ProgressBar.TabIndex = 0;
            // 
            // SplashScreen
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackgroundImage = global::DaVinci.Properties.Resources.DaVinci_Logo;
            this.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
            this.ClientSize = new System.Drawing.Size(624, 624);
            this.Controls.Add(this.labelShowVer);
            this.Controls.Add(this.ProgressBar);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.None;
            this.Name = "SplashScreen";
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "SplashScreen";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.ComponentModel.BackgroundWorker backgroundWorker;
        private CustomizedProgressBar ProgressBar;
        private System.Windows.Forms.Label labelShowVer;
    }
}