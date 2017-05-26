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

/*
 * Intel confidential -- do not distribute further
 * This source code is distributed covered by "Internal License Agreement -- For Internal Open Source DaVinci Program"
 * Please read and accept license.txt distributed with this package before using this source code
 */
using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using System.Drawing;
using System.Diagnostics;

namespace DaVinci
{
    /// <summary>
    /// Defines the entrance (both command line and GUI) of DaVinci
    /// </summary>
    public class Program
    {
        /// <summary>
        /// 
        /// </summary>
        public static bool consoleMode = false;

        private static DaVinciAPI.ImageEventHandler imageEventHandler;
        private static DaVinciAPI.TestStatusEventHandler testStatusHandler;
        private static DaVinciAPI.MessageEventHandler messageEventHandler;
        private static DaVinciAPI.DrawDotEventHandler drawDotEventHandler;
        private static DaVinciAPI.DrawLinesEventHandler drawLinesEventHandler;

        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static int Main(String[] args)
        {
            int retVal = 0;
            Assembly asm = System.Reflection.Assembly.GetExecutingAssembly();
            string version = AssemblyName.GetAssemblyName(asm.Location).Version.ToString();
            Console.WriteLine("DaVinci Version: " + AssemblyName.GetAssemblyName(asm.Location).Version + "\n\n");
            Application.ThreadException += new System.Threading.ThreadExceptionEventHandler(Application_ThreadException);
            string daVinciHome = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
            if (args.Length == 0)
            {
                if (!FolderPermissionCheck(daVinciHome))
                {
                    MessageBox.Show("No Access Permission for DaVinci Home, The Application will Exit.");
                    return retVal;
                }

                consoleMode = false;
                Thread initThread;
                initThread = (new Thread(() =>
                    {
                        retVal = DaVinciAPI.NativeMethods.Init(daVinciHome, args.Length, args);
                        //TODO: check the return of Init
                        //if (retVal != 0)
                        //{
                        //    Console.WriteLine("Unable to initialize DaVinci library!");
                        //    DaVinciAPI.NativeMethods.Shutdown();
                        //    return retVal;
                        //}
                    }));
                initThread.Name = "Init Thread";
                initThread.Start();
                using (var splashScreen = new SplashScreen(version))
                {
                    splashScreen.Show();
                    while (initThread.IsAlive)
                    {
                        Application.DoEvents();
                    }
                    if (!initThread.IsAlive)
                    {
                        splashScreen.SetInitEnd();
                        Thread.Sleep(1000);
                        splashScreen.Close();
                    }
                }

                Application.EnableVisualStyles();

                imageEventHandler = new DaVinciAPI.ImageEventHandler(DisplayImage);
                DaVinciAPI.NativeMethods.SetImageEventHandler(imageEventHandler);
                messageEventHandler = new DaVinciAPI.MessageEventHandler(DisplayMessage);
                DaVinciAPI.NativeMethods.SetMessageEventHandler(messageEventHandler);

                drawDotEventHandler = new DaVinciAPI.DrawDotEventHandler(drawDot);
                DaVinciAPI.NativeMethods.SetDrawDotHandler(drawDotEventHandler);

                drawLinesEventHandler = new DaVinciAPI.DrawLinesEventHandler(drawLines);
                DaVinciAPI.NativeMethods.SetDrawLinesHandler(drawLinesEventHandler);

                Application.Run(mainWindowInst);

                // Release native resources
                DaVinciAPI.NativeMethods.Shutdown();
            }
            else
            {
                if (!FolderPermissionCheck(daVinciHome))
                {
                    Console.WriteLine("No Access Permission for DaVinci Home, The Application will Exit.\n");
                    return retVal; 
                }

                Console.CancelKeyPress += new ConsoleCancelEventHandler(CancelKeyHandler);
                consoleMode = true;
                retVal = DaVinciAPI.NativeMethods.Init(daVinciHome, args.Length, args);
                if (retVal != 0)
                {
                    Console.WriteLine("Unable to initialize DaVinci library!");
                    DaVinciAPI.NativeMethods.Shutdown();
                    return retVal;
                }

                DaVinciAPI.TestConfiguration testConfiguration = DaVinciAPI.NativeMethods.GetTestConfiguration();

                testStatusHandler = new DaVinciAPI.TestStatusEventHandler(TestStatusEventHandler);
                DaVinciAPI.NativeMethods.SetTestStatusEventHandler(testStatusHandler);

                if (testConfiguration.suppressImageBox == DaVinciAPI.BoolType.BoolFalse)
                {
                    OpenImageViewer();
                    imageEventHandler = new DaVinciAPI.ImageEventHandler(DisplayImage);
                    DaVinciAPI.NativeMethods.SetImageEventHandler(imageEventHandler);
                }

                // start the specified test from command line
                DaVinciAPI.NativeMethods.StartTest("");
                WaitForTestStop();
                DaVinciAPI.NativeMethods.Shutdown();

                if (testConfiguration.suppressImageBox == DaVinciAPI.BoolType.BoolFalse)
                {
                    CloseImageViewer();
                }

                Console.CancelKeyPress -= new ConsoleCancelEventHandler(CancelKeyHandler);
            }
            return retVal;
        }

        static void CancelKeyHandler(object sender, ConsoleCancelEventArgs args)
        {
            Console.WriteLine("Received Control+C, canceling test...");
            DaVinciAPI.NativeMethods.StopTest();
            args.Cancel = true;
        }

        /// <summary>
        /// The exception handler for all unhandled exceptions in this application.
        /// This is added in order to avoid the situation that the crash of QScript Editor breaks the whole DaVinci.
        /// </summary>
        static void Application_ThreadException(object sender, System.Threading.ThreadExceptionEventArgs e)
        {
            if (e.Exception.Source.IndexOf("QScriptEditor") != -1)
                MessageBox.Show(e.Exception.Message + "\nYou can ignore it or restart the Editor. \nContact DaVinci developers if it appears again.", "QScript Editor Exception", MessageBoxButtons.OK, MessageBoxIcon.Error);
            else
                throw e.Exception;
        }

        static AutoResetEvent testStopEvent = new AutoResetEvent(false);

        static void WaitForTestStop()
        {
            while (DaVinciAPI.NativeMethods.GetTestStatus() != DaVinciAPI.TestStatus.TestStatusStopped)
                testStopEvent.WaitOne(1000);
        }

        static void TestStatusEventHandler(DaVinciAPI.TestStatus status)
        {
            if (status == DaVinciAPI.TestStatus.TestStatusStopped)
            {
                testStopEvent.Set();
            }
        }

        /// <summary>
        /// 
        /// </summary>
        public static MainWindow_V mainWindowInst = new MainWindow_V();
        static void DisplayImage(ref DaVinciAPI.ImageInfo image)
        {
            // console mode
            if (consoleMode)
            {
                ShowImageInViewer(image);
            }
            else
            {
                mainWindowInst.ShowImgInMainWindow(image);
            }
        }

        static void drawDot(ref DaVinciAPI.ActionDotInfo dot)
        {
            var drawBrush = new SolidBrush(Color.Red);
            mainWindowInst.DrawDotOnImageBox(drawBrush, dot.coorX, dot.coorY, dot.orientation, 10);
        }

        static DaVinciAPI.ActionDotInfo getCopiedDotInfo(ref DaVinciAPI.ActionDotInfo dot)
        {
            DaVinciAPI.ActionDotInfo dotRtn;
            dotRtn.coorX = dot.coorX;
            dotRtn.coorY = dot.coorY;
            dotRtn.orientation = dot.orientation;
            return dotRtn;
        }

        static void drawLines(ref DaVinciAPI.ActionDotInfo dot1, ref DaVinciAPI.ActionDotInfo dot2, int width, int height)
        {
            DaVinciAPI.ActionDotInfo dot1Cpy = getCopiedDotInfo(ref dot1);
            DaVinciAPI.ActionDotInfo dot2Cpy = getCopiedDotInfo(ref dot2);
            var drawBrush = new SolidBrush(Color.Red);
            var myPen = new Pen(drawBrush, 5);
            mainWindowInst.DrawLinesOnImageBox(myPen, dot1Cpy, dot2Cpy, width, height);
        }


        static void DisplayMessage(string message)
        {
            if (!consoleMode)
            {
                mainWindowInst.UpdateText(message);
            }
        }

        static ImageViewer imageViewer = new ImageViewer();

        static Thread showViewerThread;

        static void ShowDialog()
        {
            Application.Run(imageViewer);
        }

        /// <summary>
        /// 
        /// </summary>
        static void OpenImageViewer()
        {
            imageViewer.Height = 787 + 45;
            imageViewer.Width = 447 + 18;

            imageViewer.StartPosition = FormStartPosition.Manual;
            imageViewer.SetDesktopLocation(800, 20);
            imageViewer.ImageBox.SizeMode = PictureBoxSizeMode.Normal;

            const int maxSize = 1024;
            StringBuilder deviceName = new StringBuilder(maxSize);
            if (DaVinciAPI.NativeMethods.GetCurrentTargetDevice(deviceName, maxSize) == DaVinciAPI.BoolType.BoolTrue)
                imageViewer.Text += " - " + deviceName.ToString();

            showViewerThread = new Thread(new ThreadStart(ShowDialog));
            showViewerThread.Name = "ShowViewerThread";
            showViewerThread.Start();
            while (!imageViewer.IsHandleCreated)
            {
                Thread.Sleep(10);
            }
        }

        static void CloseImageViewer()
        {
            // console mode
            if (imageViewer != null && imageViewer.IsDisposed == false)
                imageViewer.Invoke(new Action(() => { imageViewer.Close(); }));
            showViewerThread.Join();
        }

        /// <summary>
        /// 
        /// </summary>
        public static int displayCapturedImageLock = 0;

        // Show image in image viewer
        static void ShowImageInViewer(DaVinciAPI.ImageInfo image)
        {
            if (imageViewer != null && imageViewer.ImageBox != null)
            {
                if (imageViewer.ImageBox.InvokeRequired && !imageViewer.ImageBox.IsDisposed)
                {
                    if (Interlocked.CompareExchange(ref displayCapturedImageLock, 1, 0) == 0)
                    {
                        imageViewer.ImageBox.BeginInvoke((Action<DaVinciAPI.ImageInfo>)ShowImageInViewer, image);
                    }
                    else
                    {
                        DaVinciAPI.NativeMethods.ReleaseImage(ref image);
                    }
                }
                else
                {
                    if (imageViewer.ImageBox.IsDisposed == false)
                    {
                        imageViewer.ImageBox.Image = image;
                    }
                    Interlocked.Exchange(ref displayCapturedImageLock, 0);
                }
            }
        }

        static bool FolderPermissionCheck(string dirname)
        {
            try
            {
                if (!Directory.Exists(dirname))
                {
                    Console.WriteLine("DaVinci Home: " + dirname + " Not Exists.\n\n");
                    throw new Exception();  
                }
                else
                {
                    string testfile = dirname + "\\" + Path.GetRandomFileName(); 

                    using (FileStream fstream = new FileStream(testfile, FileMode.Create))
                    using (TextWriter writer = new StreamWriter(fstream))
                    {
                        writer.WriteLine("hello!");
                    }

                    File.Delete(testfile);
                }
            }
            catch
            {
                return false;
            }
            return true;
        }
    }
}

