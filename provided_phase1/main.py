
cases = "00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36".split(" ")

#print(testCases)

from logging import exception
import subprocess
from time import sleep

testCaseOutputList = []
for case in cases:
    try:
        print(f"starting test {case} ....\n")
        byteOut = subprocess.check_output(f"make test{case}", shell=True)
        byteOut = subprocess.check_output(f"./test{case}", shell=True)
        strOut = byteOut.decode("utf-8")
        print(strOut)
    except:
        print(f"{case}: Error")
byteOut = subprocess.check_output(f"make clean", shell=True)
#byteOut = subprocess.check_output("make test00", shell=True)

#print(byteOut)
quit()
testResultsTxt = None
with open("testResults.txt", "r") as testResults:
     testResultsTxt = testResults.readlines()
start = 0
for idx,eachline in enumerate(testResultsTxt):
    if "starting test" in eachline:
        if(start == 0):
            start = idx
        else:
            testCaseOutputList.append(testResultsTxt[start+1: idx-1])
print(testCaseOutputList[7])
for idx,case in enumerate(cases):
    print(f"test{case}: Starting to check!")
    byteOut = subprocess.check_output(f"make test{case}", shell=True)
    strOut = byteOut.decode("utf-8")
    for line in strOut:
        if "Error" in line:
            print(f"Failed TestCase {case}")
            continue
        try:
            byteOut = subprocess.check_output(f"./test{case}", shell=True)
            strOut = byteOut.decode("utf-8")
        except:
            print(f"test{case}: Error on running programing:")

        for line in testCaseOutputList[idx]:
            if("():" in line):
                if line in strOut:
                    #print(f"test{case}: {line} was found")
                    break
                else:
                    print(f"test{case}: {line} was not found")
    sleep(2)