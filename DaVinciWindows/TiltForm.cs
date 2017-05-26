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
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace DaVinci
{
    /// <summary>
    /// 
    /// </summary>
    public partial class TiltForm : Form
    {
        /// <summary>
        /// 
        /// </summary>
        public TiltForm()
        {
            InitializeComponent();
        }

        private void buttonOK_Click(object sender, EventArgs e)
        {
            string input1 = this.textBoxAngle1.Text;
            string input2 = this.textBoxAngle2.Text;
            int angle1 = 15, angle2 = 15;
            bool is_int = int.TryParse(input1, out angle1);
            if (!is_int)
            {
                MessageBox.Show("Invalid parameter Angle1! Will set this value to 15 automatically!",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation
                    );
                angle1 = 15;
            }

            is_int = int.TryParse(input2, out angle2);
            if (!is_int) 
            {
                MessageBox.Show("Invalid parameter Angle2! Will set this value to 15 automatically!",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
                angle2 = 15;
            }

            DaVinciAPI.NativeMethods.OnTilt(DaVinciAPI.TiltAction.TiltActionTo, angle1, angle2);
            this.Close();
        }

        private void buttonCancel_Click(object sender, EventArgs e)
        {
            this.Close();
        }
    }
}
