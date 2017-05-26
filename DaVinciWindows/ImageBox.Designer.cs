namespace DaVinci
{
    partial class ImageBox
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
            if (disposing)
            {
                DaVinciAPI.NativeMethods.ReleaseImage(ref imageToDraw);
                if (components != null)
                {
                    components.Dispose();
                }
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
            this.SuspendLayout();
            // 
            // ImageBox
            // 
            this.ClientSize = new System.Drawing.Size(282, 439);
            this.Name = "ImageBox";
            this.Text = "ImageBox";
            this.ResumeLayout(false);

        }

        #endregion
    }
}