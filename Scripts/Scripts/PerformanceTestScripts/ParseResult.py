import sys
import os
import optparse
import re
import datetime
import time
import logging.config
import stat
import xlsxwriter
from xml.etree import ElementTree
from collections import OrderedDict
from Helper import PrintAndLogInfo, copyFile, deleteFile

def AppendAverageScores(check_points, all_score):
    dic_score = OrderedDict()

    # init each checkpoint for dic
    for check_point in check_points:
        dic_score[check_point] = []

    # append each round scores in dic.v
    for item in all_score:
        for (k, v) in item.items():
            dic_score[k].append(v)

    # get the average data
    for (k, v) in dic_score.items():
        sum = 0
        num = 0
        for each_number in v:
            try:
                float_each_number = float(each_number)
                sum = sum + float_each_number
                num = num + 1
            except:
                continue
        if num == 0:
            average_score = ""
        else:
            average_score = round(float(sum/num), 2)
        dic_score[k].append(average_score)
    return dic_score


def CFBenchAntutuCaffeineResult(all_test_info, output_info):
    all_cfbench_score = []
    all_antutu_score = []
    all_caffeinemark_score = []

    cfbench_check_points = []
    antutu_check_points = []
    caffeinemark_check_points = []

    total_cfbench_score = OrderedDict()
    total_antutu_score = OrderedDict()
    total_caffeinemark_score = OrderedDict()

    for one_test_info in all_test_info:
        if (one_test_info["TestName"].lower().strip() == "cfbench"):
            cfbench_check_points = one_test_info["CheckPoints"]
        elif (one_test_info["TestName"].lower().strip() == "antutu"):
            antutu_check_points = one_test_info["CheckPoints"]
        elif (one_test_info["TestName"].lower().strip() == "caffeinemark"):
            caffeinemark_check_points = one_test_info["CheckPoints"]

    for one_output_info in output_info:
        dic_cfbench_score = {}
        dic_antutu_score = {}
        dic_caffeinemark_score = {}
        
        if (one_output_info["TestName"].lower().strip() == "cfbench"):
            for result_files in one_output_info["OutputFiles"].values():
                dic_cfbench_score = ParseCFBenchResult(result_files, cfbench_check_points)      # parse CFBench result
                all_cfbench_score.append(dic_cfbench_score)
        elif (one_output_info["TestName"].lower().strip() == "antutu"):
            for result_files in one_output_info["OutputFiles"].values():
                dic_antutu_score = ParseAntutuResult(result_files, antutu_check_points)         # parse Antutu result
                all_antutu_score.append(dic_antutu_score)
        elif (one_output_info["TestName"].lower().strip() == "caffeinemark"):
            for result_files in one_output_info["OutputFiles"].values():
                dic_caffeinemark_score = ParseCaffeinemarkResult(result_files, caffeinemark_check_points)   # parse Caffeinemark result
                all_caffeinemark_score.append(dic_caffeinemark_score)

    # get average score for each key
    if len(all_cfbench_score) > 0:
        total_cfbench_score = AppendAverageScores(cfbench_check_points, all_cfbench_score)        
    if len(all_antutu_score) > 0:
        total_antutu_score = AppendAverageScores(antutu_check_points, all_antutu_score)        
    if len(all_caffeinemark_score) > 0:
        total_caffeinemark_score = AppendAverageScores(caffeinemark_check_points, all_caffeinemark_score)
          
    return total_cfbench_score, total_antutu_score, total_caffeinemark_score


def oxAndeAndroBenchResult(all_test_info, output_info):
    all_oxbench_score = []
    all_andebench_score = []
    all_androbench_score = []

    oxbench_check_points = []
    andebench_check_points = []
    androbench_check_points = []

    total_oxbench_score = OrderedDict()
    total_andebench_score = OrderedDict()
    total_androbench_score = OrderedDict()

    for one_test_info in all_test_info:
        if (one_test_info["TestName"].lower().strip() == "oxbench"):
            oxbench_check_points = one_test_info["CheckPoints"]
        elif (one_test_info["TestName"].lower().strip() == "andebench"):
            andebench_check_points = one_test_info["CheckPoints"]
        elif (one_test_info["TestName"].lower().strip() == "androbench"):
            androbench_check_points = one_test_info["CheckPoints"]

    for one_output_info in output_info: 
        dic_oxbench_score = {}
        dic_andebench_score = {}
        dic_androbench_score = {}

        if (one_output_info["TestName"].lower().strip() == "oxbench"):
            for result_files in one_output_info["OutputFiles"].values():
                dic_oxbench_score = Parse0xbenchResult(result_files, oxbench_check_points)   # parse 0xbench result
                all_oxbench_score.append(dic_oxbench_score)
        elif (one_output_info["TestName"].lower().strip() == "andebench"):
            for result_files in one_output_info["OutputFiles"].values():
                dic_andebench_score = ParseAndEBenchResult(result_files, andebench_check_points)   # parse AndEBench result
                all_andebench_score.append(dic_andebench_score)
        elif (one_output_info["TestName"].lower().strip() == "androbench"):
            for result_files in one_output_info["OutputFiles"].values():
                dic_androbench_score = ParseAndroBenchResult(result_files, androbench_check_points)   # parse AndroBench result
                all_androbench_score.append(dic_androbench_score)

    if len(all_oxbench_score) > 0:
        total_oxbench_score = AppendAverageScores(oxbench_check_points, all_oxbench_score)
    if len(all_andebench_score) > 0:
        total_andebench_score = AppendAverageScores(andebench_check_points, all_andebench_score)
    if len(all_androbench_score) > 0:
        new_androbench_check_points = androbench_check_points
        new_androbench_check_points.append("new_Random Read")
        new_androbench_check_points.append("new_Random Write")
        new_androbench_check_points.append("new_SQLite Insert")
        new_androbench_check_points.append("new_SQLite Update")
        new_androbench_check_points.append("new_SQLite Delete")
        total_new_androbench_score = AppendAverageScores(new_androbench_check_points, all_androbench_score)
        total_androbench_score = GenerateAndroBenchScore(total_new_androbench_score, andebench_check_points)
        
    return total_oxbench_score, total_andebench_score, total_androbench_score


def piMonjoriEzbenchResult(all_test_info, output_info):
    all_pi_score = []
    all_monjori_score = []
    all_ezbench_score = []

    pi_check_points = []
    monjori_check_points = []
    ezbench_check_points = []

    total_pi_score = OrderedDict()
    total_monjori_score = OrderedDict()
    total_ezbench_score = OrderedDict()

    for one_test_info in all_test_info:
        if (one_test_info["TestName"].lower().strip() == "pi"):
            pi_check_points = one_test_info["CheckPoints"]
        elif (one_test_info["TestName"].lower().strip() == "monjori"):
            monjori_check_points = one_test_info["CheckPoints"]
        elif (one_test_info["TestName"].lower().strip() == "ezbench_arm"):
            ezbench_check_points = one_test_info["CheckPoints"]

    for one_output_info in output_info:
        dic_pi_score = {}
        dic_monjori_score = {}
        dic_ezbench_score = {}

        if (one_output_info["TestName"].lower().strip() == "pi"):
            for result_files in one_output_info["OutputFiles"].values():
                dic_pi_score = ParsePiResult(result_files, pi_check_points)                 # parse Pi result
                all_pi_score.append(dic_pi_score)
        elif (one_output_info["TestName"].lower().strip() == "monjori"):
            for result_files in one_output_info["OutputFiles"].values():
                dic_monjori_score = ParseMonjoriOrEzBenchResult(result_files, monjori_check_points)  # parse Monjori result
                all_monjori_score.append(dic_monjori_score)
        elif (one_output_info["TestName"].lower().strip() == "ezbench_arm"):
            for result_files in one_output_info["OutputFiles"].values():
                dic_ezbench_score = ParseMonjoriOrEzBenchResult(result_files, ezbench_check_points)  # parse EzBench result
                all_ezbench_score.append(dic_ezbench_score)

    if len(all_pi_score) > 0:
        total_pi_score = AppendAverageScores(pi_check_points, all_pi_score)
    if len(all_monjori_score) > 0:
        total_monjori_score = AppendAverageScores(monjori_check_points, all_monjori_score)
    if len(all_ezbench_score) > 0:
        total_ezbench_score = AppendAverageScores(ezbench_check_points, all_ezbench_score)

    return total_pi_score, total_monjori_score, total_ezbench_score


def nenamarkQuakeiiiThreedmarkResult(all_test_info, output_info):
    all_nenamarkone_score = []
    all_nenamarktwo_score = []
    all_quakeiii_score = []
    all_threedmark_score = []
    
    nenamarkone_check_points = []
    nenamarktwo_check_points = []
    quakeiii_check_points = []
    threedmark_check_points = []

    total_nenamarkone_score = OrderedDict()
    total_nenamarktwo_score = OrderedDict()
    total_quakeiii_score = OrderedDict()
    total_threedmark_score = OrderedDict()

    for one_test_info in all_test_info:
        if (one_test_info["TestName"].lower().strip() == "nenamarkone"):
            nenamarkone_check_points = one_test_info["CheckPoints"]
        elif (one_test_info["TestName"].lower().strip() == "nenamarktwo"):
            nenamarktwo_check_points = one_test_info["CheckPoints"]
        elif (one_test_info["TestName"].lower().strip() == "quakeiii"):
            quakeiii_check_points = one_test_info["CheckPoints"]
        elif (one_test_info["TestName"].lower().strip() == "threedmark"):
            threedmark_check_points = one_test_info["CheckPoints"]

    for one_output_info in output_info:                
        dic_nenamarkone_score = {}
        dic_nenamarktwo_score = {}
        dic_quakeiii_score = {}
        dic_threedmark_score ={}
    
        if (one_output_info["TestName"].lower().strip() == "nenamarkone"):
            for result_files in one_output_info["OutputFiles"].values():
                dic_nenamarkone_score = ParseNenaMarkResult(result_files, nenamarkone_check_points)  # parse NenaMarkOne result
                all_nenamarkone_score.append(dic_nenamarkone_score)
        elif (one_output_info["TestName"].lower().strip() == "nenamarktwo"):
            for result_files in one_output_info["OutputFiles"].values():
                dic_nenamarktwo_score = ParseNenaMarkResult(result_files, nenamarktwo_check_points)  # parse NenaMarkTwo result
                all_nenamarktwo_score.append(dic_nenamarktwo_score)
        elif (one_output_info["TestName"].lower().strip() == "quakeiii"):
            for result_files in one_output_info["OutputFiles"].values():
                dic_quakeiii_score = ParseQuakeIIIResult(result_files, quakeiii_check_points)        # parse QuakeIII result
                all_quakeiii_score.append(dic_quakeiii_score)
        elif (one_output_info["TestName"].lower().strip() == "threedmark"):
            for result_files in one_output_info["OutputFiles"].values():
                dic_threedmark_score = ParseThreeDMarkResult(result_files, threedmark_check_points)   # parse 3DMark result
                all_threedmark_score.append(dic_threedmark_score)

    if len(all_nenamarkone_score) > 0:
        total_nenamarkone_score = AppendAverageScores(nenamarkone_check_points, all_nenamarkone_score)
    if len(all_nenamarktwo_score) > 0:
        total_nenamarktwo_score = AppendAverageScores(nenamarktwo_check_points, all_nenamarktwo_score)
    if len(all_quakeiii_score) > 0:
        total_quakeiii_score = AppendAverageScores(quakeiii_check_points, all_quakeiii_score)
    if len(all_threedmark_score) > 0:
        total_threedmark_score = AppendAverageScores(threedmark_check_points, all_threedmark_score)
        
    return total_nenamarkone_score, total_nenamarktwo_score, total_quakeiii_score, total_threedmark_score


def GenerateBenchmarkResult(script_path, all_test_info, output_info, timestamp_nm):
    dic_final_result = {}
    
    # get CFBench, Antutu, Caffeinemark result
    total_cfbench_score, total_antutu_score, total_caffeinemark_score = CFBenchAntutuCaffeineResult(all_test_info, output_info)
    dic_final_result["CFBench"] = total_cfbench_score
    dic_final_result["Antutu"] = total_antutu_score
    dic_final_result["Caffeinemark-FlexyCore"] = total_caffeinemark_score

    # get 0xBench, AndEBench, AndroBench result
    total_oxbench_score, total_andebench_score, total_androbench_score = oxAndeAndroBenchResult(all_test_info, output_info)
    dic_final_result["0xBench"] = total_oxbench_score
    dic_final_result["AndEBench"] = total_andebench_score
    dic_final_result["AndroBench"] = total_androbench_score

    # get Pi, Monjori, EZBench result
    total_pi_score, total_monjori_score, total_ezbench_score = piMonjoriEzbenchResult(all_test_info, output_info)
    dic_final_result["Pi"] = total_pi_score
    dic_final_result["Monjori"] = total_monjori_score
    dic_final_result["EZBench"] = total_ezbench_score

    # get NenaMarkOne, NenaMarkTwo, QuakeIII, 3DMark result
    total_nenamarkone_score, total_nenamarktwo_score, total_quakeiii_score, total_threedmark_score = nenamarkQuakeiiiThreedmarkResult(all_test_info, output_info)
    dic_final_result["NenaMarkOne"] = total_nenamarkone_score
    dic_final_result["NenaMarkTwo"] = total_nenamarktwo_score
    dic_final_result["QuakeIII"] = total_quakeiii_score
    dic_final_result["3DMark"] = total_threedmark_score

    MergeAllResults(script_path, timestamp_nm, dic_final_result)         # merge all the results

    MoveResult(script_path, timestamp_nm, output_info)            # move all the result into one folder


def GenerateAndroBenchScore(total_new_androbench_score, androbench_check_points):
    dic_score = OrderedDict()
    each_key = "Sequential Read"
    each_value = total_new_androbench_score[each_key]
    index = 0
    while (index < len(each_value)):
        each_value[index] = (str)(each_value[index]) + " MB/s"
        index = index + 1
    dic_score[each_key] = each_value

    each_key = "Sequential Write"
    each_value = total_new_androbench_score[each_key]
    index = 0
    while (index < len(each_value)):
        each_value[index] = (str)(each_value[index]) + " MB/s"
        index = index + 1
    dic_score[each_key] = each_value

    each_key1 = "Random Read"
    each_value1 = total_new_androbench_score[each_key1]
    each_key2 = "new_Random Read"
    each_value2 = total_new_androbench_score[each_key2]
    index = 0
    while index < len(each_value1) and index < len(each_value2):
        each_value1[index] = (str)(each_value1[index]) + " MB/s, " + (str)(each_value2[index]) + " IOPS(4K)"
        index = index + 1
    dic_score[each_key1] = each_value1

    each_key1 = "Random Write"
    each_value1 = total_new_androbench_score[each_key1]
    each_key2 = "new_Random Write"
    each_value2 = total_new_androbench_score[each_key2]
    index = 0
    while index < len(each_value1) and index < len(each_value2):
        each_value1[index] = (str)(each_value1[index]) + " MB/s, " + (str)(each_value2[index]) + " IOPS(4K)"
        index = index + 1
    dic_score[each_key1] = each_value1

    each_key1 = "SQLite Insert"
    each_value1 = total_new_androbench_score[each_key1]
    each_key2 = "new_SQLite Insert"
    each_value2 = total_new_androbench_score[each_key2]
    index = 0
    while index < len(each_value1) and index < len(each_value2):
        each_value1[index] = (str)(each_value1[index]) + " TPS, " + (str)(each_value2[index]) + " sec"
        index = index + 1
    dic_score[each_key1] = each_value1

    each_key1 = "SQLite Update"
    each_value1 = total_new_androbench_score[each_key1]
    each_key2 = "new_SQLite Update"
    each_value2 = total_new_androbench_score[each_key2]
    index = 0
    while index < len(each_value1) and index < len(each_value2):
        each_value1[index] = (str)(each_value1[index]) + " TPS, " + (str)(each_value2[index]) + " sec"
        index = index + 1
    dic_score[each_key1] = each_value1

    each_key1 = "SQLite Delete"
    each_value1 = total_new_androbench_score[each_key1]
    each_key2 = "new_SQLite Delete"
    each_value2 = total_new_androbench_score[each_key2]
    index = 0
    while index < len(each_value1) and index < len(each_value2):
        each_value1[index] = (str)(each_value1[index]) + " TPS, " + (str)(each_value2[index]) + " sec"
        index = index + 1
    dic_score[each_key1] = each_value1

    each_key = "Browser"
    each_value = total_new_androbench_score[each_key]
    index = 0
    while (index < len(each_value)):
        each_value[index] = (str)(each_value[index]) + " msec"
        index = index + 1
    dic_score[each_key] = each_value

    each_key = "Market"
    each_value = total_new_androbench_score[each_key]
    index = 0
    while (index < len(each_value)):
        each_value[index] = (str)(each_value[index]) + " msec"
        index = index + 1
    dic_score[each_key] = each_value

    each_key = "Camera"
    each_value = total_new_androbench_score[each_key]
    index = 0
    while (index < len(each_value)):
        each_value[index] = (str)(each_value[index]) + " msec"
        index = index + 1
    dic_score[each_key] = each_value

    each_key = "Camcorder"
    each_value = total_new_androbench_score[each_key]
    index = 0
    while (index < len(each_value)):
        each_value[index] = (str)(each_value[index]) + " msec"
        index = index + 1
    dic_score[each_key] = each_value
    return dic_score

def ParseThreeDMarkResult(result_files, check_points):
    dic_score = OrderedDict()
    index = 0 
    while (index < len(check_points)):
        dic_score[check_points[index]] = "NA"
        index = index + 1

    for xml_file_name in result_files:
        all_text = []
        pre_name = xml_file_name.replace(".xml","").split("_")[-1]
        root = ElementTree.parse(xml_file_name).getroot()
        all_nodes = root.findall('./node/node/node/node/node/node/node/node/node')
        for one_node in all_nodes:
            for child in one_node.getchildren():
                for key in child.attrib:
                    if (key == "content-desc"):
                        all_text.append(child.attrib[key].replace("( --- )","").replace(" ( --- FPS)","").strip())

        index = 0
        while (index < len(all_text)):
            for check_point in check_points:
                if check_point.replace(pre_name + ": ", "") in all_text[index]:
                    dic_score[check_point] = all_text[index+1].strip()
            index = index + 1
    return dic_score


def ParseQuakeIIIResult(result_files, check_points):
    dic_score = OrderedDict()
    index = 0 
    while (index < len(check_points)):
        dic_score[check_points[index]] = "NA"
        index = index + 1

    for xml_file_name in result_files:
        xml_file = open(xml_file_name)
        for line in xml_file:
            contents = line.split("fps")
            if (len(contents) > 1):
                pre_contents = contents[0].split("seconds")
                if (len(pre_contents) > 1):
                    dic_score["FPS"] = pre_contents[1].strip()
                    break
        xml_file.close()
    return dic_score

def ParseNenaMarkResult(result_files, check_points):
    dic_score = OrderedDict()
    all_text = []
    index = 0 
    while (index < len(check_points)):
        dic_score[check_points[index]] = "NA"
        index = index + 1

    for xml_file_name in result_files:
        root = ElementTree.parse(xml_file_name).getroot()
        all_nodes = root.findall('./node/node/node/node/node/node')
        for one_node in all_nodes:
            for child in one_node.getchildren():
                for key in child.attrib:
                    if (key == "text"):
                        all_text.append(child.attrib[key].replace("Model","").strip())

    index = 0
    while (index < len(all_text)):
        for check_point in check_points:
            if check_point in all_text[index]:
                one_text = all_text[index].split(":")
                if (len(one_text) > 1):
                    dic_score[check_point] = one_text[1].strip()
        index = index + 1
    return dic_score

def ParseMonjoriOrEzBenchResult(result_files, check_points):
    dic_score = OrderedDict()
    all_text = []
    index = 0 
    while (index < len(check_points)):
        dic_score[check_points[index]] = "NA"
        index = index + 1

    for xml_file_name in result_files:
        root = ElementTree.parse(xml_file_name).getroot()
        all_nodes = root.findall('./node/node/node/node')
        for one_node in all_nodes:
            for child in one_node.getchildren():
                for key in child.attrib:
                    if (key == "text"):
                        all_text.append(child.attrib[key].strip())

    index = 0
    while (index < len(all_text)):
        for check_point in check_points:
            if check_point in all_text[index]:
                one_text = all_text[index].split(":")
                if (len(one_text) > 1):
                    dic_score[check_point] = one_text[1].strip()
        index = index + 1
    return dic_score

def ParsePiResult(result_files, check_points):
    dic_score = OrderedDict()
    all_text = []
    index = 0 
    while (index < len(check_points)):
        dic_score[check_points[index]] = "NA"
        index = index + 1

    for xml_file_name in result_files:
        root = ElementTree.parse(xml_file_name).getroot()
        all_nodes = root.findall('./node/node/node/node/node/node/node')
        for one_node in all_nodes:
            for child in one_node.getchildren():
                for key in child.attrib:
                    if (key == "text"):
                        all_text.append(child.attrib[key].strip())

    index = 0
    while (index < len(all_text)):
        if all_text[index] in check_points:
            one_text = all_text[index+1].replace("s", "")
            dic_score[all_text[index]] = one_text.strip()
        index = index + 1
    return dic_score


def ParseAndroBenchResult(result_files, androbench_check_points):
    dic_androbench_score = OrderedDict()
    all_text = []
    index = 0 
    while (index < len(androbench_check_points)):
        dic_androbench_score[androbench_check_points[index]] = "NA"
        index = index + 1

    for xml_file_name in result_files:
        root = ElementTree.parse(xml_file_name).getroot()
        all_nodes = root.findall('./node/node/node/node/node/node/node/node/node/node/node/node/node/node')
        for one_node in all_nodes:
            for child in one_node.getchildren():
                for key in child.attrib:
                    if (key == "text"):
                        all_text.append(child.attrib[key].strip())
    index = 0
    while (index < len(all_text) - 1):
        if all_text[index] in androbench_check_points:
            one_text = all_text[index+1].replace("MB/s", "").replace("IOPS(4K)", "").replace("TPS", "").replace("msec","").replace("sec", "").strip()
            if "," not in one_text:
                dic_androbench_score[all_text[index]] = one_text.strip()
            else:
                dic_androbench_score[all_text[index]] = one_text.split(",")[0].strip()
                dic_androbench_score["new_" + all_text[index]] = one_text.split(",")[1].strip()
        index = index + 1
    return dic_androbench_score


def ParseAndEBenchResult(result_files, andebench_check_points):
    dic_andebench_score = OrderedDict()
    all_text = []
    index = 0 
    while (index < len(andebench_check_points)):
        dic_andebench_score[andebench_check_points[index]] = "NA"
        index = index + 1

    for xml_file_name in result_files:
        root = ElementTree.parse(xml_file_name).getroot()
        all_nodes = root.findall('./node/node/node/node/node/node/node/node/node/node/node/node/node/node')
        for one_node in all_nodes:
            for child in one_node.getchildren():
                for key in child.attrib:
                    if (key == "text"):
                        all_text.append(child.attrib[key].strip())

    for each_text in all_text:
        if ("AndEMark Native:" in each_text):
            total_info = each_text.split("AndEMark Native:")
            if (len(total_info) > 1):
                detail_info = total_info[1].split("AndEMark Java:")
                if (len(detail_info) > 1):
                    dic_andebench_score["Native 4T"] = detail_info[0].strip().replace("?","")
                    dic_andebench_score["Java 4T"] = detail_info[1].strip().replace("?","")
            break
    return dic_andebench_score

def Parse0xbenchResult(result_files, oxbench_check_points):
    dic_oxbench_score = OrderedDict()
    total_text = ""
    index = 0 
    while (index < len(oxbench_check_points)):
        dic_oxbench_score[oxbench_check_points[index]] = "NA"
        index = index + 1

    for xml_file_name in result_files:
        root = ElementTree.parse(xml_file_name).getroot()
        all_nodes = root.findall('./node/node/node/node/node/node/node')
        for one_node in all_nodes:
            for child in one_node.getchildren():
                for key in child.attrib:
                    if (key == "text"):
                        total_text = child.attrib[key].strip()
                        break
    total_info = total_text.split("Linpack")
    for linpack_info in total_info:
        if ("1thread" in linpack_info.lower()):
            linpack1 = linpack_info.split("Time")[0].strip().split(":")
            if (len(linpack1) > 1):
                str_linpack1 = linpack1[1].replace("?", "")
                dic_oxbench_score["Linpack 1T"] = (float)(str_linpack1)
        elif ("2thread" in linpack_info.lower()):
            linpack2 = linpack_info.split("Time")[0].strip().split(":")
            if (len(linpack2) > 1):
                str_linpack2 = linpack2[1].replace("?", "")
                dic_oxbench_score["Linpack 2T"] = (float)(str_linpack2)
        elif ("4thread" in linpack_info.lower()):
            linpack4 = linpack_info.split("Time")[0].strip().split(":")
            if (len(linpack4) > 1):
                str_linpack4 = linpack4[1].replace("?", "")
                dic_oxbench_score["Linpack 4T"] = (float)(str_linpack4)
    return dic_oxbench_score


def ParseCaffeinemarkResult(result_files, caffeinemark_check_points):
    dic_caffeinemark_score = OrderedDict()
    all_text = []
    index = 0 
    while (index < len(caffeinemark_check_points)):
        dic_caffeinemark_score[caffeinemark_check_points[index]] = "NA"
        index = index + 1
    for check_point in caffeinemark_check_points:
        dic_caffeinemark_score[check_point] = "NA"

    # find total score
    for xml_file_name in result_files:
        root = ElementTree.parse(xml_file_name).getroot()
        all_nodes = root.findall('./node/node/node/node/node/node/node')
        for one_node in all_nodes:
            for child in one_node.getchildren():
                for key in child.attrib:
                    if (key == "text"):
                        all_text.append(child.attrib[key].strip())
    index = 0
    while (index < len(all_text) - 1):
        if (all_text[index].lower().strip() == "overall score:"):
            all_text[index] = "Overall Score"      # remove colon
            break
        index = index + 1

    # find each score
    for xml_file_name in result_files:
        root = ElementTree.parse(xml_file_name).getroot()
        all_nodes = root.findall('./node/node/node/node/node/node')
        for one_node in all_nodes:
            for key in one_node.attrib:
                if (key == "text"):
                    all_text.append(one_node.attrib[key].strip())

    index = 0
    while (index < len(all_text) - 1):
        if all_text[index] in caffeinemark_check_points:
            dic_caffeinemark_score[all_text[index]] = all_text[index+1]
        index = index + 1
    return dic_caffeinemark_score


def ParseAntutuResult(result_files, antutu_check_points):
    dic_antutu_score = OrderedDict()
    all_text = []
    index = 0 
    while (index < len(antutu_check_points)):
        dic_antutu_score[antutu_check_points[index]] = "NA"
        index = index + 1
    for check_point in antutu_check_points:
        dic_antutu_score[check_point] = "NA"

    # find total score
    device_name = "skytek_r25"
    for xml_file_name in result_files:
        root = ElementTree.parse(xml_file_name).getroot()
        all_nodes = root.findall('./node/node/node/node/node/node/node/node/node')
        for one_node in all_nodes:
            for child in one_node.getchildren():
                for key in child.attrib:
                    if (key == "text"):
                        all_text.append(child.attrib[key].strip())
                    if (key == "resource-id" and child.attrib[key].strip() == "com.antutu.ABenchMark:id/device_name_text"):
                        device_name = child.attrib["text"].lower().strip()
    index = 0
    while (index < len(all_text) - 1):
        if (all_text[index].lower().strip() == device_name and index >= 3):
            dic_antutu_score["Total"] = all_text[index-2]
            break
        index = index + 1

    # find each scores
    for xml_file_name in result_files:
        root = ElementTree.parse(xml_file_name).getroot()
        all_nodes = root.findall('./node/node/node/node/node/node/node/node/node/node/node/node')
        for one_node in all_nodes:
            for child in one_node.getchildren():
                for key in child.attrib:
                    if key == "text":
                        all_text.append(child.attrib[key].strip())
    index = 0
    while (index < len(all_text) - 1):
        for check_point in antutu_check_points:
            if all_text[index].lower().strip().startswith(check_point.lower().strip()) and dic_antutu_score[check_point] == "":
                if "[" in all_text[index+1] and "]" in all_text[index+1]:           # need to remove [1280*720] of 3D graphics
                    start_num = all_text[index+1].index("]")+1
                    dic_antutu_score[check_point] = all_text[index+1][start_num:].strip()
                else:
                    dic_antutu_score[check_point] = all_text[index+1]
            elif all_text[index].lower().strip() == "cpu integer:":                 # there are two keywords of 'cpu integer'
                dic_antutu_score["CPU integer(single)"] = all_text[index+1]
            elif all_text[index].lower().strip() == "cpu float-point:":             # there are two keywords of 'cpu float-point'
                dic_antutu_score["CPU float-point(single)"] = all_text[index+1]
        index = index + 1
    return dic_antutu_score


def ParseCFBenchResult(result_files, cfbench_check_points):
    dic_cfbench_score = OrderedDict()
    for check_point in cfbench_check_points:
        dic_cfbench_score[check_point] = "NA"

    # find each score in xml files
    all_text = []
    for xml_file_name in result_files:
        root = ElementTree.parse(xml_file_name).getroot()
        all_nodes = root.findall('./node/node/node/node/node/node/node/node')
        for one_node in all_nodes:
            for child in one_node.getchildren():
                for key in child.attrib:
                    if key == "text":
                        all_text.append(child.attrib[key].strip())
    index = 0
    while (index < len(all_text) - 1):
        if all_text[index] in cfbench_check_points:
            dic_cfbench_score[all_text[index]] = all_text[index+1]
        index = index + 1
    return dic_cfbench_score

def WriteResult(init_row, title, total_score, bold, sheet1):
    row = int(init_row)
    column = 0

    sheet1.write(row, column, title, bold)       # write title
    while (column < len(total_score.values()[0])):
        column = column + 1
        sheet1.write(row, column, "Round " + str(column), bold)
    sheet1.write(row, column, "Average", bold)

    for k in total_score.keys():
        row = row + 1
        column = 0
        sheet1.write(row,column,k)          # write each check point
        column = column + 1

    row = int(init_row)
    for key in total_score.keys():
        row = row + 1
        column = 1
        for v in total_score[key]:
            sheet1.write(row,column,v)      # write each scores
            column = column + 1


def MergeAllResults(script_path, timestamp_nm, dic_final_result):
    try:
        summary_file_name = script_path + "\\Summary.xlsx"
        book = xlsxwriter.Workbook(summary_file_name)
        sheet1 = book.add_worksheet("summary")
        bold = book.add_format({'bold': True})

        report_title = "Performance Testing - " + timestamp_nm
        sheet1.write(0,0,report_title,bold)

        row = 1
        for key in dic_final_result:
            if (len(dic_final_result[key]) > 0):
                WriteResult(row, key + " Result", dic_final_result[key], bold, sheet1)
                row = row + len(dic_final_result[key].keys()) + 3

        book.close()
        PrintAndLogInfo("\nGenerate Summary Successfully!")
    except Exception as ex:
        print str(ex)

def MoveResult(script_path, timestamp_nm, output_files):
    result_folder = os.path.join(script_path, timestamp_nm)
    if not os.path.exists(result_folder):
        os.makedirs(result_folder)

    # move output uiautomator xml files into result folder
    for item in output_files:
        for file_names in item["OutputFiles"].values():
            for xml_file_name in file_names:
                if os.path.exists(xml_file_name.strip()):
                    copyFile(xml_file_name, result_folder)
                    deleteFile(xml_file_name)

    # move summary.xlsx into result folder
    summary_file_name = script_path + "\\Summary.xlsx"
    if os.path.exists(summary_file_name):
        copyFile(summary_file_name, result_folder)
        deleteFile(summary_file_name)

    print "\nSummary report: %s" % os.path.join(result_folder, "Summary.xlsx")
