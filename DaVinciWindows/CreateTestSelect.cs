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
    public partial class CreateTestSelect : Form
    {
        /// <summary>
        /// 
        /// </summary>
        public CreateTestSelect()
        {
            InitializeComponent();
        }

        private void CancelButton_Click(object sender, EventArgs e)
        {
            MainWindow_V.testSelectForm.Close();
        }

        /// <summary>
        /// 
        /// </summary>
        public static RnRTestForm rnrForm;
        /// <summary>
        /// 
        /// </summary>
        public static FPSTestForm fpsForm;
        /// <summary>
        /// 
        /// </summary>
        public static launchTimeTestForm launchTimeForm;
        /// <summary>
        /// 
        /// </summary>
        public static GetRnRWizard getRnRWizard;

        private void SeTestTypeSelectButton_Click(object sender, EventArgs e)
        {
            if (radioButtonRnRButton.Checked == true)
            {
                MainWindow_V.testSelectForm.Close();
                rnrForm = new RnRTestForm();
                rnrForm.ShowDialog();
            }
            else if (radioButtonFPS.Checked == true)
            {
                MainWindow_V.testSelectForm.Close();
                fpsForm = new FPSTestForm();
                fpsForm.ShowDialog();
            }
            else if (radioButtonLaunchTime.Checked == true)
            {
                MainWindow_V.testSelectForm.Close();
                launchTimeForm = new launchTimeTestForm();
                launchTimeForm.ShowDialog();
            }
        }
    }
}
