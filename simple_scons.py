import glob
import os
import sys
import re


def normalizeLineEnding(fdir):
    # Add trailing / to path
    return fdir if fdir[-1] == os.sep else fdir+os.sep

def getSrcFromFolder(srcDirs, srcPattern, trgtDir):
    #print("Extract files from " , srcDirs)
    res=[]
    trgtDir = normalizeLineEnding(trgtDir)
    for srcF in srcDirs:
        srcF = normalizeLineEnding(srcF)
        print("Process %s folder" % srcF)
        for f in glob.glob(srcF+srcPattern):
             print("  Append %s" %(trgtDir+f))
             res.append(trgtDir+f)
    return tuple(res)

def RegisterSrcFolderInEnv(srcDirs, env, trgtDir):
    for srcF in srcDirs:
        print("Register folder %s" % (srcF))
        srcF = normalizeLineEnding(srcF)
        env.VariantDir(trgtDir+srcF, srcF, duplicate=0)

testComLib = 'ctest'
cutLib  =   'cut'
drvLib  =   'drv'
compile_opt={}
#
# Google test setup from :http://www.solasistim.net/posts/scons_and_google_mock/
#
googletest_framework_root = "test/googletest"

googletest_include_paths = (
    googletest_framework_root + "/googletest",
    googletest_framework_root + "/googletest/include",
    googletest_framework_root + "/googlemock",
    googletest_framework_root + "/googlemock/include"
)

gtest_all_path = googletest_framework_root + "/googletest/src/gtest-all.cc"
gmock_all_path = googletest_framework_root + "/googlemock/src/gmock-all.cc"

debug = ARGUMENTS.get('debug', '0')
target = ARGUMENTS.get('target', '0')
print("Build target is %s" % (target))
print("Debug is %s" % (debug))

testComFiles =()
testCutFiles = ()
cutFolders =()
testCutFolders = ()
genTestFolders = ('./test/',)
incPath  = ('inc/',)
incPath  += ('mcal/',)


linkLibs =(testComLib, cutLib)


if  target == 'simple':
    print("   Create hello world tests.")
    cutFolders += ('./common/src/',)
    testCutFolders = ('./common/test/',)
    ccFlags  = '-DUNIT_TEST '
    incPath +=('common/inc/',)
    incPath +=('common/test/',)
    incPath += ('test/',)
    incPath +=(googletest_include_paths,)
    linkLibs +=('pthread',)   
    linkFlags = '-Xlinker -Map=output.map'
    debug = True
else:
    print("Unknown target {0}".format(target))
    sys.exit()

if debug:
    print("*** Debug build...")
    binFolder = '.'
else:
    print("*** Release build...")
    binFolder = '.'

linkLibs += ('CppUTest','CppUTestExt')
cutFiles = getSrcFromFolder(cutFolders,'simple_*.c',binFolder)
testCutFiles += getSrcFromFolder(testCutFolders,'simple_*.cpp',binFolder)
testComFiles += getSrcFromFolder(genTestFolders,'AllTests.cpp',binFolder)
testComFiles += getSrcFromFolder((googletest_framework_root,), "googletest/src/gtest-all.cc",binFolder)
testComFiles += getSrcFromFolder((googletest_framework_root,), "googlemock/src/gmock-all.cc",binFolder)

print("testComFiles : %s" % " ".join(i for i in testComFiles))
print("testCutFiles : %s" % " ".join(i for i in testCutFiles))
print("cutFiles     : %s" % " ".join(i for i in cutFiles))
print("linkLibs     : %s" % " ".join(i for i in linkLibs))


libPath  = binFolder
ccDebFlags = '-g '
ccFlags  += '-Wall ' + ("" if not debug else " %s" % ccDebFlags)
cflags  =" -std=c11 -fstack-protector-strong"
cxxflags=" -std=c++1z"

env = Environment(variant_dir=binFolder,
                  LIBPATH=binFolder,
                  #LIBS=linkLibs,
                  CPPPATH=incPath, CCFLAGS=ccFlags,
                  CFLAGS=cflags, CXXFLAGS=cxxflags,
                  LINKFLAGS=linkFlags, **compile_opt)

#RegisterSrcFolderInEnv(cutFolders, env, binFolder)
#print("Register library %s with %s" %(cutFolders,str(cutFiles)))
#env.Library(target=cutLib, source= cutFiles)

#RegisterSrcFolderInEnv(testCutFolders, env, binFolder)
#print("Register library %s with %s" %(testCutFolders, str(testCutFiles)))
#env.Library(target=testCutFolders, source= testCutFiles)

#print("Process %s" % genTestFolders)
#RegisterSrcFolderInEnv(genTestFolders, env, binFolder)
#print("Register library %s" %genTestFolders)
#env.Library(target=genTestFolders, source= testComFiles)

print("Build executable %s" % "simple")
env.Program("simple", (cutFiles, testCutFiles, testComFiles))
