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
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace DaVinci
{
    class TimeStampFileReader
    {
        private string fileName;
        private List<double> timeStampList;
        private List<traceInfo> qsTrace;

        /// <summary>
        /// Type of trace information used in qts file for trace debug.
        /// </summary>
        public struct traceInfo
        {
            /// <summary>
            /// label or line number
            /// </summary>
            public int label;
            /// <summary>
            /// absolute time 
            /// </summary>
            public double time;
        }

        private List<string> GetTraceListFromQs(string qsFile)
        {
            String[] qs_lines = null;
            try
            {
                qs_lines = System.IO.File.ReadAllLines(qsFile);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
            }
            List<string> tempTraceList = qs_lines.ToList().FindAll(s => s.Length > 0 && s[0] >= '0' && s[0] <= '9');
            List<string> rtnTraceList = new List<string>();
            for (int i = 0; i < tempTraceList.Count; i++)
            {
                string tempStr = ":" + Regex.Split(tempTraceList[i], @"\s+")[0].ToString() + ":" + Regex.Split(tempTraceList[i],@"\s+")[1].ToString();
                tempStr = tempStr.Substring(0, tempStr.Length - 1);
                rtnTraceList.Insert(i, tempStr); 
            }
            return rtnTraceList;
        }
        /// <summary>
        /// Analyze the time stamp file
        /// </summary>
        /// <param name="timeStampFile"> the file path </param>
        public TimeStampFileReader(string timeStampFile)
        {
            this.fileName = timeStampFile;
            bool oldVersion = false;

            String[] qts_lines;
            try
            {
                qts_lines = System.IO.File.ReadAllLines(timeStampFile);
            }
            catch
            {
                return;
            }
            if (qts_lines[0].IndexOf("Version") < 0)
                oldVersion = true;
            if (qts_lines.Length >= 2 && qts_lines[1] == "")
            {
                String[] filtered_qts_lines = new String[qts_lines.Length / 2];
                for (int i = 0; i < qts_lines.Length / 2; i++)
                    filtered_qts_lines[i] = qts_lines[i * 2];
                this.timeStampList = filtered_qts_lines.ToList().FindAll(s => s.Length > 0 && s[0] >= '0' && s[0] <= '9').ConvertAll(s => double.Parse(s));
            }
            else
            {
                this.timeStampList = new List<double>();
                List<String> qts_time_stamp_lines = qts_lines.ToList().FindAll(s => s.Length > 0 && s[0] >= '0' && s[0] <= '9');
                foreach (String time_stamp_line in qts_time_stamp_lines)
                {
                    String[] time_stamp_line_items = time_stamp_line.Split(' ');
                    System.Diagnostics.Debug.Assert(time_stamp_line_items.Length > 0, "The QTS file is invalid. Please check...");

                    double time_stamp_value = Convert.ToDouble(time_stamp_line_items[0]);

                    this.timeStampList.Add(time_stamp_value);
                }
            }

            List<string> traceList = qts_lines.ToList().FindAll(s => s.Length > 0 && s[0] == ':');
            // If trace cannot be extract from qts file, then try to extract them from qs file
            if (traceList.Count == 0)
            {
                string qsFile = timeStampFile.Replace(".qts", ".qs");
                traceList = GetTraceListFromQs(qsFile);
            }
            if (oldVersion)
            {
                for (int i = 0; i < this.timeStampList.Count; i++)
                    this.timeStampList[i] = this.timeStampList[i] / ((double)TimeSpan.TicksPerMillisecond);
            }
            qsTrace = null;
            if (traceList.Count > 0)
                qsTrace = new List<traceInfo>();
            for (int i = 0; i < traceList.Count; i++)
            {
                string[] ss = traceList[i].Split(new char[] { ':' });
                traceInfo ti;
                if (ss.Length >= 3 && Int32.TryParse(ss[1], out ti.label) && double.TryParse(ss[2], out ti.time))
                {
                    if (oldVersion)
                        ti.time = ti.time / ((double)TimeSpan.TicksPerMillisecond);
                    if (qsTrace != null)
                        qsTrace.Add(ti);
                }
                else
                    continue;
            }
        }

        /// <summary>
        /// get the time stamp list
        /// </summary>
        /// <returns></returns>
        public List<double> getTimeStampList()
        {
            return this.timeStampList;
        }

        /// <summary>
        /// get the opcode execution trace 
        /// </summary>
        /// <returns></returns>
        public List<traceInfo> getTrace()
        {
            return this.qsTrace;
        }
    }
}
