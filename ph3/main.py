
from asyncio.subprocess import DEVNULL
from copy import copy
import subprocess
import json as json
import os
from sys import stderr

# 10 11 12 19 are differn't because of time
CASES = "00 01 02 03 10 11 12 13 14 15 17 18 19 21 22 23 24 25".split(" ")
EXFILEJSON = "expected.json"


def expected_json_dump():
    if os.path.isfile(EXFILEJSON):  # Create JSON of Expected output
        return

    exCasesJson = {}  # Expected Cases Json
    data = []
    cCase = []  # Current Case
    caseName = "00"

    with open("testcases/testResultsSolo.txt") as f_in:  # LOAD FILE IN
        data = f_in.readlines()

    for data in data[1:]:  # SKIP FIRST INDEX
        data = data.strip("\n")  # STRIP \n
        if "starting test " in data:
            exCasesJson[caseName] = cCase
            caseName = data.split(" ")[2]  # Grab Name
            cCase = []
            continue
        elif data != "":
            cCase.append(data)

    exCasesJson[caseName] = cCase
    with open(EXFILEJSON, "w") as f_out:  # DUMP JSON
        json.dump(exCasesJson, f_out)


def process_byte(byteOut):
    strOut = byteOut.decode("utf-8").split("\n")

    strCopy = copy(strOut)
    for i, str in enumerate(strCopy):
        if str == "":
            del strOut[i]
    newStrOut = {}
    for i, str in enumerate(strOut):
        newStrOut[i] = str
    return newStrOut


expected_json_dump()


myCasesJson = {}
exCasesJson = {}

with open(EXFILEJSON, "r") as f_in:
    exCasesJson = json.load(f_in)

for case in CASES:
    try:
        differnt = False
        exCasesDifJson = {}
        #print(f"Starting test {case} ....")
        #subprocess.run(["make", f"test{case}"])
        subprocess.call(f"make test{case}", shell=True,
                        stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
        byteOut = subprocess.check_output(f"./test{case}", shell=True)
        myCaseOut = process_byte(byteOut)

        for i, exCaseLine in enumerate(exCasesJson[case]):
            if len(myCaseOut) <= i or exCaseLine != myCaseOut[i]:
                differnt = True
                exCasesDifJson[i] = exCaseLine
                #print(f"CASE {case}\n{exCaseLine}")

        if differnt == True:
            print(f"CASE {case} HAS CHANGED\n")
            newexCasesJson = {}
            for i, newexcase in enumerate(exCasesJson[case]):
                newexCasesJson[i] = newexcase
            myCasesJson[case] = {"DIF": exCasesDifJson,
                                 "MINE": myCaseOut, "HIS": newexCasesJson}

        # print(strOut)
    except:
        print(f"{case}: Error")
byteOut = subprocess.check_output(f"make clean", shell=True)
with open("differnt.json", "w") as f_out:
    json.dump(myCasesJson, f_out)
